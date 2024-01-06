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
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/PersistentData.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/simulation/analyse/ROMDataAnalyser.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/overlays/DebugSidePanel.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/resources/SpriteCache.h"

#include <lemon/program/FunctionWrapper.h>
#include <lemon/program/Module.h>
#include <lemon/runtime/Runtime.h>

#include <rmxmedia.h>

#include <iomanip>


namespace
{
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


	uint32 System_loadPersistentData(uint32 targetAddress, lemon::StringRef key, uint32 maxBytes)
	{
		const std::vector<uint8>& data = PersistentData::instance().getData(key.getHash());
		return detail::loadData(getEmulatorInterface(), targetAddress, data, 0, maxBytes);
	}

	void System_savePersistentData(uint32 sourceAddress, lemon::StringRef key, uint32 bytes)
	{
		const uint8* src = getEmulatorInterface().getMemoryPointer(sourceAddress, false, bytes);
		if (nullptr == src)
			return;

		const size_t size = (size_t)bytes;
		std::vector<uint8> data;
		data.resize(size);
		memcpy(&data[0], src, size);
		if (key.isValid())
		{
			PersistentData::instance().setData(key.getString(), data);
		}
	}

	uint32 SRAM_load(uint32 address, uint16 offset, uint16 bytes)
	{
		return (uint32)getEmulatorInterface().loadSRAM(address, (size_t)offset, (size_t)bytes);
	}

	void SRAM_save(uint32 address, uint16 offset, uint16 bytes)
	{
		getEmulatorInterface().saveSRAM(address, (size_t)offset, (size_t)bytes);
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
		RMX_ASSERT(RAND_MAX >= 0x0800, "RAND_MAX not high enough on this platform, adjustments needed");
		return ((uint32)(rand() & 0x03ff) << 22) + ((uint32)(rand() & 0x07ff) << 11) + (uint32)(rand() & 0x07ff);
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
		const std::vector<const ResourcesCache::RawData*>& rawDataVector = ResourcesCache::instance().getRawData(key.getHash());
		return !rawDataVector.empty();
	}

	uint32 System_loadExternalRawData1(lemon::StringRef key, uint32 targetAddress, uint32 offset, uint32 maxBytes, bool loadOriginalData, bool loadModdedData)
	{
		const std::vector<const ResourcesCache::RawData*>& rawDataVector = ResourcesCache::instance().getRawData(key.getHash());
		const ResourcesCache::RawData* rawData = nullptr;
		for (int i = (int)rawDataVector.size() - 1; i >= 0; --i)
		{
			const ResourcesCache::RawData* candidate = rawDataVector[i];
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
		const ResourcesCache::Palette* palette = ResourcesCache::instance().getPalette(key.getHash(), line);
		return (nullptr != palette);
	}

	uint16 System_loadExternalPaletteData(lemon::StringRef key, uint8 line, uint32 targetAddress, uint8 maxColors)
	{
		const ResourcesCache::Palette* palette = ResourcesCache::instance().getPalette(key.getHash(), line);
		if (nullptr == palette)
			return 0;

		const std::vector<Color>& colors = palette->mColors;
		const size_t numColors = std::min<size_t>(colors.size(), maxColors);
		uint32* targetPointer = (uint32*)getEmulatorInterface().getMemoryPointer(targetAddress, true, (uint32)numColors * 4);
		for (size_t i = 0; i < numColors; ++i)
		{
			targetPointer[i] = palette->mColors[i].getRGBA32();
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
	// Basic functions
	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);
	module.addNativeFunction("assert", lemon::wrap(&scriptAssert1), defaultFlags);
	module.addNativeFunction("assert", lemon::wrap(&scriptAssert2), defaultFlags);

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
		module.addNativeFunction("_equal", lemon::wrap(&checkFlags_equal), defaultFlags);
		module.addNativeFunction("_negative", lemon::wrap(&checkFlags_negative), defaultFlags);

		// Explictly set flags
		module.addNativeFunction("_setZeroFlagByValue", lemon::wrap(&setZeroFlagByValue), defaultFlags)
			.setParameterInfo(0, "value");

		module.addNativeFunction("_setNegativeFlagByValue", lemon::wrap(&setNegativeFlagByValue<int8>), defaultFlags)
			.setParameterInfo(0, "value");

		module.addNativeFunction("_setNegativeFlagByValue", lemon::wrap(&setNegativeFlagByValue<int16>), defaultFlags)
			.setParameterInfo(0, "value");

		module.addNativeFunction("_setNegativeFlagByValue", lemon::wrap(&setNegativeFlagByValue<int32>), defaultFlags)
			.setParameterInfo(0, "value");


		// Memory access
		module.addNativeFunction("copyMemory", lemon::wrap(&copyMemory), defaultFlags)
			.setParameterInfo(0, "destAddress")
			.setParameterInfo(1, "sourceAddress")
			.setParameterInfo(2, "bytes");

		module.addNativeFunction("zeroMemory", lemon::wrap(&zeroMemory), defaultFlags)
			.setParameterInfo(0, "startAddress")
			.setParameterInfo(1, "bytes");

		module.addNativeFunction("fillMemory_u8", lemon::wrap(&fillMemory_u8), defaultFlags)
			.setParameterInfo(0, "startAddress")
			.setParameterInfo(1, "bytes")
			.setParameterInfo(2, "value");

		module.addNativeFunction("fillMemory_u16", lemon::wrap(&fillMemory_u16), defaultFlags)
			.setParameterInfo(0, "startAddress")
			.setParameterInfo(1, "bytes")
			.setParameterInfo(2, "value");

		module.addNativeFunction("fillMemory_u32", lemon::wrap(&fillMemory_u32), defaultFlags)
			.setParameterInfo(0, "startAddress")
			.setParameterInfo(1, "bytes")
			.setParameterInfo(2, "value");


		// Push and pop
		module.addNativeFunction("push", lemon::wrap(&push), defaultFlags);
		module.addNativeFunction("pop", lemon::wrap(&pop), defaultFlags);


		// Persistent data
		module.addNativeFunction("System.loadPersistentData", lemon::wrap(&System_loadPersistentData), defaultFlags)
			.setParameterInfo(0, "targetAddress")
			.setParameterInfo(1, "key")
			.setParameterInfo(2, "bytes");

		module.addNativeFunction("System.savePersistentData", lemon::wrap(&System_savePersistentData), defaultFlags)
			.setParameterInfo(0, "sourceAddress")
			.setParameterInfo(1, "key")
			.setParameterInfo(2, "bytes");


		// SRAM
		module.addNativeFunction("SRAM.load", lemon::wrap(&SRAM_load), defaultFlags)
			.setParameterInfo(0, "address")
			.setParameterInfo(1, "offset")
			.setParameterInfo(2, "bytes");

		module.addNativeFunction("SRAM.save", lemon::wrap(&SRAM_save), defaultFlags)
			.setParameterInfo(0, "address")
			.setParameterInfo(1, "offset")
			.setParameterInfo(2, "bytes");


		// System
		module.addNativeFunction("System.callFunctionByName", lemon::wrap(&System_callFunctionByName))	// Should not get inline executed
			.setParameterInfo(0, "functionName");

		module.addNativeFunction("System.setupCallFrame", lemon::wrap(&System_setupCallFrame1))		// Should not get inline executed
			.setParameterInfo(0, "functionName");

		module.addNativeFunction("System.setupCallFrame", lemon::wrap(&System_setupCallFrame2))		// Should not get inline executed
			.setParameterInfo(0, "functionName")
			.setParameterInfo(1, "labelName");

		module.addNativeFunction("System.getGlobalVariableValueByName", lemon::wrap(&System_getGlobalVariableValueByName), defaultFlags)
			.setParameterInfo(0, "variableName");

		module.addNativeFunction("System.setGlobalVariableValueByName", lemon::wrap(&System_setGlobalVariableValueByName), defaultFlags)
			.setParameterInfo(0, "variableName")
			.setParameterInfo(1, "value");

		module.addNativeFunction("System.rand", lemon::wrap(&System_rand), defaultFlags);

		module.addNativeFunction("System.getPlatformFlags", lemon::wrap(&System_getPlatformFlags), defaultFlags);

		module.addNativeFunction("System.hasPlatformFlag", lemon::wrap(&System_hasPlatformFlag), defaultFlags)
			.setParameterInfo(0, "flag");


		// Access external data
		module.addNativeFunction("System.hasExternalRawData", lemon::wrap(&System_hasExternalRawData), defaultFlags)
			.setParameterInfo(0, "key");

		module.addNativeFunction("System.loadExternalRawData", lemon::wrap(&System_loadExternalRawData1), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "targetAddress")
			.setParameterInfo(2, "offset")
			.setParameterInfo(3, "maxBytes")
			.setParameterInfo(4, "loadOriginalData")
			.setParameterInfo(5, "loadModdedData");

		module.addNativeFunction("System.loadExternalRawData", lemon::wrap(&System_loadExternalRawData2), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "targetAddress");

		module.addNativeFunction("System.hasExternalPaletteData", lemon::wrap(&System_hasExternalPaletteData), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "line");

		module.addNativeFunction("System.loadExternalPaletteData", lemon::wrap(&System_loadExternalPaletteData), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "line")
			.setParameterInfo(2, "targetAddress")
			.setParameterInfo(3, "maxColors");
	}

	// High-level functionality
	{
		// Input
		module.addNativeFunction("Input.getController", lemon::wrap(&Input_getController), defaultFlags)
			.setParameterInfo(0, "controllerIndex");

		module.addNativeFunction("Input.getControllerPrevious", lemon::wrap(&Input_getControllerPrevious), defaultFlags)
			.setParameterInfo(0, "controllerIndex");

		module.addNativeFunction("buttonDown", lemon::wrap(&Input_buttonDown), defaultFlags)			// Deprecated
			.setParameterInfo(0, "index");

		module.addNativeFunction("buttonPressed", lemon::wrap(&Input_buttonPressed), defaultFlags)		// Deprecated
			.setParameterInfo(0, "index");

		module.addNativeFunction("Input.buttonDown", lemon::wrap(&Input_buttonDown), defaultFlags)
			.setParameterInfo(0, "index");

		module.addNativeFunction("Input.buttonPressed", lemon::wrap(&Input_buttonPressed), defaultFlags)
			.setParameterInfo(0, "index");

		module.addNativeFunction("Input.setTouchInputMode", lemon::wrap(&Input_setTouchInputMode), defaultFlags)
			.setParameterInfo(0, "mode");

		module.addNativeFunction("Input.resetControllerRumble", lemon::wrap(&Input_resetControllerRumble), defaultFlags)
			.setParameterInfo(0, "playerIndex");

		module.addNativeFunction("Input.setControllerRumble", lemon::wrap(&Input_setControllerRumble), defaultFlags)
			.setParameterInfo(0, "playerIndex")
			.setParameterInfo(1, "lowFrequencyRumble")
			.setParameterInfo(2, "highFrequencyRumble")
			.setParameterInfo(3, "milliseconds");

		module.addNativeFunction("Input.setControllerLEDs", lemon::wrap(&Input_setControllerLEDs), defaultFlags)
			.setParameterInfo(0, "playerIndex")
			.setParameterInfo(1, "color");

		// Yield
		module.addNativeFunction("yieldExecution", lemon::wrap(&yieldExecution));	// Should not get inline executed
	}

	// Renderer bindings
	RendererBindings::registerBindings(module);

	{
		// Audio
		module.addNativeFunction("Audio.getAudioKeyType", lemon::wrap(&Audio_getAudioKeyType), defaultFlags)
			.setParameterInfo(0, "sfxId");

		module.addNativeFunction("Audio.isPlayingAudio", lemon::wrap(&Audio_isPlayingAudio), defaultFlags)
			.setParameterInfo(0, "sfxId");

		module.addNativeFunction("Audio.playAudio", lemon::wrap(&Audio_playAudio1), defaultFlags)
			.setParameterInfo(0, "sfxId")
			.setParameterInfo(1, "contextId");

		module.addNativeFunction("Audio.playAudio", lemon::wrap(&Audio_playAudio2), defaultFlags)
			.setParameterInfo(0, "sfxId");

		module.addNativeFunction("Audio.stopChannel", lemon::wrap(&Audio_stopChannel), defaultFlags)
			.setParameterInfo(0, "channel");

		module.addNativeFunction("Audio.fadeInChannel", lemon::wrap(&Audio_fadeInChannel), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "seconds");

		module.addNativeFunction("Audio.fadeInChannel", lemon::wrap(&Audio_fadeInChannel2), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "length");

		module.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "seconds");

		module.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel2), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "length");

		module.addNativeFunction("Audio.playOverride", lemon::wrap(&Audio_playOverride), defaultFlags)
			.setParameterInfo(0, "sfxId")
			.setParameterInfo(1, "contextId")
			.setParameterInfo(2, "channelId")
			.setParameterInfo(3, "overriddenChannelId");

		module.addNativeFunction("Audio.enableAudioModifier", lemon::wrap(&Audio_enableAudioModifier), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "context")
			.setParameterInfo(2, "postfix")
			.setParameterInfo(3, "relativeSpeed");

		module.addNativeFunction("Audio.enableAudioModifier", lemon::wrap(&Audio_enableAudioModifier2), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "context")
			.setParameterInfo(2, "postfix")
			.setParameterInfo(3, "relativeSpeed");

		module.addNativeFunction("Audio.disableAudioModifier", lemon::wrap(&Audio_disableAudioModifier), defaultFlags)
			.setParameterInfo(0, "channel")
			.setParameterInfo(1, "context");


		// Misc
		module.addNativeFunction("Mods.isModActive", lemon::wrap(&Mods_isModActive), defaultFlags)
			.setParameterInfo(0, "modName");

		module.addNativeFunction("Mods.getModPriority", lemon::wrap(&Mods_getModPriority), defaultFlags)
			.setParameterInfo(0, "modName");
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

		module.addNativeFunction("debugLog", lemon::wrap(&debugLog), defaultFlags)
			.setParameterInfo(0, "value");

		module.addNativeFunction("debugLogColors", lemon::wrap(&debugLogColors), defaultFlags)
			.setParameterInfo(0, "name")
			.setParameterInfo(1, "startAddress")
			.setParameterInfo(2, "numColors");

	#if 0
		// Only for debugging value stack issues in lemonscript itself
		module.addNativeFunction("debugLogValueStack", lemon::wrap(&debugLogValueStack), defaultFlags);
	#endif


		// Debug keys
		for (int i = 0; i < 10; ++i)
		{
			lemon::UserDefinedVariable& var = module.addUserDefinedVariable("Key" + std::to_string(i), &lemon::PredefinedDataTypes::UINT_8);
			var.mGetter = std::bind(debugKeyGetter, i);
		}


		// Watches
		module.addNativeFunction("debugWatch", lemon::wrap(&debugWatch), defaultFlags)
			.setParameterInfo(0, "address")
			.setParameterInfo(1, "bytes");


		// Dump to file
		module.addNativeFunction("debugDumpToFile", lemon::wrap(&debugDumpToFile), defaultFlags)
			.setParameterInfo(0, "filename")
			.setParameterInfo(1, "startAddress")
			.setParameterInfo(2, "bytes");


		// ROM data analyser
		module.addNativeFunction("ROMDataAnalyser.isEnabled", lemon::wrap(&ROMDataAnalyser_isEnabled), defaultFlags);

		module.addNativeFunction("ROMDataAnalyser.hasEntry", lemon::wrap(&ROMDataAnalyser_hasEntry), defaultFlags)
			.setParameterInfo(0, "category")
			.setParameterInfo(1, "address");

		module.addNativeFunction("ROMDataAnalyser.beginEntry", lemon::wrap(&ROMDataAnalyser_beginEntry), defaultFlags)
			.setParameterInfo(0, "category")
			.setParameterInfo(1, "address");

		module.addNativeFunction("ROMDataAnalyser.endEntry", lemon::wrap(&ROMDataAnalyser_endEntry), defaultFlags);

		module.addNativeFunction("ROMDataAnalyser.addKeyValue", lemon::wrap(&ROMDataAnalyser_addKeyValue), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "value");

		module.addNativeFunction("ROMDataAnalyser.beginObject", lemon::wrap(&ROMDataAnalyser_beginObject), defaultFlags)
			.setParameterInfo(0, "key");

		module.addNativeFunction("ROMDataAnalyser.endObject", lemon::wrap(&ROMDataAnalyser_endObject), defaultFlags);


		// Debug side panel
		module.addNativeFunction("System.SidePanel.setupCustomCategory", lemon::wrap(&System_SidePanel_setupCustomCategory), defaultFlags)
			.setParameterInfo(0, "shortName")
			.setParameterInfo(1, "fullName");

		module.addNativeFunction("System.SidePanel.addOption", lemon::wrap(&System_SidePanel_addOption), defaultFlags)
			.setParameterInfo(0, "text")
			.setParameterInfo(1, "defaultValue");

		module.addNativeFunction("System.SidePanel.addEntry", lemon::wrap(&System_SidePanel_addEntry), defaultFlags)
			.setParameterInfo(0, "key");

		module.addNativeFunction("System.SidePanel.addLine", lemon::wrap(&System_SidePanel_addLine1), defaultFlags)
			.setParameterInfo(0, "text")
			.setParameterInfo(1, "indent")
			.setParameterInfo(2, "color");

		module.addNativeFunction("System.SidePanel.addLine", lemon::wrap(&System_SidePanel_addLine2), defaultFlags)
			.setParameterInfo(0, "text")
			.setParameterInfo(1, "indent");

		module.addNativeFunction("System.SidePanel.isEntryHovered", lemon::wrap(&System_SidePanel_isEntryHovered), defaultFlags)
			.setParameterInfo(0, "key");


		// This is not really debugging-related, as it's meant to be written in non-developer environment as well
		module.addNativeFunction("System.writeDisplayLine", lemon::wrap(&System_writeDisplayLine), defaultFlags)
			.setParameterInfo(0, "text");
	}

	// Register game-specific script bindings
	EngineMain::getDelegate().registerScriptBindings(module);
}

void LemonScriptBindings::setDebugNotificationInterface(DebugNotificationInterface* debugNotificationInterface)
{
	mDebugNotificationInterface = debugNotificationInterface;
}
