/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/LemonScriptBindings.h"
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
#include "oxygen/resources/FontCollection.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/resources/SpriteCache.h"

#include <lemon/program/FunctionWrapper.h>
#include <lemon/program/Module.h>
#include <lemon/runtime/Runtime.h>
#include <lemon/runtime/StandardLibrary.h>

#include <rmxmedia.h>


namespace
{
	namespace detail
	{
		uint32 loadData(uint32 targetAddress, const std::vector<uint8>& data, uint32 offset, uint32 maxBytes)
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

			uint8* dst = EmulatorInterface::instance().getMemoryPointer(targetAddress, true, bytes);
			memcpy(dst, &data[offset], bytes);
			return bytes;
		}
	}


	DebugNotificationInterface* gDebugNotificationInterface = nullptr;

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
		return EmulatorInterface::instance().getFlagZ();
	}

	uint8 checkFlags_negative()
	{
		return EmulatorInterface::instance().getFlagN();
	}

	void setZeroFlagByValue(uint32 value)
	{
		// In contrast to the emulator, we use the zero flag in its original form: it gets set when value is zero
		EmulatorInterface::instance().setFlagZ(value == 0);
	}

	template<typename T>
	void setNegativeFlagByValue(T value)
	{
		const int bits = sizeof(T) * 8;
		EmulatorInterface::instance().setFlagN((value >> (bits - 1)) != 0);
	}


	void copyMemory(uint32 destAddress, uint32 sourceAddress, uint32 bytes)
	{
		uint8* destPointer = EmulatorInterface::instance().getMemoryPointer(destAddress, true, bytes);
		uint8* sourcePointer = EmulatorInterface::instance().getMemoryPointer(sourceAddress, false, bytes);
		memcpy(destPointer, sourcePointer, bytes);
	}

	void zeroMemory(uint32 startAddress, uint32 bytes)
	{
		uint8* pointer = EmulatorInterface::instance().getMemoryPointer(startAddress, true, bytes);
		memset(pointer, 0, bytes);
	}

	void fillMemory_u8(uint32 startAddress, uint32 bytes, uint8 value)
	{
		uint8* pointer = EmulatorInterface::instance().getMemoryPointer(startAddress, true, bytes);
		for (uint32 i = 0; i < bytes; ++i)
		{
			pointer[i] = value;
		}
	}

	void fillMemory_u16(uint32 startAddress, uint32 bytes, uint16 value)
	{
		RMX_CHECK((startAddress & 0x01) == 0, "Odd address not valid", return);
		RMX_CHECK((bytes & 0x01) == 0, "Odd number of bytes not valid", return);

		uint8* pointer = EmulatorInterface::instance().getMemoryPointer(startAddress, true, bytes);

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

		uint8* pointer = EmulatorInterface::instance().getMemoryPointer(startAddress, true, bytes);

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
		uint32& A7 = EmulatorInterface::instance().getRegister(15);
		A7 -= 4;
		EmulatorInterface::instance().writeMemory32(A7, value);
	}

	uint32 pop()
	{
		uint32& A7 = EmulatorInterface::instance().getRegister(15);
		const uint32 result = EmulatorInterface::instance().readMemory32(A7);
		A7 += 4;
		return result;
	}


	uint32 System_loadPersistentData(uint32 targetAddress, lemon::StringRef key, uint32 maxBytes)
	{
		const std::vector<uint8>& data = PersistentData::instance().getData(key.getHash());
		return detail::loadData(targetAddress, data, 0, maxBytes);
	}

	void System_savePersistentData(uint32 sourceAddress, lemon::StringRef key, uint32 bytes)
	{
		const size_t size = (size_t)bytes;
		std::vector<uint8> data;
		data.resize(size);
		const uint8* src = EmulatorInterface::instance().getMemoryPointer(sourceAddress, false, bytes);
		memcpy(&data[0], src, size);

		if (key.isValid())
		{
			PersistentData::instance().setData(key.getString(), data);
		}
	}

	uint32 SRAM_load(uint32 address, uint16 offset, uint16 bytes)
	{
		return (uint32)EmulatorInterface::instance().loadSRAM(address, (size_t)offset, (size_t)bytes);
	}

	void SRAM_save(uint32 address, uint16 offset, uint16 bytes)
	{
		EmulatorInterface::instance().saveSRAM(address, (size_t)offset, (size_t)bytes);
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

		return detail::loadData(targetAddress, rawData->mContent, offset, maxBytes);
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
		uint32* targetPointer = (uint32*)EmulatorInterface::instance().getMemoryPointer(targetAddress, true, (uint32)numColors * 4);
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
		RMX_ASSERT(success, "No active lemon script runtime");

		LogDisplay::ScriptLogSingleEntry& scriptLogSingleEntry = LogDisplay::instance().updateScriptLogValue(*String(0, "%04d", lineNumber), valueString);
		if (gDebugNotificationInterface)
			gDebugNotificationInterface->onLog(scriptLogSingleEntry);

		Application::instance().getSimulation().stopSingleStepContinue();
	}

	void logSetter(int64 value, bool decimal)
	{
		const std::string valueString = decimal ? *String(0, "%d", value) : *String(0, "%08x", value);
		debugLogInternal(valueString);
	}

	void debugLog(lemon::StringRef text)
	{
		if (text.isValid())
		{
			debugLogInternal(text.getString());
		}
	}

	void debugLogColors(lemon::StringRef name, uint32 startAddress, uint8 numColors)
	{
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			if (!name.isValid())
				return;

			CodeExec* codeExec = CodeExec::getActiveInstance();
			RMX_CHECK(nullptr != codeExec, "No running CodeExec instance", return);
			EmulatorInterface& emulatorInterface = codeExec->getEmulatorInterface();

			LogDisplay::ColorLogEntry entry;
			entry.mName = name.getString();
			entry.mColors.reserve(numColors);
			for (uint8 i = 0; i < numColors; ++i)
			{
				const uint16 packedColor = emulatorInterface.readMemory16(startAddress + i * 2);
				entry.mColors.push_back(PaletteManager::unpackColor(packedColor));
			}
			LogDisplay::instance().addColorLogEntry(entry);

			Application::instance().getSimulation().stopSingleStepContinue();
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
		return (controllerIndex < 2) ? ControlsIn::instance().getInputPad((size_t)controllerIndex) : 0;
	}

	uint16 Input_getControllerPrevious(uint8 controllerIndex)
	{
		return (controllerIndex < 2) ? ControlsIn::instance().getPrevInputPad((size_t)controllerIndex) : 0;
	}

	bool getButtonState(int index, bool previousValue = false)
	{
		ControlsIn& controlsIn = ControlsIn::instance();
		const int playerIndex = (index & 0x10) ? 1 : 0;
		const uint16 bitmask = previousValue ? controlsIn.getPrevInputPad(playerIndex) : controlsIn.getInputPad(playerIndex);
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

	void Input_setTouchInputMode(uint8 index)
	{
		return InputManager::instance().setTouchInputMode((InputManager::TouchInputMode)index);
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

	uint16 getScreenWidth()
	{
		return (uint16)VideoOut::instance().getScreenWidth();
	}

	uint16 getScreenHeight()
	{
		return (uint16)VideoOut::instance().getScreenHeight();
	}

	uint16 getScreenExtend()
	{
		return (uint16)(VideoOut::instance().getScreenWidth() - 320) / 2;
	}


	enum class WriteTarget
	{
		VRAM,
		VSRAM,
		CRAM
	};
	WriteTarget mWriteTarget = WriteTarget::VRAM;
	uint16 mWriteAddress = 0;
	uint16 mWriteIncrement = 2;

	void VDP_setupVRAMWrite(uint16 vramAddress)
	{
		mWriteTarget = WriteTarget::VRAM;
		mWriteAddress = vramAddress;
	}

	void VDP_setupVSRAMWrite(uint16 vsramAddress)
	{
		mWriteTarget = WriteTarget::VSRAM;
		mWriteAddress = vsramAddress;
	}

	void VDP_setupCRAMWrite(uint16 cramAddress)
	{
		mWriteTarget = WriteTarget::CRAM;
		mWriteAddress = cramAddress;
	}

	void VDP_setWriteIncrement(uint16 increment)
	{
		mWriteIncrement = increment;
	}

	uint16 VDP_readData16()
	{
		uint16 result;
		switch (mWriteTarget)
		{
			case WriteTarget::VRAM:
			{
				result = EmulatorInterface::instance().readVRam16(mWriteAddress);
				break;
			}

			case WriteTarget::VSRAM:
			{
				const uint8 index = (mWriteAddress / 2) & 0x3f;
				result = EmulatorInterface::instance().getVSRam()[index];
				break;
			}

			default:
			{
				RMX_ERROR("Not supported", );
				return 0;
			}
		}
		mWriteAddress += mWriteIncrement;
		return result;
	}

	uint32 VDP_readData32()
	{
		const uint16 hi = VDP_readData16();
		const uint16 lo = VDP_readData16();
		return ((uint32)hi << 16) | lo;
	}

	void VDP_writeData16(uint16 value)
	{
		switch (mWriteTarget)
		{
			case WriteTarget::VRAM:
			{
				if (nullptr != gDebugNotificationInterface)
					gDebugNotificationInterface->onVRAMWrite(mWriteAddress, 2);

				EmulatorInterface::instance().writeVRam16(mWriteAddress, value);
				break;
			}

			case WriteTarget::VSRAM:
			{
				const uint8 index = (mWriteAddress / 2) & 0x3f;
				EmulatorInterface::instance().getVSRam()[index] = value;
				break;
			}

			case WriteTarget::CRAM:
			{
				RenderParts::instance().getPaletteManager().writePaletteEntryPacked(0, mWriteAddress / 2, value);
				break;
			}
		}
		mWriteAddress += mWriteIncrement;
	}

	void VDP_writeData32(uint32 value)
	{
		VDP_writeData16((uint16)(value >> 16));
		VDP_writeData16((uint16)value);
	}

	void VDP_copyToVRAM(uint32 address, uint16 bytes)
	{
		RMX_CHECK((bytes & 1) == 0, "Number of bytes in VDP_copyToVRAM must be divisible by two, but is " << bytes, bytes &= 0xfffe);
		RMX_CHECK(uint32(mWriteAddress) + bytes <= 0x10000, "Invalid VRAM access from " << rmx::hexString(mWriteAddress, 8) << " to " << rmx::hexString(mWriteAddress+bytes-1, 8) << " in VDP_copyToVRAM", return);

		EmulatorInterface& emulatorInterface = EmulatorInterface::instance();
		if (mWriteIncrement == 2)
		{
			// Optimized version of the code below
			if (nullptr != gDebugNotificationInterface)
			{
				gDebugNotificationInterface->onVRAMWrite(mWriteAddress, bytes);
			}

			emulatorInterface.copyFromMemoryToVRam(mWriteAddress, address, bytes);
			mWriteAddress += bytes;
		}
		else
		{
			if (nullptr != gDebugNotificationInterface)
			{
				for (uint16 i = 0; i < bytes; i += 2)
					gDebugNotificationInterface->onVRAMWrite(mWriteAddress + mWriteIncrement * i/2, 2);
			}

			for (uint16 i = 0; i < bytes; i += 2)
			{
				const uint16 value = emulatorInterface.readMemory16(address);
				EmulatorInterface::instance().writeVRam16(mWriteAddress, value);
				mWriteAddress += mWriteIncrement;
				address += 2;
			}
		}
	}

	void VDP_fillVRAMbyDMA(uint16 fillValue, uint16 vramAddress, uint16 bytes)
	{
		RMX_CHECK(uint32(vramAddress) + bytes <= 0x10000, "Invalid VRAM access from " << rmx::hexString(vramAddress, 8) << " to " << rmx::hexString(uint32(vramAddress)+bytes-1, 8) << " in VDP_fillVRAMbyDMA", return);

		if (nullptr != gDebugNotificationInterface)
			gDebugNotificationInterface->onVRAMWrite(vramAddress, bytes);

		EmulatorInterface::instance().fillVRam(vramAddress, fillValue, bytes);
		mWriteAddress = vramAddress + bytes;
	}

	void VDP_zeroVRAM(uint16 bytes)
	{
		if (nullptr != gDebugNotificationInterface)
			gDebugNotificationInterface->onVRAMWrite(mWriteAddress, bytes);

		VDP_fillVRAMbyDMA(0, mWriteAddress, bytes);
	}

	void VDP_copyToCRAM(uint32 address, uint16 bytes)
	{
		RMX_ASSERT(mWriteAddress < 0x80 && mWriteAddress + bytes <= 0x80, "Invalid write access to CRAM");
		RMX_ASSERT((mWriteAddress % 2) == 0, "Invalid CRAM write address " << mWriteAddress);
		RMX_ASSERT((mWriteIncrement % 2) == 0, "Invalid CRAM write increment " << mWriteIncrement);

		PaletteManager& paletteManager = RenderParts::instance().getPaletteManager();
		for (uint16 i = 0; i < bytes; i += 2)
		{
			const uint16 colorValue = EmulatorInterface::instance().readMemory16(address + i);
			paletteManager.writePaletteEntryPacked(0, mWriteAddress / 2, colorValue);
			mWriteAddress += mWriteIncrement;
		}
	}

	void VDP_copyToVRAMbyDMA(uint32 sourceAddress, uint16 vramAddress, uint16 bytes)
	{
		VDP_setupVRAMWrite(vramAddress);
		VDP_copyToVRAM(sourceAddress, bytes);
	}

	void VDP_copyToCRAMbyDMA(uint32 sourceAddress, uint16 vramAddress, uint16 bytes)
	{
		VDP_setupCRAMWrite(vramAddress);
		VDP_copyToCRAM(sourceAddress, bytes);
	}


	void VDP_Config_setActiveDisplay(uint8 enable)
	{
		RenderParts::instance().setActiveDisplay(enable != 0);
	}

	void VDP_Config_setNameTableBasePlaneB(uint16 vramAddress)
	{
		RenderParts::instance().getPlaneManager().setNameTableBaseB(vramAddress);
	}

	void VDP_Config_setNameTableBasePlaneA(uint16 vramAddress)
	{
		RenderParts::instance().getPlaneManager().setNameTableBaseA(vramAddress);
	}

	void VDP_Config_setNameTableBasePlaneW(uint16 vramAddress)
	{
		RenderParts::instance().getPlaneManager().setNameTableBaseW(vramAddress);
	}

	void VDP_Config_setVerticalScrolling(uint8 verticalScrolling, uint8 horizontalScrollMask)
	{
		RenderParts::instance().getScrollOffsetsManager().setVerticalScrolling(verticalScrolling != 0);
		RenderParts::instance().getScrollOffsetsManager().setHorizontalScrollMask(horizontalScrollMask);
	}

	void VDP_Config_setBackdropColor(uint8 paletteIndex)
	{
		RenderParts::instance().getPaletteManager().setBackdropColorIndex(paletteIndex);
	}

	void VDP_Config_setRenderingModeConfiguration(uint8 shadowHighlightPalette)
	{
		// TODO: Implement this
	}

	void VDP_Config_setHorizontalScrollTableBase(uint16 vramAddress)
	{
		RenderParts::instance().getScrollOffsetsManager().setHorizontalScrollTableBase(vramAddress);
	}

	void VDP_Config_setPlayfieldSizeInPatterns(uint16 width, uint16 height)
	{
		RenderParts::instance().getPlaneManager().setPlayfieldSizeInPatterns(Vec2i(width, height));
	}

	void VDP_Config_setPlayfieldSizeInPixels(uint16 width, uint16 height)
	{
		RenderParts::instance().getPlaneManager().setPlayfieldSizeInPixels(Vec2i(width, height));
	}

	void VDP_Config_setupWindowPlane(uint8 useWindowPlane, uint16 splitY)
	{
		RenderParts::instance().getPlaneManager().setupPlaneW(useWindowPlane != 0, splitY);
		RenderParts::instance().getScrollOffsetsManager().setPlaneWScrollOffset(Vec2i(0, 0));	// Reset scroll offset to default
	}

	void VDP_Config_setPlaneWScrollOffset(uint16 x, uint8 y)
	{
		RenderParts::instance().getScrollOffsetsManager().setPlaneWScrollOffset(Vec2i(x, y));
	}

	void VDP_Config_setSpriteAttributeTableBase(uint16 vramAddress)
	{
		RenderParts::instance().getSpriteManager().setSpriteAttributeTableBase(vramAddress);
	}


	uint16 getVRAM(uint16 vramAddress)
	{
		return EmulatorInterface::instance().readVRam16(vramAddress);
	}

	void setVRAM(uint16 vramAddress, uint16 value)
	{
		EmulatorInterface::instance().writeVRam16(vramAddress, value);
	}


	void Renderer_setPaletteEntry(uint8 index, uint32 color)
	{
		RenderParts::instance().getPaletteManager().writePaletteEntry(0, index, color);
	}

	void Renderer_setPaletteEntryPacked(uint8 index, uint16 color)
	{
		RenderParts::instance().getPaletteManager().writePaletteEntryPacked(0, index, color);
	}

	void Renderer_enableSecondaryPalette(uint8 line)
	{
		RenderParts::instance().getPaletteManager().setPaletteSplitPositionY(line);
	}

	void Renderer_setSecondaryPaletteEntryPacked(uint8 index, uint16 color)
	{
		RenderParts::instance().getPaletteManager().writePaletteEntryPacked(1, index, color);
	}

	void Renderer_setScrollOffsetH(uint8 setIndex, uint16 lineNumber, uint16 value)
	{
		RenderParts::instance().getScrollOffsetsManager().overwriteScrollOffsetH(setIndex, lineNumber, value);
	}

	void Renderer_setScrollOffsetV(uint8 setIndex, uint16 rowNumber, uint16 value)
	{
		RenderParts::instance().getScrollOffsetsManager().overwriteScrollOffsetV(setIndex, rowNumber, value);
	}

	void Renderer_setHorizontalScrollNoRepeat(uint8 setIndex, uint8 enable)
	{
		RenderParts::instance().getScrollOffsetsManager().setHorizontalScrollNoRepeat(setIndex, enable != 0);
	}

	void Renderer_setVerticalScrollOffsetBias(int16 bias)
	{
		RenderParts::instance().getScrollOffsetsManager().setVerticalScrollOffsetBias(bias);
	}

	void Renderer_enforceClearScreen(uint8 enabled)
	{
		RenderParts::instance().setEnforceClearScreen(enabled != 0);
	}

	void Renderer_enableDefaultPlane(uint8 planeIndex, uint8 enabled)
	{
		RenderParts::instance().getPlaneManager().setDefaultPlaneEnabled(planeIndex, enabled != 0);
	}

	void Renderer_setupPlane(int16 px, int16 py, int16 width, int16 height, uint8 planeIndex, uint8 scrollOffsets, uint16 renderQueue)
	{
		RenderParts::instance().getPlaneManager().setupCustomPlane(Recti(px, py, width, height), planeIndex, scrollOffsets, renderQueue);
	}

	void Renderer_resetCustomPlaneConfigurations()
	{
		RenderParts::instance().getPlaneManager().resetCustomPlanes();
	}

	void Renderer_resetSprites()
	{
		RenderParts::instance().getSpriteManager().resetSprites();
	}

	void Renderer_drawVdpSprite(int16 px, int16 py, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue)
	{
		RenderParts::instance().getSpriteManager().drawVdpSprite(Vec2i(px, py), encodedSize, patternIndex, renderQueue);
	}

	void Renderer_drawVdpSpriteWithAlpha(int16 px, int16 py, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, uint8 alpha)
	{
		RenderParts::instance().getSpriteManager().drawVdpSprite(Vec2i(px, py), encodedSize, patternIndex, renderQueue, Color(1.0f, 1.0f, 1.0f, (float)alpha / 255.0f));
	}

	void Renderer_drawVdpSpriteTinted(int16 px, int16 py, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, uint32 tintColor, uint32 addedColor)
	{
		RenderParts::instance().getSpriteManager().drawVdpSprite(Vec2i(px, py), encodedSize, patternIndex, renderQueue, Color::fromRGBA32(tintColor), Color::fromRGBA32(addedColor));
	}

	bool Renderer_hasCustomSprite(uint64 key)
	{
		return SpriteCache::instance().hasSprite(key);
	}

	uint64 Renderer_setupCustomUncompressedSprite(uint32 sourceBase, uint16 words, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(sourceBase, words / 0x10, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_NONE);
	}

	uint64 Renderer_setupCustomCharacterSprite(uint32 sourceBase, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(sourceBase, tableAddress, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_CHARACTER);
	}

	uint64 Renderer_setupCustomObjectSprite(uint32 sourceBase, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(sourceBase, tableAddress, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_OBJECT);
	}

	uint64 Renderer_setupKosinskiCompressedSprite1(uint32 sourceAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(sourceAddress, 0, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_KOSINSKI);
	}

	uint64 Renderer_setupKosinskiCompressedSprite2(uint32 sourceAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex, int16 indexOffset)
	{
		return SpriteCache::instance().setupSpriteFromROM(sourceAddress, 0, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_KOSINSKI, indexOffset);
	}

	void Renderer_drawCustomSprite1(uint64 key, int16 px, int16 py, uint8 atex, uint8 flags, uint16 renderQueue)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue);
	}

	void Renderer_drawCustomSprite2(uint64 key, int16 px, int16 py, uint8 atex, uint8 flags, uint16 renderQueue, uint8 angle, uint8 alpha)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color(1.0f, 1.0f, 1.0f, (float)alpha / 255.0f), (float)angle / 128.0f * PI_FLOAT);
	}

	void Renderer_drawCustomSpriteTinted(uint64 key, int16 px, int16 py, uint8 atex, uint8 flags, uint16 renderQueue, uint8 angle, uint32 tintColor, int32 scale)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), (float)angle / 128.0f * PI_FLOAT, (float)scale / 65536.0f);
	}

	void Renderer_drawCustomSpriteTransformed(uint64 key, int16 px, int16 py, uint8 atex, uint8 flags, uint16 renderQueue, uint32 tintColor, int32 transform11, int32 transform12, int32 transform21, int32 transform22)
	{
		Transform2D transformation;
		transformation.setByMatrix((float)transform11 / 65536.0f, (float)transform12 / 65536.0f, (float)transform21 / 65536.0f, (float)transform22 / 65536.0f);
		RenderParts::instance().getSpriteManager().drawCustomSpriteWithTransform(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), transformation);
	}

	void Renderer_extractCustomSprite(uint64 key, lemon::StringRef categoryName, uint8 spriteNumber, uint8 atex)
	{
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			if (categoryName.isValid())
			{
				SpriteCache::instance().dumpSprite(key, categoryName.getString(), spriteNumber, atex);
			}
		}
	}

	void Renderer_addSpriteMask(int16 px, int16 py, int16 width, int16 height, uint16 renderQueue, uint8 priorityFlag)
	{
		RenderParts::instance().getSpriteManager().addSpriteMask(Vec2i(px, py), Vec2i(width, height), renderQueue, priorityFlag != 0, SpriteManager::Space::SCREEN);
	}

	void Renderer_addSpriteMaskWorld(int16 px, int16 py, int16 width, int16 height, uint16 renderQueue, uint8 priorityFlag)
	{
		RenderParts::instance().getSpriteManager().addSpriteMask(Vec2i(px, py), Vec2i(width, height), renderQueue, priorityFlag != 0, SpriteManager::Space::WORLD);
	}

	void Renderer_setLogicalSpriteSpace(uint8 space)
	{
		RMX_CHECK(space < 2, "Invalid space index " << space, return);
		RenderParts::instance().getSpriteManager().setLogicalSpriteSpace((SpriteManager::Space)space);
	}

	void Renderer_clearSpriteTag()
	{
		RenderParts::instance().getSpriteManager().clearSpriteTag();
	}

	void Renderer_setSpriteTagWithPosition(uint64 spriteTag, uint16 px, uint16 py)
	{
		RenderParts::instance().getSpriteManager().setSpriteTagWithPosition(spriteTag, Vec2i(px, py));
	}

	void Renderer_resetViewport(uint16 renderQueue)
	{
		RenderParts::instance().addViewport(Recti(0, 0, VideoOut::instance().getScreenWidth(), VideoOut::instance().getScreenHeight()), renderQueue);
	}

	void Renderer_setViewport(int16 px, int16 py, int16 width, int16 height, uint16 renderQueue)
	{
		RenderParts::instance().addViewport(Recti(px, py, width, height), renderQueue);
	}

	void Renderer_setGlobalComponentTint(int16 tintR, int16 tintG, int16 tintB, int16 addedR, int16 addedG, int16 addedB)
	{
		const Color tintColor((float)tintR / 255.0f, (float)tintG / 255.0f, (float)tintB / 255.0f, 1.0f);
		const Color addedColor((float)addedR / 255.0f, (float)addedG / 255.0f, (float)addedB / 255.0f, 0.0f);
		RenderParts::instance().getPaletteManager().setGlobalComponentTint(tintColor, addedColor);
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
						String str = textString;
						str.lowerCase();
						sfxId = rmx::getMurmur2_64(str);
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

	void Audio_fadeInChannel(uint8 channel, uint16 length)
	{
		EngineMain::instance().getAudioOut().fadeInChannel(channel, (float)length / 256.0f);
	}

	void Audio_fadeOutChannel(uint8 channel, uint16 length)
	{
		EngineMain::instance().getAudioOut().fadeOutChannel(channel, (float)length / 256.0f);
	}

	void Audio_playOverride(uint64 sfxId, uint8 contextId, uint8 channelId, uint8 overriddenChannelId)
	{
		EngineMain::instance().getAudioOut().playOverride(sfxId, contextId, channelId, overriddenChannelId);
	}

	void Audio_enableAudioModifier(uint8 channel, uint8 context, lemon::StringRef postfix, uint32 relativeSpeed)
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


	void setWorldSpaceOffset(int32 px, int32 py)
	{
		// Note that this is needed for world space sprite masking, not only debug drawing
		RenderParts::instance().getSpacesManager().setWorldSpaceOffset(Vec2i(px, py));
	}

	void debugDrawRect(int32 px, int32 py, int32 width, int32 height)
	{
		RenderParts::instance().getOverlayManager().addDebugDrawRect(Recti(px, py, width, height));
	}

	void debugDrawRect2(int32 px, int32 py, int32 width, int32 height, uint32 color)
	{
		RenderParts::instance().getOverlayManager().addDebugDrawRect(Recti(px, py, width, height), Color::fromRGBA32(color));
	}

	void Renderer_drawText(lemon::StringRef fontKey, int32 px, int32 py, lemon::StringRef text, uint32 tintColor, uint8 alignment, int8 spacing, uint16 renderQueue, bool useWorldSpace)
	{
		RMX_CHECK(alignment >= 1 && alignment <= 9, "Invalid alignment " << alignment << " used for drawing text, fallback to alignment = 1", alignment = 1);
		if (fontKey.isValid() && text.isValid())
		{
			RenderParts::instance().getOverlayManager().addText(fontKey.getString(), fontKey.getHash(), Vec2i(px, py), text.getString(), text.getHash(), Color::fromRGBA32(tintColor), (int)alignment, (int)spacing, renderQueue, useWorldSpace ? OverlayManager::Space::WORLD : OverlayManager::Space::SCREEN);
		}
	}

	int32 Renderer_getTextWidth(lemon::StringRef fontKey, lemon::StringRef text)
	{
		if (fontKey.isValid() && text.isValid())
		{
			Font* font = FontCollection::instance().getFontByKey(fontKey.getHash());
			if (nullptr != font)
			{
				return font->getWidth(text.getString());
			}
		}
		return 0;
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
			codeExec->addWatch(address, bytes, false);
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
	// Standard library
	lemon::StandardLibrary::registerBindings(module);

	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);
	module.addNativeFunction("assert", lemon::wrap(&scriptAssert1), defaultFlags);
	module.addNativeFunction("assert", lemon::wrap(&scriptAssert2), defaultFlags);

	// Emulator interface bindings
	{
		EmulatorInterface& emulatorInterface = EmulatorInterface::instance();

		// Register access
		const std::string registerNamesDAR[16] = { "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7" };
		for (size_t i = 0; i < 16; ++i)
		{
			lemon::ExternalVariable& var = module.addExternalVariable(registerNamesDAR[i], &lemon::PredefinedDataTypes::UINT_32);
			var.mPointer = &emulatorInterface.getRegister(i);

			lemon::ExternalVariable& var_u8 = module.addExternalVariable(registerNamesDAR[i] + ".u8", &lemon::PredefinedDataTypes::UINT_8);
			var_u8.mPointer = &emulatorInterface.getRegister(i);

			lemon::ExternalVariable& var_s8 = module.addExternalVariable(registerNamesDAR[i] + ".s8", &lemon::PredefinedDataTypes::INT_8);
			var_s8.mPointer = &emulatorInterface.getRegister(i);

			lemon::ExternalVariable& var_u16 = module.addExternalVariable(registerNamesDAR[i] + ".u16", &lemon::PredefinedDataTypes::UINT_16);
			var_u16.mPointer = &emulatorInterface.getRegister(i);

			lemon::ExternalVariable& var_s16 = module.addExternalVariable(registerNamesDAR[i] + ".s16", &lemon::PredefinedDataTypes::INT_16);
			var_s16.mPointer = &emulatorInterface.getRegister(i);

			lemon::ExternalVariable& var_u32 = module.addExternalVariable(registerNamesDAR[i] + ".u32", &lemon::PredefinedDataTypes::UINT_32);
			var_u32.mPointer = &emulatorInterface.getRegister(i);

			lemon::ExternalVariable& var_s32 = module.addExternalVariable(registerNamesDAR[i] + ".s32", &lemon::PredefinedDataTypes::INT_32);
			var_s32.mPointer = &emulatorInterface.getRegister(i);
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
			.setParameterInfo(0, "index");

		module.addNativeFunction("Input.setControllerLEDs", lemon::wrap(&Input_setControllerLEDs), defaultFlags)
			.setParameterInfo(0, "playerIndex")
			.setParameterInfo(1, "color");

		// Yield
		module.addNativeFunction("yieldExecution", lemon::wrap(&yieldExecution));	// Should not get inline executed

		// Screen size query
		module.addNativeFunction("getScreenWidth", lemon::wrap(&getScreenWidth), defaultFlags);
		module.addNativeFunction("getScreenHeight", lemon::wrap(&getScreenHeight), defaultFlags);
		module.addNativeFunction("getScreenExtend", lemon::wrap(&getScreenExtend), defaultFlags);


		// VDP emulation
		module.addNativeFunction("VDP.setupVRAMWrite", lemon::wrap(&VDP_setupVRAMWrite), defaultFlags)
			.setParameterInfo(0, "vramAddress");

		module.addNativeFunction("VDP.setupVSRAMWrite", lemon::wrap(&VDP_setupVSRAMWrite), defaultFlags)
			.setParameterInfo(0, "vsramAddress");

		module.addNativeFunction("VDP.setupCRAMWrite", lemon::wrap(&VDP_setupCRAMWrite), defaultFlags)
			.setParameterInfo(0, "cramAddress");

		module.addNativeFunction("VDP.setWriteIncrement", lemon::wrap(&VDP_setWriteIncrement), defaultFlags)
			.setParameterInfo(0, "increment");

		module.addNativeFunction("VDP.readData16", lemon::wrap(&VDP_readData16), defaultFlags);

		module.addNativeFunction("VDP.readData32", lemon::wrap(&VDP_readData32), defaultFlags);

		module.addNativeFunction("VDP.writeData16", lemon::wrap(&VDP_writeData16), defaultFlags)
			.setParameterInfo(0, "value");

		module.addNativeFunction("VDP.writeData32", lemon::wrap(&VDP_writeData32), defaultFlags)
			.setParameterInfo(0, "value");

		module.addNativeFunction("VDP.copyToVRAM", lemon::wrap(&VDP_copyToVRAM), defaultFlags)
			.setParameterInfo(0, "address")
			.setParameterInfo(1, "bytes");

		module.addNativeFunction("VDP.fillVRAMbyDMA", lemon::wrap(&VDP_fillVRAMbyDMA), defaultFlags)
			.setParameterInfo(0, "fillValue")
			.setParameterInfo(1, "vramAddress")
			.setParameterInfo(2, "bytes");

		module.addNativeFunction("VDP.zeroVRAM", lemon::wrap(&VDP_zeroVRAM), defaultFlags)
			.setParameterInfo(0, "bytes");

		module.addNativeFunction("VDP.copyToVRAMbyDMA", lemon::wrap(&VDP_copyToVRAMbyDMA), defaultFlags)
			.setParameterInfo(0, "sourceAddress")
			.setParameterInfo(1, "vramAddress")
			.setParameterInfo(2, "bytes");

		module.addNativeFunction("VDP.copyToCRAMbyDMA", lemon::wrap(&VDP_copyToCRAMbyDMA), defaultFlags)
			.setParameterInfo(0, "sourceAddress")
			.setParameterInfo(1, "vramAddress")
			.setParameterInfo(2, "bytes");


		// VDP config
		module.addNativeFunction("VDP.Config.setActiveDisplay", lemon::wrap(&VDP_Config_setActiveDisplay), defaultFlags)
			.setParameterInfo(0, "enable");

		module.addNativeFunction("VDP.Config.setNameTableBasePlaneB", lemon::wrap(&VDP_Config_setNameTableBasePlaneB), defaultFlags)
			.setParameterInfo(0, "vramAddress");

		module.addNativeFunction("VDP.Config.setNameTableBasePlaneA", lemon::wrap(&VDP_Config_setNameTableBasePlaneA), defaultFlags)
			.setParameterInfo(0, "vramAddress");

		module.addNativeFunction("VDP.Config.setNameTableBasePlaneW", lemon::wrap(&VDP_Config_setNameTableBasePlaneW), defaultFlags)
			.setParameterInfo(0, "vramAddress");

		module.addNativeFunction("VDP.Config.setBackdropColor", lemon::wrap(&VDP_Config_setBackdropColor), defaultFlags)
			.setParameterInfo(0, "paletteIndex");

		module.addNativeFunction("VDP.Config.setVerticalScrolling", lemon::wrap(&VDP_Config_setVerticalScrolling), defaultFlags)
			.setParameterInfo(0, "verticalScrolling")
			.setParameterInfo(1, "horizontalScrollMask");

		module.addNativeFunction("VDP.Config.setRenderingModeConfiguration", lemon::wrap(&VDP_Config_setRenderingModeConfiguration), defaultFlags)
			.setParameterInfo(0, "shadowHighlightPalette");

		module.addNativeFunction("VDP.Config.setHorizontalScrollTableBase", lemon::wrap(&VDP_Config_setHorizontalScrollTableBase), defaultFlags)
			.setParameterInfo(0, "vramAddress");

		module.addNativeFunction("VDP.Config.setPlayfieldSizeInPatterns", lemon::wrap(&VDP_Config_setPlayfieldSizeInPatterns), defaultFlags)
			.setParameterInfo(0, "width")
			.setParameterInfo(1, "height");

		module.addNativeFunction("VDP.Config.setPlayfieldSizeInPixels", lemon::wrap(&VDP_Config_setPlayfieldSizeInPixels), defaultFlags)
			.setParameterInfo(0, "width")
			.setParameterInfo(1, "height");

		module.addNativeFunction("VDP.Config.setupWindowPlane", lemon::wrap(&VDP_Config_setupWindowPlane), defaultFlags)
			.setParameterInfo(0, "useWindowPlane")
			.setParameterInfo(1, "splitY");

		module.addNativeFunction("VDP.Config.setPlaneWScrollOffset", lemon::wrap(&VDP_Config_setPlaneWScrollOffset), defaultFlags)
			.setParameterInfo(0, "x")
			.setParameterInfo(1, "y");

		module.addNativeFunction("VDP.Config.setSpriteAttributeTableBase", lemon::wrap(&VDP_Config_setSpriteAttributeTableBase), defaultFlags)
			.setParameterInfo(0, "vramAddress");


		// Direct VRAM access
		module.addNativeFunction("getVRAM", lemon::wrap(&getVRAM), defaultFlags)
			.setParameterInfo(0, "vramAddress");

		module.addNativeFunction("setVRAM", lemon::wrap(&setVRAM), defaultFlags)
			.setParameterInfo(0, "vramAddress")
			.setParameterInfo(1, "value");


		// Special renderer functionality
		module.addNativeFunction("Renderer.setPaletteEntry", lemon::wrap(&Renderer_setPaletteEntry), defaultFlags)
			.setParameterInfo(0, "index")
			.setParameterInfo(1, "color");

		module.addNativeFunction("Renderer.setPaletteEntryPacked", lemon::wrap(&Renderer_setPaletteEntryPacked), defaultFlags)
			.setParameterInfo(0, "index")
			.setParameterInfo(1, "color");

		module.addNativeFunction("Renderer.enableSecondaryPalette", lemon::wrap(&Renderer_enableSecondaryPalette), defaultFlags)
			.setParameterInfo(0, "line");

		module.addNativeFunction("Renderer.setSecondaryPaletteEntryPacked", lemon::wrap(&Renderer_setSecondaryPaletteEntryPacked), defaultFlags)
			.setParameterInfo(0, "index")
			.setParameterInfo(1, "color");

		module.addNativeFunction("Renderer.setScrollOffsetH", lemon::wrap(&Renderer_setScrollOffsetH), defaultFlags)
			.setParameterInfo(0, "setIndex")
			.setParameterInfo(1, "lineNumber")
			.setParameterInfo(2, "value");

		module.addNativeFunction("Renderer.setScrollOffsetV", lemon::wrap(&Renderer_setScrollOffsetV), defaultFlags)
			.setParameterInfo(0, "setIndex")
			.setParameterInfo(1, "rowNumber")
			.setParameterInfo(2, "value");

		module.addNativeFunction("Renderer.setHorizontalScrollNoRepeat", lemon::wrap(&Renderer_setHorizontalScrollNoRepeat), defaultFlags)
			.setParameterInfo(0, "setIndex")
			.setParameterInfo(1, "enable");

		module.addNativeFunction("Renderer.setVerticalScrollOffsetBias", lemon::wrap(&Renderer_setVerticalScrollOffsetBias), defaultFlags)
			.setParameterInfo(0, "bias");

		module.addNativeFunction("Renderer.enforceClearScreen", lemon::wrap(&Renderer_enforceClearScreen), defaultFlags)
			.setParameterInfo(0, "enabled");

		module.addNativeFunction("Renderer.enableDefaultPlane", lemon::wrap(&Renderer_enableDefaultPlane), defaultFlags)
			.setParameterInfo(0, "planeIndex")
			.setParameterInfo(1, "enabled");

		module.addNativeFunction("Renderer.setupPlane", lemon::wrap(&Renderer_setupPlane), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height")
			.setParameterInfo(4, "planeIndex")
			.setParameterInfo(5, "scrollOffsets")
			.setParameterInfo(6, "renderQueue");

		module.addNativeFunction("Renderer.resetCustomPlaneConfigurations", lemon::wrap(&Renderer_resetCustomPlaneConfigurations), defaultFlags);

		module.addNativeFunction("Renderer.resetSprites", lemon::wrap(&Renderer_resetSprites), defaultFlags);

		module.addNativeFunction("Renderer.drawVdpSprite", lemon::wrap(&Renderer_drawVdpSprite), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "encodedSize")
			.setParameterInfo(3, "patternIndex")
			.setParameterInfo(4, "renderQueue");

		module.addNativeFunction("Renderer.drawVdpSpriteWithAlpha", lemon::wrap(&Renderer_drawVdpSpriteWithAlpha), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "encodedSize")
			.setParameterInfo(3, "patternIndex")
			.setParameterInfo(4, "renderQueue")
			.setParameterInfo(5, "alpha");

		module.addNativeFunction("Renderer.drawVdpSpriteTinted", lemon::wrap(&Renderer_drawVdpSpriteTinted), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "encodedSize")
			.setParameterInfo(3, "patternIndex")
			.setParameterInfo(4, "renderQueue")
			.setParameterInfo(5, "tintColor")
			.setParameterInfo(6, "addedColor");

		module.addNativeFunction("Renderer.hasCustomSprite", lemon::wrap(&Renderer_hasCustomSprite), defaultFlags)
			.setParameterInfo(0, "key");

		module.addNativeFunction("Renderer.setupCustomUncompressedSprite", lemon::wrap(&Renderer_setupCustomUncompressedSprite), defaultFlags)
			.setParameterInfo(0, "sourceBase")
			.setParameterInfo(1, "word")
			.setParameterInfo(2, "mappingOffset")
			.setParameterInfo(3, "animationSprite")
			.setParameterInfo(4, "atex");

		module.addNativeFunction("Renderer.setupCustomCharacterSprite", lemon::wrap(&Renderer_setupCustomCharacterSprite), defaultFlags)
			.setParameterInfo(0, "sourceBase")
			.setParameterInfo(1, "tableAddress")
			.setParameterInfo(2, "mappingOffset")
			.setParameterInfo(3, "animationSprite")
			.setParameterInfo(4, "atex");

		module.addNativeFunction("Renderer.setupCustomObjectSprite", lemon::wrap(&Renderer_setupCustomObjectSprite), defaultFlags)
			.setParameterInfo(0, "sourceBase")
			.setParameterInfo(1, "tableAddress")
			.setParameterInfo(2, "mappingOffset")
			.setParameterInfo(3, "animationSprite")
			.setParameterInfo(4, "atex");

		module.addNativeFunction("Renderer.setupKosinskiCompressedSprite", lemon::wrap(&Renderer_setupKosinskiCompressedSprite1), defaultFlags)
			.setParameterInfo(0, "sourceBase")
			.setParameterInfo(1, "mappingOffset")
			.setParameterInfo(2, "animationSprite")
			.setParameterInfo(3, "atex");

		module.addNativeFunction("Renderer.setupKosinskiCompressedSprite", lemon::wrap(&Renderer_setupKosinskiCompressedSprite2), defaultFlags)
			.setParameterInfo(0, "sourceBase")
			.setParameterInfo(1, "mappingOffset")
			.setParameterInfo(2, "animationSprite")
			.setParameterInfo(3, "atex")
			.setParameterInfo(4, "indexOffset");

		module.addNativeFunction("Renderer.drawCustomSprite", lemon::wrap(&Renderer_drawCustomSprite1), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "px")
			.setParameterInfo(2, "py")
			.setParameterInfo(3, "atex")
			.setParameterInfo(4, "flags")
			.setParameterInfo(5, "renderQueue");

		module.addNativeFunction("Renderer.drawCustomSprite", lemon::wrap(&Renderer_drawCustomSprite2), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "px")
			.setParameterInfo(2, "py")
			.setParameterInfo(3, "atex")
			.setParameterInfo(4, "flags")
			.setParameterInfo(5, "renderQueue")
			.setParameterInfo(6, "angle")
			.setParameterInfo(7, "alpha");

		module.addNativeFunction("Renderer.drawCustomSpriteTinted", lemon::wrap(&Renderer_drawCustomSpriteTinted), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "px")
			.setParameterInfo(2, "py")
			.setParameterInfo(3, "atex")
			.setParameterInfo(4, "flags")
			.setParameterInfo(5, "renderQueue")
			.setParameterInfo(6, "angle")
			.setParameterInfo(7, "tintColor")
			.setParameterInfo(8, "scale");

		module.addNativeFunction("Renderer.drawCustomSpriteTransformed", lemon::wrap(&Renderer_drawCustomSpriteTransformed), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "px")
			.setParameterInfo(2, "py")
			.setParameterInfo(3, "atex")
			.setParameterInfo(4, "flags")
			.setParameterInfo(5, "renderQueue")
			.setParameterInfo(6, "tintColor")
			.setParameterInfo(7, "transform11")
			.setParameterInfo(8, "transform12")
			.setParameterInfo(9, "transform21")
			.setParameterInfo(10, "transform22");

		module.addNativeFunction("Renderer.extractCustomSprite", lemon::wrap(&Renderer_extractCustomSprite), defaultFlags)
			.setParameterInfo(0, "key")
			.setParameterInfo(1, "categoryName")
			.setParameterInfo(2, "spriteNumber")
			.setParameterInfo(3, "atex");

		module.addNativeFunction("Renderer.addSpriteMask", lemon::wrap(&Renderer_addSpriteMask), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height")
			.setParameterInfo(4, "renderQueue")
			.setParameterInfo(5, "priorityFlag");

		module.addNativeFunction("Renderer.addSpriteMaskWorld", lemon::wrap(&Renderer_addSpriteMaskWorld), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height")
			.setParameterInfo(4, "renderQueue")
			.setParameterInfo(5, "priorityFlag");

		module.addNativeFunction("Renderer.setLogicalSpriteSpace", lemon::wrap(&Renderer_setLogicalSpriteSpace), defaultFlags)
			.setParameterInfo(0, "space");

		module.addNativeFunction("Renderer.clearSpriteTag", lemon::wrap(&Renderer_clearSpriteTag), defaultFlags);

		module.addNativeFunction("Renderer.setSpriteTagWithPosition", lemon::wrap(&Renderer_setSpriteTagWithPosition), defaultFlags)
			.setParameterInfo(0, "spriteTag")
			.setParameterInfo(1, "px")
			.setParameterInfo(2, "py");

		module.addNativeFunction("Renderer.resetViewport", lemon::wrap(&Renderer_resetViewport), defaultFlags)
			.setParameterInfo(0, "renderQueue");

		module.addNativeFunction("Renderer.setViewport", lemon::wrap(&Renderer_setViewport), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height")
			.setParameterInfo(4, "renderQueue");

		module.addNativeFunction("Renderer.setGlobalComponentTint", lemon::wrap(&Renderer_setGlobalComponentTint), defaultFlags)
			.setParameterInfo(0, "tintR")
			.setParameterInfo(1, "tintG")
			.setParameterInfo(2, "tintB")
			.setParameterInfo(3, "addedR")
			.setParameterInfo(4, "addedG")
			.setParameterInfo(5, "addedB");

		// Debug draw rects & texts
		module.addNativeFunction("setWorldSpaceOffset", lemon::wrap(&setWorldSpaceOffset), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py");

		module.addNativeFunction("Debug.drawRect", lemon::wrap(&debugDrawRect), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height");

		module.addNativeFunction("Debug.drawRect", lemon::wrap(&debugDrawRect2), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height")
			.setParameterInfo(4, "color");

		module.addNativeFunction("Renderer.drawText", lemon::wrap(&Renderer_drawText), defaultFlags)
			.setParameterInfo(0, "fontKey")
			.setParameterInfo(1, "px")
			.setParameterInfo(2, "py")
			.setParameterInfo(3, "text")
			.setParameterInfo(4, "tintColor")
			.setParameterInfo(5, "alignment")
			.setParameterInfo(6, "spacing")
			.setParameterInfo(7, "renderQueue")
			.setParameterInfo(8, "useWorldSpace");

		module.addNativeFunction("Renderer.getTextWidth", lemon::wrap(&Renderer_getTextWidth), defaultFlags)
			.setParameterInfo(0, "fontKey")
			.setParameterInfo(1, "text");
		

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
			.setParameterInfo(1, "length");

		module.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel), defaultFlags)
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
			.setParameterInfo(0, "text");

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
	gDebugNotificationInterface = debugNotificationInterface;
}
