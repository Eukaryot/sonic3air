/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/simulation/bindings/RendererBindings.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/PersistentData.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/simulation/SimulationState.h"
#include "oxygen/simulation/analyse/ROMDataAnalyser.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/overlays/DebugSidePanel.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/helper/RandomNumberGenerator.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/PaletteCollection.h"
#include "oxygen/resources/RawDataCollection.h"

#include <lemon/program/ModuleBindingsBuilder.h>
#include <lemon/runtime/Runtime.h>

#include <rmxmedia.h>

#include <iomanip>


namespace
{
	static lemon::FlyweightString FLYWEIGHTSTRING_PERSISTENTDATA("persistentdata");

	namespace detail
	{
		uint32 loadData(EmulatorInterface& emulatorInterface, uint32 targetAddress, const std::vector<uint8>& data, uint32 offset, uint32 maxBytes)
		{
			if (data.empty())
				return 0;

			uint32 bytes = (uint32)data.size();
			if (offset != 0)
			{
				if (offset >= bytes)
					return 0;
				bytes -= offset;
			}
			if (maxBytes != 0)
			{
				bytes = std::min(bytes, maxBytes);
			}

			uint8* dst = emulatorInterface.getMemoryPointer(targetAddress, true, bytes);
			if (nullptr == dst)
				return 0;

			memcpy(dst, &data[offset], bytes);
			return bytes;
		}

		const Mod* getModForCurrentFunction()
		{
			CodeExec* codeExec = CodeExec::getActiveInstance();
			if (nullptr == codeExec)
				return nullptr;

			const lemon::ControlFlow* controlFlow = lemon::Runtime::getActiveControlFlow();
			if (nullptr == controlFlow)
				return nullptr;

			const lemon::ScriptFunction* scriptFunction = controlFlow->getCurrentFunction();
			if (nullptr == scriptFunction)
				return nullptr;

			return codeExec->getLemonScriptProgram().getModByModule(scriptFunction->getModule());
		}
	}


	inline EmulatorInterface& getEmulatorInterface()
	{
		return *lemon::Runtime::getActiveEnvironmentSafe<RuntimeEnvironment>().mEmulatorInterface;
	}

	int64* accessRegister(size_t index)
	{
		uint32& reg = getEmulatorInterface().getRegister(index);
		return reinterpret_cast<int64*>(&reg);
	}

	void scriptAssert1(uint8 condition, lemon::StringRef text)
	{
		if (!condition)
		{
			std::string locationText = LemonScriptRuntime::getCurrentScriptLocationString();
			RMX_ASSERT(!locationText.empty(), "No active lemon script runtime");

			if (text.isValid())
			{
				RMX_ERROR("Script assertion failed:\n'" << text.getString() << "'.\nIn " << locationText << ".", );
			}
			else
			{
				RMX_ERROR("Script assertion failed in " << locationText << ".", );
			}
		}
	}

	void scriptAssert2(uint8 condition)
	{
		scriptAssert1(condition, lemon::StringRef());
	}


	uint8 checkFlags_equal()
	{
		return getEmulatorInterface().getFlagZ();
	}

	uint8 checkFlags_negative()
	{
		return getEmulatorInterface().getFlagN();
	}

	void setZeroFlagByValue(uint32 value)
	{
		// In contrast to the emulator, we use the zero flag in its original form: it gets set when value is zero
		getEmulatorInterface().setFlagZ(value == 0);
	}

	template<typename T>
	void setNegativeFlagByValue(T value)
	{
		const int bits = sizeof(T) * 8;
		getEmulatorInterface().setFlagN((value >> (bits - 1)) != 0);
	}


	void copyMemory(uint32 destAddress, uint32 sourceAddress, uint32 bytes)
	{
		uint8* destPointer = getEmulatorInterface().getMemoryPointer(destAddress, true, bytes);
		uint8* sourcePointer = getEmulatorInterface().getMemoryPointer(sourceAddress, false, bytes);
		memmove(destPointer, sourcePointer, bytes);
	}

	void zeroMemory(uint32 startAddress, uint32 bytes)
	{
		uint8* pointer = getEmulatorInterface().getMemoryPointer(startAddress, true, bytes);
		memset(pointer, 0, bytes);
	}

	void fillMemory_u8(uint32 startAddress, uint32 bytes, uint8 value)
	{
		uint8* pointer = getEmulatorInterface().getMemoryPointer(startAddress, true, bytes);
		for (uint32 i = 0; i < bytes; ++i)
		{
			pointer[i] = value;
		}
	}

	void fillMemory_u16(uint32 startAddress, uint32 bytes, uint16 value)
	{
		RMX_CHECK((startAddress & 0x01) == 0, "Odd address not valid", return);
		RMX_CHECK((bytes & 0x01) == 0, "Odd number of bytes not valid", return);

		uint8* pointer = getEmulatorInterface().getMemoryPointer(startAddress, true, bytes);

		value = (value << 8) + (value >> 8);
		for (uint32 i = 0; i < bytes; i += 2)
		{
			*(uint16*)(&pointer[i]) = value;
		}
	}

	void fillMemory_u32(uint32 startAddress, uint32 bytes, uint32 value)
	{
		RMX_CHECK((startAddress & 0x01) == 0, "Odd address not valid", return);
		RMX_CHECK((bytes & 0x03) == 0, "Number of bytes must be divisible by 4", return);

		uint8* pointer = getEmulatorInterface().getMemoryPointer(startAddress, true, bytes);

		value = ((value & 0x000000ff) << 24)
			  + ((value & 0x0000ff00) << 8)
			  + ((value & 0x00ff0000) >> 8)
			  + ((value & 0xff000000) >> 24);

		for (uint32 i = 0; i < bytes; i += 4)
		{
			*(uint32*)(&pointer[i]) = value;
		}
	}


	void push(uint32 value)
	{
		EmulatorInterface& emulatorInterface = getEmulatorInterface();
		uint32& A7 = emulatorInterface.getRegister(15);
		A7 -= 4;
		emulatorInterface.writeMemory32(A7, value);
	}

	uint32 pop()
	{
		EmulatorInterface& emulatorInterface = getEmulatorInterface();
		uint32& A7 = emulatorInterface.getRegister(15);
		const uint32 result = emulatorInterface.readMemory32(A7);
		A7 += 4;
		return result;
	}


	uint32 System_loadPersistentData(uint32 targetAddress, uint32 bytes, lemon::StringRef file, lemon::StringRef key, bool localFile)
	{
		if (!key.isValid() || key.isEmpty() || !file.isValid())
			return 0;
		if (file.isEmpty())
			file = lemon::StringRef(FLYWEIGHTSTRING_PERSISTENTDATA);

		uint64 fileHash;
		const Mod* mod = localFile ? detail::getModForCurrentFunction() : nullptr;
		if (nullptr != mod)
			fileHash = rmx::getMurmur2_64(mod->mUniqueID + "/" + std::string(file.getString()));
		else
			fileHash = file.getHash();

		const std::vector<uint8>& data = PersistentData::instance().getData(fileHash, key.getHash());
		return detail::loadData(getEmulatorInterface(), targetAddress, data, 0, bytes);
	}

	void System_savePersistentData(uint32 sourceAddress, uint32 bytes, lemon::StringRef file, lemon::StringRef key, bool localFile)
	{
		if (!key.isValid() || key.isEmpty() || !file.isValid())
			return;
		if (file.isEmpty())
			file = lemon::StringRef(FLYWEIGHTSTRING_PERSISTENTDATA);

		const uint8* src = getEmulatorInterface().getMemoryPointer(sourceAddress, false, bytes);
		if (nullptr == src)
			return;

		const size_t size = (size_t)bytes;
		std::vector<uint8> data;
		data.resize(size);
		memcpy(&data[0], src, size);

		const Mod* mod = localFile ? detail::getModForCurrentFunction() : nullptr;
		if (nullptr != mod)
		{
			PersistentData::instance().setData(mod->mUniqueID + "/" + std::string(file.getString()), key.getString(), data);
		}
		else
		{
			PersistentData::instance().setData(file.getString(), key.getString(), data);
		}
	}

	void System_removePersistentData(lemon::StringRef file, lemon::StringRef key, bool localFile)
	{
		if (!key.isValid() || key.isEmpty() || !file.isValid())
			return;
		if (file.isEmpty())
			file = lemon::StringRef(FLYWEIGHTSTRING_PERSISTENTDATA);

		uint64 fileHash;
		const Mod* mod = localFile ? detail::getModForCurrentFunction() : nullptr;
		if (nullptr != mod)
			fileHash = rmx::getMurmur2_64(mod->mUniqueID + "/" + std::string(file.getString()));
		else
			fileHash = file.getHash();

		PersistentData::instance().removeKey(fileHash, key.getHash());
	}

	bool System_callFunctionByName(lemon::StringRef functionName)
	{
		if (!functionName.isValid())
			return false;

		CodeExec* codeExec = CodeExec::getActiveInstance();
		RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return false);
		return codeExec->getLemonScriptRuntime().callFunctionByName(functionName, false);
	}

	void System_setupCallFrame2(lemon::StringRef functionName, lemon::StringRef labelName)
	{
		if (!functionName.isValid())
			return;

		CodeExec* codeExec = CodeExec::getActiveInstance();
		RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
		codeExec->setupCallFrame(functionName.getString(), labelName.getString());
	}

	void System_setupCallFrame1(lemon::StringRef functionName)
	{
		System_setupCallFrame2(functionName, lemon::StringRef());
	}

	int64 System_getGlobalVariableValueByName(lemon::StringRef variableName)
	{
		CodeExec* codeExec = CodeExec::getActiveInstance();
		RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return 0);
		return codeExec->getLemonScriptRuntime().getGlobalVariableValue_int64(variableName);
	}

	void System_setGlobalVariableValueByName(lemon::StringRef variableName, int64 value)
	{
		CodeExec* codeExec = CodeExec::getActiveInstance();
		RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
		codeExec->getLemonScriptRuntime().setGlobalVariableValue_int64(variableName, value);
	}

	uint32 System_rand()
	{
		RandomNumberGenerator& rng = Application::instance().getSimulation().getSimulationState().getRandomNumberGenerator();
		return (uint32)rng.getRandomUint64();
	}

	float System_randomFloat()
	{
		RandomNumberGenerator& rng = Application::instance().getSimulation().getSimulationState().getRandomNumberGenerator();
		return (float)(rng.getRandomUint64() % 8388608) / 8388607.0f;	// 8388608 is 2^23
	}

	int32 System_randRange1(int32 minimum, int32 maximum)
	{
		if (minimum < maximum)
			return minimum + System_rand() % (maximum - minimum + 1);
		else if (minimum > maximum)
			return maximum + System_rand() % (minimum - maximum + 1);
		else
			return minimum;
	}

	float System_randRange2(float minimum, float maximum)
	{
		return minimum + System_randomFloat() * (maximum - minimum);
	}

	uint32 System_getPlatformFlags()
	{
		return EngineMain::instance().getPlatformFlags();
	}

	bool System_hasPlatformFlag(uint32 flag)
	{
		return (System_getPlatformFlags() & flag) != 0;
	}

	bool System_hasExternalRawData(lemon::StringRef key)
	{
		const std::vector<const RawDataCollection::RawData*>& rawDataVector = RawDataCollection::instance().getRawData(key.getHash());
		return !rawDataVector.empty();
	}

	uint32 System_loadExternalRawData1(lemon::StringRef key, uint32 targetAddress, uint32 offset, uint32 maxBytes, bool loadOriginalData, bool loadModdedData)
	{
		const std::vector<const RawDataCollection::RawData*>& rawDataVector = RawDataCollection::instance().getRawData(key.getHash());
		const RawDataCollection::RawData* rawData = nullptr;
		for (int i = (int)rawDataVector.size() - 1; i >= 0; --i)
		{
			const RawDataCollection::RawData* candidate = rawDataVector[i];
			const bool allow = (candidate->mIsModded) ? loadModdedData : loadOriginalData;
			if (allow)
			{
				rawData = candidate;
				break;
			}
		}

		if (nullptr == rawData)
			return 0;

		return detail::loadData(getEmulatorInterface(), targetAddress, rawData->mContent, offset, maxBytes);
	}

	uint32 System_loadExternalRawData2(lemon::StringRef key, uint32 targetAddress)
	{
		return System_loadExternalRawData1(key, targetAddress, 0, 0, true, true);
	}

	bool System_hasExternalPaletteData(lemon::StringRef key, uint8 line)
	{
		const PaletteBase* palette = PaletteCollection::instance().getPalette(key.getHash(), line);
		return (nullptr != palette);
	}

	uint16 System_loadExternalPaletteData(lemon::StringRef key, uint8 line, uint32 targetAddress, uint8 maxColors)
	{
		const PaletteBase* palette = PaletteCollection::instance().getPalette(key.getHash(), line);
		if (nullptr == palette)
			return 0;

		const size_t numColors = std::min<size_t>(palette->getSize(), maxColors);
		if (numColors == 0)
			return 0;

		const uint32* colors = palette->getRawColors();
		uint32* targetPointer = (uint32*)getEmulatorInterface().getMemoryPointer(targetAddress, true, (uint32)numColors * sizeof(uint32));
		for (size_t i = 0; i < numColors; ++i)
		{
			// Maintain ABGR32 color format despite endianness change by swapping bytes
			targetPointer[i] = swapBytes32(colors[i]);
		}
		return (uint16)numColors;
	}


	void debugLogInternal(std::string_view valueString)
	{
		uint32 lineNumber = 0;
		const bool success = LemonScriptRuntime::getCurrentScriptFunction(nullptr, nullptr, &lineNumber, nullptr);
		RMX_ASSERT(success, "Could not determine current script function during logging");

		if (nullptr != LemonScriptBindings::mDebugNotificationInterface)
			LemonScriptBindings::mDebugNotificationInterface->onScriptLog(*String(0, "%04d", lineNumber), valueString);
	}

	void logSetter(int64 value, bool decimal)
	{
		const std::string valueString = decimal ? *String(0, "%d", value) : *String(0, "%08x", value);
		debugLogInternal(valueString);
	}

	template<typename T>
	void debugLogIntSigned(T value)
	{
		if (value < 0)
		{
			debugLogInternal('-' + rmx::hexString(-value, sizeof(T) * 2));
		}
		else
		{
			debugLogInternal(rmx::hexString(value, sizeof(T) * 2));
		}
	}

	template<typename T>
	void debugLogIntUnsigned(T value)
	{
		debugLogInternal(rmx::hexString(value, sizeof(T) * 2));
	}

	void debugLog(lemon::AnyTypeWrapper param)
	{
		switch (param.mType->getClass())
		{
			case lemon::DataTypeDefinition::Class::INTEGER:
			{
				if (param.mType == &lemon::PredefinedDataTypes::INT_8)
				{
					debugLogIntSigned(param.mValue.get<int8>());
				}
				else if (param.mType == &lemon::PredefinedDataTypes::UINT_8)
				{
					debugLogIntUnsigned(param.mValue.get<uint8>());
				}
				else if (param.mType == &lemon::PredefinedDataTypes::INT_16)
				{
					debugLogIntSigned(param.mValue.get<int16>());
				}
				else if (param.mType == &lemon::PredefinedDataTypes::UINT_16)
				{
					debugLogIntUnsigned(param.mValue.get<uint16>());
				}
				else if (param.mType == &lemon::PredefinedDataTypes::INT_32)
				{
					debugLogIntSigned(param.mValue.get<int32>());
				}
				else if (param.mType == &lemon::PredefinedDataTypes::UINT_32)
				{
					debugLogIntUnsigned(param.mValue.get<uint32>());
				}
				else if (param.mType == &lemon::PredefinedDataTypes::INT_64)
				{
					debugLogIntSigned(param.mValue.get<int64>());
				}
				else
				{
					debugLogIntUnsigned(param.mValue.get<uint64>());
				}
				break;
			}

			case lemon::DataTypeDefinition::Class::FLOAT:
			{
				std::stringstream str;
				if (param.mType->getBytes() == 4)
				{
					str << param.mValue.get<float>();
				}
				else
				{
					str << param.mValue.get<double>();
				}
				debugLogInternal(str.str());
				break;
			}

			case lemon::DataTypeDefinition::Class::STRING:
			{
				lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
				if (nullptr != runtime)
				{
					const lemon::FlyweightString* str = runtime->resolveStringByKey(param.mValue.get<uint64>());
					if (nullptr != str)
					{
						debugLogInternal(str->getString());
					}
				}
				break;
			}

			default:
				break;
		}
	}

	void debugLogColors(lemon::StringRef name, uint32 startAddress, uint8 numColors)
	{
		if (EngineMain::getDelegate().useDeveloperFeatures() && name.isValid())
		{
			CodeExec* codeExec = CodeExec::getActiveInstance();
			RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
			codeExec->getDebugTracking().addColorLogEntry(name.getString(), startAddress, numColors);
		}
	}

	void debugLogValueStack()
	{
		const size_t valueStackSize = Application::instance().getSimulation().getCodeExec().getLemonScriptRuntime().getInternalLemonRuntime().getActiveControlFlow()->getValueStackSize();
		const std::string valueString = *String(0, "Value Stack Size = %d", valueStackSize);
		debugLogInternal(valueString);
	}


	uint16 Input_getController(uint8 controllerIndex)
	{
		return ControlsIn::instance().getGamepad((size_t)controllerIndex).mCurrentInput;
	}

	uint16 Input_getControllerPrevious(uint8 controllerIndex)
	{
		return ControlsIn::instance().getGamepad((size_t)controllerIndex).mPreviousInput;
	}

	bool getButtonState(int index, bool previousValue = false)
	{
		const size_t playerIndex = (index & 0x10) ? 1 : 0;
		const ControlsIn::Gamepad& gamepad = ControlsIn::instance().getGamepad(playerIndex);
		const uint16 bitmask = previousValue ? gamepad.mPreviousInput : gamepad.mCurrentInput;
		return ((bitmask >> (index & 0x0f)) & 1) != 0;
	}

	uint8 Input_buttonDown(uint8 index)
	{
		// Button down right now
		return getButtonState((int)index) ? 1 : 0;
	}

	uint8 Input_buttonPressed(uint8 index)
	{
		// Button down now, but not in previous frame
		return (getButtonState((int)index) && !getButtonState(index, true)) ? 1 : 0;
	}

	void Input_setTouchInputMode(uint8 mode)
	{
		return InputManager::instance().setTouchInputMode((InputManager::TouchInputMode)mode);
	}

	void Input_resetControllerRumble(int8 playerIndex)
	{
		if (playerIndex < 0)
		{
			// All players
			InputManager::instance().resetControllerRumbleForPlayer(0);
			InputManager::instance().resetControllerRumbleForPlayer(1);
		}
		else if (playerIndex < 2)
		{
			InputManager::instance().resetControllerRumbleForPlayer(playerIndex);
		}
	}

	void Input_setControllerRumble(int8 playerIndex, float lowFrequencyRumble, float highFrequencyRumble, uint16 milliseconds)
	{
		// Limit length to 30 seconds
		milliseconds = std::min<uint16>(milliseconds, 30000);
		if (playerIndex < 0)
		{
			// All players
			InputManager::instance().setControllerRumbleForPlayer(0, lowFrequencyRumble, highFrequencyRumble, milliseconds);
			InputManager::instance().setControllerRumbleForPlayer(1, lowFrequencyRumble, highFrequencyRumble, milliseconds);
		}
		else if (playerIndex < 2)
		{
			InputManager::instance().setControllerRumbleForPlayer(playerIndex, lowFrequencyRumble, highFrequencyRumble, milliseconds);
		}
	}

	void Input_setControllerLEDs(uint8 playerIndex, uint32 color)
	{
		InputManager::instance().setControllerLEDsForPlayer(playerIndex, Color::fromABGR32(color));
	}


	void yieldExecution()
	{
		CodeExec* codeExec = CodeExec::getActiveInstance();
		RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
		codeExec->yieldExecution();
	}


	uint8 Audio_getAudioKeyType(uint64 sfxId)
	{
		return (uint8)EngineMain::instance().getAudioOut().getAudioKeyType(sfxId);
	}

	bool Audio_isPlayingAudio(uint64 sfxId)
	{
		return EngineMain::instance().getAudioOut().isPlayingSfxId(sfxId);
	}

	void Audio_playAudio1(uint64 sfxId, uint8 contextId)
	{
		const bool success = EngineMain::instance().getAudioOut().playAudioBase(sfxId, contextId);
		if (!success)
		{
			// Audio collections expect lowercase IDs, so we might need to do the conversion here first
			lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
			if (nullptr != runtime)
			{
				const lemon::FlyweightString* str = runtime->resolveStringByKey(sfxId);
				if (nullptr != str)
				{
					const std::string_view textString = str->getString();

					// Does the string contain any uppercase letters?
					if (containsByPredicate(textString, [](char ch) { return (ch >= 'A' && ch <= 'Z'); } ))
					{
						// Convert to lowercase and try again
						String tempStr = textString;
						tempStr.lowerCase();
						sfxId = rmx::getMurmur2_64(tempStr);
						EngineMain::instance().getAudioOut().playAudioBase(sfxId, contextId);
					}
				}
			}
		}
	}

	void Audio_playAudio2(uint64 sfxId)
	{
		Audio_playAudio1(sfxId, 0x01);	// In-game sound effect context
	}

	void Audio_stopChannel(uint8 channel)
	{
		EngineMain::instance().getAudioOut().stopChannel(channel);
	}

	void Audio_fadeInChannel(uint8 channel, float seconds)
	{
		EngineMain::instance().getAudioOut().fadeInChannel(channel, seconds);
	}

	void Audio_fadeInChannel2(uint8 channel, uint16 length)
	{
		EngineMain::instance().getAudioOut().fadeInChannel(channel, (float)length / 256.0f);
	}

	void Audio_fadeOutChannel(uint8 channel, float seconds)
	{
		EngineMain::instance().getAudioOut().fadeOutChannel(channel, seconds);
	}

	void Audio_fadeOutChannel2(uint8 channel, uint16 length)
	{
		EngineMain::instance().getAudioOut().fadeOutChannel(channel, (float)length / 256.0f);
	}

	void Audio_playOverride(uint64 sfxId, uint8 contextId, uint8 channelId, uint8 overriddenChannelId)
	{
		EngineMain::instance().getAudioOut().playOverride(sfxId, contextId, channelId, overriddenChannelId);
	}

	void Audio_enableAudioModifier(uint8 channel, uint8 context, lemon::StringRef postfix, float relativeSpeed)
	{
		if (postfix.isValid())
		{
			EngineMain::instance().getAudioOut().enableAudioModifier(channel, context, postfix.getString(), relativeSpeed);
		}
	}

	void Audio_enableAudioModifier2(uint8 channel, uint8 context, lemon::StringRef postfix, uint32 relativeSpeed)
	{
		if (postfix.isValid())
		{
			EngineMain::instance().getAudioOut().enableAudioModifier(channel, context, postfix.getString(), (float)relativeSpeed / 65536.0f);
		}
	}

	void Audio_disableAudioModifier(uint8 channel, uint8 context)
	{
		EngineMain::instance().getAudioOut().disableAudioModifier(channel, context);
	}


	const Mod* getActiveModByNameHash(lemon::StringRef modName)
	{
		if (modName.isValid())
		{
			Mod*const* modPtr = mapFind(ModManager::instance().getActiveModsByNameHash(), modName.getHash());
			if (nullptr != modPtr)
				return *modPtr;
		}
		return nullptr;
	}

	uint8 Mods_isModActive(lemon::StringRef modName)
	{
		const Mod* mod = getActiveModByNameHash(modName);
		return (nullptr != mod);
	}

	int32 Mods_getModPriority(lemon::StringRef modName)
	{
		const Mod* mod = getActiveModByNameHash(modName);
		return (nullptr != mod) ? (int32)mod->mActivePriority : -1;
	}


	uint64 debugKeyGetter(int index)
	{
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			const int key = index + '0';
			return (FTX::keyState(key) && FTX::keyChange(key) && !FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT)) ? 1 : 0;
		}
		else
		{
			return 0;
		}
	}


	void debugWatch(uint32 address, uint16 bytes)
	{
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			CodeExec* codeExec = CodeExec::getActiveInstance();
			RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
			codeExec->getDebugTracking().addWatch(address, bytes, false);
		}
	}


	void debugDumpToFile(lemon::StringRef filename, uint32 startAddress, uint32 bytes)
	{
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			CodeExec* codeExec = CodeExec::getActiveInstance();
			RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
			EmulatorInterface& emulatorInterface = codeExec->getEmulatorInterface();
			const bool isValid = emulatorInterface.isValidMemoryRegion(startAddress, bytes);
			RMX_CHECK(isValid, "No valid memory region for debugDumpToFile: startAddress = " << rmx::hexString(startAddress, 6) << ", bytes = " << rmx::hexString(bytes, 2), return);

			if (filename.isValid())
			{
				const uint8* src = emulatorInterface.getMemoryPointer(startAddress, false, bytes);
				FTX::FileSystem->saveFile(filename.getString(), src, (size_t)bytes);
			}
		}
	}


	bool ROMDataAnalyser_isEnabled()
	{
		return Configuration::instance().mEnableROMDataAnalyser;
	}

	bool ROMDataAnalyser_hasEntry(lemon::StringRef category, uint32 address)
	{
		if (Configuration::instance().mEnableROMDataAnalyser)
		{
			ROMDataAnalyser* analyser = Application::instance().getSimulation().getROMDataAnalyser();
			if (nullptr != analyser)
			{
				if (category.isValid())
				{
					return analyser->hasEntry(category.getString(), address);
				}
			}
		}
		return false;
	}

	void ROMDataAnalyser_beginEntry(lemon::StringRef category, uint32 address)
	{
		if (Configuration::instance().mEnableROMDataAnalyser)
		{
			ROMDataAnalyser* analyser = Application::instance().getSimulation().getROMDataAnalyser();
			if (nullptr != analyser)
			{
				if (category.isValid())
				{
					analyser->beginEntry(category.getString(), address);
				}
			}
		}
	}

	void ROMDataAnalyser_endEntry()
	{
		if (Configuration::instance().mEnableROMDataAnalyser)
		{
			ROMDataAnalyser* analyser = Application::instance().getSimulation().getROMDataAnalyser();
			if (nullptr != analyser)
			{
				analyser->endEntry();
			}
		}
	}

	void ROMDataAnalyser_addKeyValue(lemon::StringRef key, lemon::StringRef value)
	{
		if (Configuration::instance().mEnableROMDataAnalyser)
		{
			ROMDataAnalyser* analyser = Application::instance().getSimulation().getROMDataAnalyser();
			if (nullptr != analyser)
			{
				if (key.isValid() && value.isValid())
				{
					analyser->addKeyValue(key.getString(), value.getString());
				}
			}
		}
	}

	void ROMDataAnalyser_beginObject(lemon::StringRef key)
	{
		if (Configuration::instance().mEnableROMDataAnalyser)
		{
			ROMDataAnalyser* analyser = Application::instance().getSimulation().getROMDataAnalyser();
			if (nullptr != analyser)
			{
				if (key.isValid())
				{
					analyser->beginObject(key.getString());
				}
			}
		}
	}

	void ROMDataAnalyser_endObject()
	{
		if (Configuration::instance().mEnableROMDataAnalyser)
		{
			ROMDataAnalyser* analyser = Application::instance().getSimulation().getROMDataAnalyser();
			if (nullptr != analyser)
			{
				analyser->endObject();
			}
		}
	}

	bool System_SidePanel_setupCustomCategory(lemon::StringRef shortName, lemon::StringRef fullName)
	{
		if (!shortName.isValid())
			return false;
		return Application::instance().getDebugSidePanel()->setupCustomCategory(fullName.getString(), shortName.getString()[0]);
	}

	bool System_SidePanel_addOption(lemon::StringRef text, bool defaultValue)
	{
		if (!text.isValid())
			return false;
		return Application::instance().getDebugSidePanel()->addOption(text.getString(), defaultValue);
	}

	void System_SidePanel_addEntry(uint64 key)
	{
		return Application::instance().getDebugSidePanel()->addEntry(key);
	}

	void System_SidePanel_addLine1(lemon::StringRef text, int8 indent, uint32 color)
	{
		if (text.isValid())
		{
			Application::instance().getDebugSidePanel()->addLine(text.getString(), (int)indent, Color::fromRGBA32(color));
		}
	}

	void System_SidePanel_addLine2(lemon::StringRef str, int8 indent)
	{
		System_SidePanel_addLine1(str, indent, 0xffffffff);
	}

	bool System_SidePanel_isEntryHovered(uint64 key)
	{
		return Application::instance().getDebugSidePanel()->isEntryHovered(key);
	}

	void System_writeDisplayLine(lemon::StringRef text)
	{
		if (text.isValid())
		{
			LogDisplay::instance().setLogDisplay(text.getString(), 2.0f);
		}
	}

}


void LemonScriptBindings::registerBindings(lemon::Module& module)
{
	lemon::ModuleBindingsBuilder builder(module);

	// Basic functions
	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);
	builder.addNativeFunction("assert", lemon::wrap(&scriptAssert1), defaultFlags);
	builder.addNativeFunction("assert", lemon::wrap(&scriptAssert2), defaultFlags);

	// Emulator interface bindings
	{
		// Register access
		const std::string registerNamesDAR[16] = { "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7" };
		for (size_t i = 0; i < 16; ++i)
		{
			module.addExternalVariable(registerNamesDAR[i],			 &lemon::PredefinedDataTypes::UINT_32, std::bind(accessRegister, i));
			module.addExternalVariable(registerNamesDAR[i] + ".u8",  &lemon::PredefinedDataTypes::UINT_8,  std::bind(accessRegister, i));
			module.addExternalVariable(registerNamesDAR[i] + ".s8",  &lemon::PredefinedDataTypes::INT_8,   std::bind(accessRegister, i));
			module.addExternalVariable(registerNamesDAR[i] + ".u16", &lemon::PredefinedDataTypes::UINT_16, std::bind(accessRegister, i));
			module.addExternalVariable(registerNamesDAR[i] + ".s16", &lemon::PredefinedDataTypes::INT_16,  std::bind(accessRegister, i));
			module.addExternalVariable(registerNamesDAR[i] + ".u32", &lemon::PredefinedDataTypes::UINT_32, std::bind(accessRegister, i));
			module.addExternalVariable(registerNamesDAR[i] + ".s32", &lemon::PredefinedDataTypes::INT_32,  std::bind(accessRegister, i));
		}

		// Query flags
		builder.addNativeFunction("_equal", lemon::wrap(&checkFlags_equal), defaultFlags);
		builder.addNativeFunction("_negative", lemon::wrap(&checkFlags_negative), defaultFlags);

		// Explictly set flags
		builder.addNativeFunction("_setZeroFlagByValue", lemon::wrap(&setZeroFlagByValue), defaultFlags)
			.setParameters("value");

		builder.addNativeFunction("_setNegativeFlagByValue", lemon::wrap(&setNegativeFlagByValue<int8>), defaultFlags)
			.setParameters("value");

		builder.addNativeFunction("_setNegativeFlagByValue", lemon::wrap(&setNegativeFlagByValue<int16>), defaultFlags)
			.setParameters("value");

		builder.addNativeFunction("_setNegativeFlagByValue", lemon::wrap(&setNegativeFlagByValue<int32>), defaultFlags)
			.setParameters("value");


		// Memory access
		builder.addNativeFunction("copyMemory", lemon::wrap(&copyMemory), defaultFlags)
			.setParameters("destAddress", "sourceAddress", "bytes");

		builder.addNativeFunction("zeroMemory", lemon::wrap(&zeroMemory), defaultFlags)
			.setParameters("startAddress", "bytes");

		builder.addNativeFunction("fillMemory_u8", lemon::wrap(&fillMemory_u8), defaultFlags)
			.setParameters("startAddress", "bytes", "value");

		builder.addNativeFunction("fillMemory_u16", lemon::wrap(&fillMemory_u16), defaultFlags)
			.setParameters("startAddress", "bytes", "value");

		builder.addNativeFunction("fillMemory_u32", lemon::wrap(&fillMemory_u32), defaultFlags)
			.setParameters("startAddress", "bytes", "value");


		// Push and pop
		builder.addNativeFunction("push", lemon::wrap(&push), defaultFlags);
		builder.addNativeFunction("pop", lemon::wrap(&pop), defaultFlags);


		// Persistent data
		builder.addNativeFunction("System.loadPersistentData", lemon::wrap(&System_loadPersistentData), defaultFlags)
			.setParameters("targetAddress", "bytes", "file", "key", "localFile");

		builder.addNativeFunction("System.savePersistentData", lemon::wrap(&System_savePersistentData), defaultFlags)
			.setParameters("sourceAddress", "bytes", "file", "key", "localFile");

		builder.addNativeFunction("System.removePersistentData", lemon::wrap(&System_removePersistentData), defaultFlags)
			.setParameters("file", "key", "localFile");


		// System
		builder.addNativeFunction("System.callFunctionByName", lemon::wrap(&System_callFunctionByName))	// Should not get inline executed
			.setParameters("functionName");

		builder.addNativeFunction("System.setupCallFrame", lemon::wrap(&System_setupCallFrame1))		// Should not get inline executed
			.setParameters("functionName");

		builder.addNativeFunction("System.setupCallFrame", lemon::wrap(&System_setupCallFrame2))		// Should not get inline executed
			.setParameters("functionName", "labelName");

		builder.addNativeFunction("System.getGlobalVariableValueByName", lemon::wrap(&System_getGlobalVariableValueByName), defaultFlags)
			.setParameters("variableName");

		builder.addNativeFunction("System.setGlobalVariableValueByName", lemon::wrap(&System_setGlobalVariableValueByName), defaultFlags)
			.setParameters("variableName", "value");

		builder.addNativeFunction("System.rand", lemon::wrap(&System_rand), defaultFlags);

		builder.addNativeFunction("System.randomFloat", lemon::wrap(&System_randomFloat), defaultFlags);

		builder.addNativeFunction("System.randRange", lemon::wrap(&System_randRange1), defaultFlags)
			.setParameters("minimum", "maximum");

		builder.addNativeFunction("System.randRange", lemon::wrap(&System_randRange2), defaultFlags)
			.setParameters("minimum", "maximum");

		builder.addNativeFunction("System.getPlatformFlags", lemon::wrap(&System_getPlatformFlags), defaultFlags);

		builder.addNativeFunction("System.hasPlatformFlag", lemon::wrap(&System_hasPlatformFlag), defaultFlags)
			.setParameters("flag");


		// Access external data
		builder.addNativeFunction("System.hasExternalRawData", lemon::wrap(&System_hasExternalRawData), defaultFlags)
			.setParameters("key");

		builder.addNativeFunction("System.loadExternalRawData", lemon::wrap(&System_loadExternalRawData1), defaultFlags)
			.setParameters("key", "targetAddress", "offset", "maxBytes", "loadOriginalData", "loadModdedData");

		builder.addNativeFunction("System.loadExternalRawData", lemon::wrap(&System_loadExternalRawData2), defaultFlags)
			.setParameters("key", "targetAddress");

		builder.addNativeFunction("System.hasExternalPaletteData", lemon::wrap(&System_hasExternalPaletteData), defaultFlags)
			.setParameters("key", "line");

		builder.addNativeFunction("System.loadExternalPaletteData", lemon::wrap(&System_loadExternalPaletteData), defaultFlags)
			.setParameters("key", "line", "targetAddress", "maxColors");
	}

	// High-level functionality
	{
		// Input
		builder.addNativeFunction("Input.getController", lemon::wrap(&Input_getController), defaultFlags)
			.setParameters("controllerIndex");

		builder.addNativeFunction("Input.getControllerPrevious", lemon::wrap(&Input_getControllerPrevious), defaultFlags)
			.setParameters("controllerIndex");

		builder.addNativeFunction("buttonDown", lemon::wrap(&Input_buttonDown), defaultFlags)			// Deprecated
			.setParameters("index");

		builder.addNativeFunction("buttonPressed", lemon::wrap(&Input_buttonPressed), defaultFlags)		// Deprecated
			.setParameters("index");

		builder.addNativeFunction("Input.buttonDown", lemon::wrap(&Input_buttonDown), defaultFlags)
			.setParameters("index");

		builder.addNativeFunction("Input.buttonPressed", lemon::wrap(&Input_buttonPressed), defaultFlags)
			.setParameters("index");

		builder.addNativeFunction("Input.setTouchInputMode", lemon::wrap(&Input_setTouchInputMode), defaultFlags)
			.setParameters("mode");

		builder.addNativeFunction("Input.resetControllerRumble", lemon::wrap(&Input_resetControllerRumble), defaultFlags)
			.setParameters("playerIndex");

		builder.addNativeFunction("Input.setControllerRumble", lemon::wrap(&Input_setControllerRumble), defaultFlags)
			.setParameters("playerIndex", "lowFrequencyRumble", "highFrequencyRumble", "milliseconds");

		builder.addNativeFunction("Input.setControllerLEDs", lemon::wrap(&Input_setControllerLEDs), defaultFlags)
			.setParameters("playerIndex", "color");

		// Yield
		builder.addNativeFunction("yieldExecution", lemon::wrap(&yieldExecution));	// Should not get inline executed
	}

	// Renderer bindings
	RendererBindings::registerBindings(module);

	{
		// Audio
		builder.addNativeFunction("Audio.getAudioKeyType", lemon::wrap(&Audio_getAudioKeyType), defaultFlags)
			.setParameters("sfxId");

		builder.addNativeFunction("Audio.isPlayingAudio", lemon::wrap(&Audio_isPlayingAudio), defaultFlags)
			.setParameters("sfxId");

		builder.addNativeFunction("Audio.playAudio", lemon::wrap(&Audio_playAudio1), defaultFlags)
			.setParameters("sfxId", "contextId");

		builder.addNativeFunction("Audio.playAudio", lemon::wrap(&Audio_playAudio2), defaultFlags)
			.setParameters("sfxId");

		builder.addNativeFunction("Audio.stopChannel", lemon::wrap(&Audio_stopChannel), defaultFlags)
			.setParameters("channel");

		builder.addNativeFunction("Audio.fadeInChannel", lemon::wrap(&Audio_fadeInChannel), defaultFlags)
			.setParameters("channel", "seconds");

		builder.addNativeFunction("Audio.fadeInChannel", lemon::wrap(&Audio_fadeInChannel2), defaultFlags)
			.setParameters("channel", "length");

		builder.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel), defaultFlags)
			.setParameters("channel", "seconds");

		builder.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel2), defaultFlags)
			.setParameters("channel", "length");

		builder.addNativeFunction("Audio.playOverride", lemon::wrap(&Audio_playOverride), defaultFlags)
			.setParameters("sfxId", "contextId", "channelId", "overriddenChannelId");

		builder.addNativeFunction("Audio.enableAudioModifier", lemon::wrap(&Audio_enableAudioModifier), defaultFlags)
			.setParameters("channel", "context", "postfix", "relativeSpeed");

		builder.addNativeFunction("Audio.enableAudioModifier", lemon::wrap(&Audio_enableAudioModifier2), defaultFlags)
			.setParameters("channel", "context", "postfix", "relativeSpeed");

		builder.addNativeFunction("Audio.disableAudioModifier", lemon::wrap(&Audio_disableAudioModifier), defaultFlags)
			.setParameters("channel", "context");


		// Misc
		builder.addNativeFunction("Mods.isModActive", lemon::wrap(&Mods_isModActive), defaultFlags)
			.setParameters("modName");

		builder.addNativeFunction("Mods.getModPriority", lemon::wrap(&Mods_getModPriority), defaultFlags)
			.setParameters("modName");
	}

	// Debug features
	{
		// Debug log output
		{
			lemon::UserDefinedVariable& var = module.addUserDefinedVariable("Log", &lemon::PredefinedDataTypes::UINT_32);
			var.mSetter = std::bind(logSetter, std::placeholders::_1, false);
		}
		{
			lemon::UserDefinedVariable& var = module.addUserDefinedVariable("LogDec", &lemon::PredefinedDataTypes::UINT_32);
			var.mSetter = std::bind(logSetter, std::placeholders::_1, true);
		}

		builder.addNativeFunction("debugLog", lemon::wrap(&debugLog), defaultFlags)
			.setParameters("value");

		builder.addNativeFunction("debugLogColors", lemon::wrap(&debugLogColors), defaultFlags)
			.setParameters("name", "startAddress", "numColors");

	#if 0
		// Only for debugging value stack issues in lemonscript itself
		builder.addNativeFunction("debugLogValueStack", lemon::wrap(&debugLogValueStack), defaultFlags);
	#endif


		// Debug keys
		for (int i = 0; i < 10; ++i)
		{
			lemon::UserDefinedVariable& var = module.addUserDefinedVariable("Key" + std::to_string(i), &lemon::PredefinedDataTypes::UINT_8);
			var.mGetter = std::bind(debugKeyGetter, i);
		}


		// Watches
		builder.addNativeFunction("debugWatch", lemon::wrap(&debugWatch), defaultFlags)
			.setParameters("address", "bytes");


		// Dump to file
		builder.addNativeFunction("debugDumpToFile", lemon::wrap(&debugDumpToFile), defaultFlags)
			.setParameters("filename", "startAddress", "bytes");


		// ROM data analyser
		builder.addNativeFunction("ROMDataAnalyser.isEnabled", lemon::wrap(&ROMDataAnalyser_isEnabled), defaultFlags);

		builder.addNativeFunction("ROMDataAnalyser.hasEntry", lemon::wrap(&ROMDataAnalyser_hasEntry), defaultFlags)
			.setParameters("category", "address");

		builder.addNativeFunction("ROMDataAnalyser.beginEntry", lemon::wrap(&ROMDataAnalyser_beginEntry), defaultFlags)
			.setParameters("category", "address");

		builder.addNativeFunction("ROMDataAnalyser.endEntry", lemon::wrap(&ROMDataAnalyser_endEntry), defaultFlags);

		builder.addNativeFunction("ROMDataAnalyser.addKeyValue", lemon::wrap(&ROMDataAnalyser_addKeyValue), defaultFlags)
			.setParameters("key", "value");

		builder.addNativeFunction("ROMDataAnalyser.beginObject", lemon::wrap(&ROMDataAnalyser_beginObject), defaultFlags)
			.setParameters("key");

		builder.addNativeFunction("ROMDataAnalyser.endObject", lemon::wrap(&ROMDataAnalyser_endObject), defaultFlags);


		// Debug side panel
		builder.addNativeFunction("System.SidePanel.setupCustomCategory", lemon::wrap(&System_SidePanel_setupCustomCategory), defaultFlags)
			.setParameters("shortName", "fullName");

		builder.addNativeFunction("System.SidePanel.addOption", lemon::wrap(&System_SidePanel_addOption), defaultFlags)
			.setParameters("text", "defaultValue");

		builder.addNativeFunction("System.SidePanel.addEntry", lemon::wrap(&System_SidePanel_addEntry), defaultFlags)
			.setParameters("key");

		builder.addNativeFunction("System.SidePanel.addLine", lemon::wrap(&System_SidePanel_addLine1), defaultFlags)
			.setParameters("text", "indent", "color");

		builder.addNativeFunction("System.SidePanel.addLine", lemon::wrap(&System_SidePanel_addLine2), defaultFlags)
			.setParameters("text", "indent");

		builder.addNativeFunction("System.SidePanel.isEntryHovered", lemon::wrap(&System_SidePanel_isEntryHovered), defaultFlags)
			.setParameters("key");


		// This is not really debugging-related, as it's meant to be written in non-developer environment as well
		builder.addNativeFunction("System.writeDisplayLine", lemon::wrap(&System_writeDisplayLine), defaultFlags)
			.setParameters("text");
	}

	// Register game-specific script bindings
	EngineMain::getDelegate().registerScriptBindings(module);
}

void LemonScriptBindings::setDebugNotificationInterface(DebugNotificationInterface* debugNotificationInterface)
{
	mDebugNotificationInterface = debugNotificationInterface;
}
