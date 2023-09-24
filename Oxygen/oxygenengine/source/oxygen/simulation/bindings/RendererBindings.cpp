/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/bindings/RendererBindings.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/RuntimeEnvironment.h"

#include <lemon/program/DataType.h>
#include <lemon/program/FunctionWrapper.h>
#include <lemon/program/Module.h>


namespace
{
	inline EmulatorInterface& getEmulatorInterface()
	{
		return *lemon::Runtime::getActiveEnvironmentSafe<RuntimeEnvironment>().mEmulatorInterface;
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
				result = getEmulatorInterface().readVRam16(mWriteAddress);
				break;
			}

			case WriteTarget::VSRAM:
			{
				const uint8 index = (mWriteAddress / 2) & 0x3f;
				result = getEmulatorInterface().getVSRam()[index];
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
				if (nullptr != LemonScriptBindings::mDebugNotificationInterface)
					LemonScriptBindings::mDebugNotificationInterface->onVRAMWrite(mWriteAddress, 2);

				getEmulatorInterface().writeVRam16(mWriteAddress, value);
				break;
			}

			case WriteTarget::VSRAM:
			{
				const uint8 index = (mWriteAddress / 2) & 0x3f;
				getEmulatorInterface().getVSRam()[index] = value;
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

		EmulatorInterface& emulatorInterface = getEmulatorInterface();
		if (mWriteIncrement == 2)
		{
			// Optimized version of the code below
			if (nullptr != LemonScriptBindings::mDebugNotificationInterface)
			{
				LemonScriptBindings::mDebugNotificationInterface->onVRAMWrite(mWriteAddress, bytes);
			}

			emulatorInterface.copyFromMemoryToVRam(mWriteAddress, address, bytes);
			mWriteAddress += bytes;
		}
		else
		{
			if (nullptr != LemonScriptBindings::mDebugNotificationInterface)
			{
				for (uint16 i = 0; i < bytes; i += 2)
					LemonScriptBindings::mDebugNotificationInterface->onVRAMWrite(mWriteAddress + mWriteIncrement * i/2, 2);
			}

			for (uint16 i = 0; i < bytes; i += 2)
			{
				const uint16 value = emulatorInterface.readMemory16(address);
				emulatorInterface.writeVRam16(mWriteAddress, value);
				mWriteAddress += mWriteIncrement;
				address += 2;
			}
		}
	}

	void VDP_fillVRAMbyDMA(uint16 fillValue, uint16 vramAddress, uint16 bytes)
	{
		RMX_CHECK(uint32(vramAddress) + bytes <= 0x10000, "Invalid VRAM access from " << rmx::hexString(vramAddress, 8) << " to " << rmx::hexString(uint32(vramAddress)+bytes-1, 8) << " in VDP_fillVRAMbyDMA", return);

		if (nullptr != LemonScriptBindings::mDebugNotificationInterface)
			LemonScriptBindings::mDebugNotificationInterface->onVRAMWrite(vramAddress, bytes);

		getEmulatorInterface().fillVRam(vramAddress, fillValue, bytes);
		mWriteAddress = vramAddress + bytes;
	}

	void VDP_zeroVRAM(uint16 bytes)
	{
		if (nullptr != LemonScriptBindings::mDebugNotificationInterface)
			LemonScriptBindings::mDebugNotificationInterface->onVRAMWrite(mWriteAddress, bytes);

		VDP_fillVRAMbyDMA(0, mWriteAddress, bytes);
	}

	void VDP_copyToCRAM(uint32 address, uint16 bytes)
	{
		RMX_ASSERT(mWriteAddress < 0x80 && mWriteAddress + bytes <= 0x80, "Invalid write access to CRAM");
		RMX_ASSERT((mWriteAddress % 2) == 0, "Invalid CRAM write address " << mWriteAddress);
		RMX_ASSERT((mWriteIncrement % 2) == 0, "Invalid CRAM write increment " << mWriteIncrement);

		EmulatorInterface& emulatorInterface = getEmulatorInterface();
		PaletteManager& paletteManager = RenderParts::instance().getPaletteManager();
		for (uint16 i = 0; i < bytes; i += 2)
		{
			const uint16 colorValue = emulatorInterface.readMemory16(address + i);
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
		return getEmulatorInterface().readVRam16(vramAddress);
	}

	void setVRAM(uint16 vramAddress, uint16 value)
	{
		getEmulatorInterface().writeVRam16(vramAddress, value);
	}


	void Renderer_setPaletteColor(uint16 index, uint32 color)
	{
		RenderParts::instance().getPaletteManager().writePaletteEntry(0, index, color);
	}

	void Renderer_setPaletteColorPacked(uint16 index, uint16 color)
	{
		RenderParts::instance().getPaletteManager().writePaletteEntryPacked(0, index, color);
	}

	void Renderer_enableSecondaryPalette(uint8 line)
	{
		RenderParts::instance().getPaletteManager().setPaletteSplitPositionY(line);
	}

	void Renderer_setSecondaryPaletteColorPacked(uint16 index, uint16 color)
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
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), sourceBase, words / 0x10, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_NONE);
	}

	uint64 Renderer_setupCustomCharacterSprite(uint32 sourceBase, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), sourceBase, tableAddress, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_CHARACTER);
	}

	uint64 Renderer_setupCustomObjectSprite(uint32 sourceBase, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), sourceBase, tableAddress, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_OBJECT);
	}

	uint64 Renderer_setupKosinskiCompressedSprite1(uint32 sourceAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), sourceAddress, 0, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_KOSINSKI);
	}

	uint64 Renderer_setupKosinskiCompressedSprite2(uint32 sourceAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex, int16 indexOffset)
	{
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), sourceAddress, 0, mappingOffset, animationSprite, atex, SpriteCache::ENCODING_KOSINSKI, indexOffset);
	}

	void Renderer_drawSprite1(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue);
	}

	void Renderer_drawSprite2(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, uint8 angle, uint8 alpha)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color(1.0f, 1.0f, 1.0f, (float)alpha / 255.0f), (float)angle / 128.0f * PI_FLOAT);
	}

	void Renderer_drawSpriteTinted(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, float angle, uint32 tintColor, float scale)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), angle, scale);
	}

	void Renderer_drawSpriteTinted2(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, uint8 angle, uint32 tintColor, int32 scale)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), (float)angle / 128.0f * PI_FLOAT, (float)scale / 65536.0f);
	}

	void Renderer_drawSpriteTinted3(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, float angle, uint32 tintColor, float scaleX, float scaleY)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), angle, Vec2f(scaleX, scaleY));
	}

	void Renderer_drawSpriteTinted4(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, uint8 angle, uint32 tintColor, int32 scaleX, int32 scaleY)
	{
		RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), (float)angle / 128.0f * PI_FLOAT, Vec2f((float)scaleX, (float)scaleY) / 65536.0f);
	}

	void Renderer_drawSpriteTransformed(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, uint32 tintColor, float transform11, float transform12, float transform21, float transform22)
	{
		Transform2D transformation;
		transformation.setByMatrix(transform11, transform12, transform21, transform22);
		RenderParts::instance().getSpriteManager().drawCustomSpriteWithTransform(key, Vec2i(px, py), atex, flags, renderQueue, Color::fromRGBA32(tintColor), transformation);
	}

	void Renderer_drawSpriteTransformed2(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue, uint32 tintColor, int32 transform11, int32 transform12, int32 transform21, int32 transform22)
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
		RenderParts::instance().getSpriteManager().addSpriteMask(Vec2i(px, py), Vec2i(width, height), renderQueue, priorityFlag != 0, SpacesManager::Space::SCREEN);
	}

	void Renderer_addSpriteMaskWorld(int16 px, int16 py, int16 width, int16 height, uint16 renderQueue, uint8 priorityFlag)
	{
		RenderParts::instance().getSpriteManager().addSpriteMask(Vec2i(px, py), Vec2i(width, height), renderQueue, priorityFlag != 0, SpacesManager::Space::WORLD);
	}

	void Renderer_setLogicalSpriteSpace(uint8 space)
	{
		RMX_CHECK(space < 2, "Invalid space index " << space, return);
		RenderParts::instance().getSpriteManager().setLogicalSpriteSpace((SpacesManager::Space)space);
	}

	void Renderer_clearSpriteTag()
	{
		RenderParts::instance().getSpriteManager().clearSpriteTag();
	}

	void Renderer_setSpriteTagWithPosition(uint64 spriteTag, uint16 px, uint16 py)
	{
		RenderParts::instance().getSpriteManager().setSpriteTagWithPosition(spriteTag, Vec2i(px, py));
	}

	void Renderer_setScreenSize(uint16 width, uint16 height)
	{
		width = clamp(width, 128, 1024);
		height = clamp(height, 128, 1024);
		VideoOut::instance().setScreenSize(width, height);
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


	struct SpriteHandleWrapper
	{
		uint32 mHandle = 0;
		static inline const lemon::CustomDataType* mDataType = nullptr;
	};

	SpriteHandleWrapper Renderer_addSpriteHandle(uint64 spriteKey, int32 px, int32 py, uint16 renderQueue)
	{
		const uint32 handle = RenderParts::instance().getSpriteManager().addSpriteHandle(spriteKey, Vec2i(px, py), renderQueue);
		return SpriteHandleWrapper { handle };
	}

	void SpriteHandle_setFlags(SpriteHandleWrapper spriteHandle, uint8 flags)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mFlipX = (flags & 0x01) != 0;
			spriteHandleData->mFlipY = (flags & 0x02) != 0;
			spriteHandleData->mBlendMode = (flags & 0x10) ? BlendMode::OPAQUE : BlendMode::ALPHA;
			spriteHandleData->mCoordinatesSpace = ((flags & 0x20) != 0) ? SpacesManager::Space::WORLD : SpacesManager::Space::SCREEN;
			spriteHandleData->mPriorityFlag = (flags & 0x40) != 0;
			spriteHandleData->mUseGlobalComponentTint = (flags & 0x80) == 0;
		}
	}

	void SpriteHandle_setFlipX(SpriteHandleWrapper spriteHandle, bool flipX)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mFlipX = flipX;
		}
	}

	void SpriteHandle_setFlipY(SpriteHandleWrapper spriteHandle, bool flipY)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mFlipY = flipY;
		}
	}

	void SpriteHandle_setRotation(SpriteHandleWrapper spriteHandle, float degrees)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = degrees * PI_FLOAT / 180.f;
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setScale1(SpriteHandleWrapper spriteHandle, float scale)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mScale = Vec2f(scale);
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setScale2(SpriteHandleWrapper spriteHandle, float scaleX, float scaleY)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mScale = Vec2f(scaleX, scaleY);
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setRotationScale1(SpriteHandleWrapper spriteHandle, float degrees, float scale)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = degrees * PI_FLOAT / 180.0f;
			spriteHandleData->mScale = Vec2f(scale);
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setRotationScale2(SpriteHandleWrapper spriteHandle, float degrees, float scaleX, float scaleY)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = degrees * PI_FLOAT / 180.0f;
			spriteHandleData->mScale = Vec2f(scaleX, scaleY);
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setTransform(SpriteHandleWrapper spriteHandle, float transform11, float transform12, float transform21, float transform22)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = 0.0f;
			spriteHandleData->mScale = Vec2f(1.0f, 1.0f);
			spriteHandleData->mTransformation.setByMatrix(transform11, transform12, transform21, transform22);
		}
	}

	void SpriteHandle_setPriorityFlag(SpriteHandleWrapper spriteHandle, bool priorityFlag)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mPriorityFlag = priorityFlag;
		}
	}

	void SpriteHandle_setCoordinateSpace(SpriteHandleWrapper spriteHandle, uint8 space)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mCoordinatesSpace = (space == 0) ? SpacesManager::Space::SCREEN : SpacesManager::Space::WORLD;
		}
	}

	void SpriteHandle_setUseGlobalComponentTint(SpriteHandleWrapper spriteHandle, bool enable)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mUseGlobalComponentTint = enable;
		}
	}

	void SpriteHandle_setBlendMode(SpriteHandleWrapper spriteHandle, uint8 blendMode)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mBlendMode = (BlendMode)blendMode;
			if (spriteHandleData->mBlendMode > BlendMode::MAXIMUM)
				spriteHandleData->mBlendMode = BlendMode::ALPHA;
		}
	}

	void SpriteHandle_setPaletteOffset(SpriteHandleWrapper spriteHandle, uint16 paletteOffset)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mAtex = paletteOffset;
		}
	}

	void SpriteHandle_setTintColor(SpriteHandleWrapper spriteHandle, float red, float green, float blue, float alpha)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mTintColor.set(red, green, blue, alpha);
		}
	}

	void SpriteHandle_setOpacity(SpriteHandleWrapper spriteHandle, float opacity)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mTintColor.a = opacity;
		}
	}

	void SpriteHandle_setAddedColor(SpriteHandleWrapper spriteHandle, float red, float green, float blue)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mAddedColor.set(red, green, blue, 0.0f);
		}
	}

	void SpriteHandle_setSpriteTag(SpriteHandleWrapper spriteHandle, uint64 spriteTag, int32 px, int32 py)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mSpriteTag = spriteTag;
			spriteHandleData->mTaggedSpritePosition.set(px, py);
		}
	}
}


namespace lemon
{
	namespace traits
	{
		template<> const DataTypeDefinition* getDataType<SpriteHandleWrapper>()  { return SpriteHandleWrapper::mDataType; }
	}

	namespace internal
	{
		template<>
		void pushStackGeneric<SpriteHandleWrapper>(SpriteHandleWrapper value, const NativeFunction::Context context)
		{
			context.mControlFlow.pushValueStack(value.mHandle);
		};

		template<>
		SpriteHandleWrapper popStackGeneric(const NativeFunction::Context context)
		{
			return SpriteHandleWrapper { context.mControlFlow.popValueStack<uint32>() };
		}
	}
}


void RendererBindings::registerBindings(lemon::Module& module)
{
	// Data type
	SpriteHandleWrapper::mDataType = module.addDataType("SpriteHandle", lemon::BaseType::UINT_32);


	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);

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


	// Renderer functions
	module.addNativeFunction("Renderer.setPaletteColor", lemon::wrap(&Renderer_setPaletteColor), defaultFlags)
		.setParameterInfo(0, "index")
		.setParameterInfo(1, "color");

	module.addNativeFunction("Renderer.setPaletteColorPacked", lemon::wrap(&Renderer_setPaletteColorPacked), defaultFlags)
		.setParameterInfo(0, "index")
		.setParameterInfo(1, "color");

	module.addNativeFunction("Renderer.enableSecondaryPalette", lemon::wrap(&Renderer_enableSecondaryPalette), defaultFlags)
		.setParameterInfo(0, "line");

	module.addNativeFunction("Renderer.setSecondaryPaletteColorPacked", lemon::wrap(&Renderer_setSecondaryPaletteColorPacked), defaultFlags)
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

	module.addNativeFunction("Renderer.drawSprite", lemon::wrap(&Renderer_drawSprite1), defaultFlags)
		.setParameterInfo(0, "key")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "atex")
		.setParameterInfo(4, "flags")
		.setParameterInfo(5, "renderQueue");

	module.addNativeFunction("Renderer.drawSprite", lemon::wrap(&Renderer_drawSprite2), defaultFlags)
		.setParameterInfo(0, "key")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "atex")
		.setParameterInfo(4, "flags")
		.setParameterInfo(5, "renderQueue")
		.setParameterInfo(6, "angle")
		.setParameterInfo(7, "alpha");

	module.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted), defaultFlags)
		.setParameterInfo(0, "key")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "atex")
		.setParameterInfo(4, "flags")
		.setParameterInfo(5, "renderQueue")
		.setParameterInfo(6, "angle")
		.setParameterInfo(7, "tintColor")
		.setParameterInfo(8, "scale");

	module.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted2), defaultFlags)
		.setParameterInfo(0, "key")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "atex")
		.setParameterInfo(4, "flags")
		.setParameterInfo(5, "renderQueue")
		.setParameterInfo(6, "angle")
		.setParameterInfo(7, "tintColor")
		.setParameterInfo(8, "scale");

	module.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted3), defaultFlags)
		.setParameterInfo(0, "key")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "atex")
		.setParameterInfo(4, "flags")
		.setParameterInfo(5, "renderQueue")
		.setParameterInfo(6, "angle")
		.setParameterInfo(7, "tintColor")
		.setParameterInfo(8, "scaleX")
		.setParameterInfo(9, "scaleY");

	module.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted4), defaultFlags)
		.setParameterInfo(0, "key")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "atex")
		.setParameterInfo(4, "flags")
		.setParameterInfo(5, "renderQueue")
		.setParameterInfo(6, "angle")
		.setParameterInfo(7, "tintColor")
		.setParameterInfo(8, "scaleX")
		.setParameterInfo(9, "scaleY");

	module.addNativeFunction("Renderer.drawSpriteTransformed", lemon::wrap(&Renderer_drawSpriteTransformed), defaultFlags)
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

	module.addNativeFunction("Renderer.drawSpriteTransformed", lemon::wrap(&Renderer_drawSpriteTransformed2), defaultFlags)
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

	module.addNativeFunction("Renderer.setScreenSize", lemon::wrap(&Renderer_setScreenSize), defaultFlags)
		.setParameterInfo(0, "width")
		.setParameterInfo(1, "height");

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


	// Debug draw rects
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


	// Sprite handle
	module.addNativeFunction("Renderer.addSpriteHandle", lemon::wrap(&Renderer_addSpriteHandle), defaultFlags)
		.setParameterInfo(0, "spriteKey")
		.setParameterInfo(1, "px")
		.setParameterInfo(2, "py")
		.setParameterInfo(3, "renderQueue");

	module.addNativeMethod("SpriteHandle", "setFlags", lemon::wrap(&SpriteHandle_setFlags), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "flags");

	module.addNativeMethod("SpriteHandle", "setFlipX", lemon::wrap(&SpriteHandle_setFlipX), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "flipX");

	module.addNativeMethod("SpriteHandle", "setFlipY", lemon::wrap(&SpriteHandle_setFlipY), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "flipY");

	module.addNativeMethod("SpriteHandle", "setRotation", lemon::wrap(&SpriteHandle_setRotation), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "degrees");

	module.addNativeMethod("SpriteHandle", "setScale", lemon::wrap(&SpriteHandle_setScale1), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "scale");

	module.addNativeMethod("SpriteHandle", "setScale", lemon::wrap(&SpriteHandle_setScale2), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "scaleX")
		.setParameterInfo(2, "scaleY");

	module.addNativeMethod("SpriteHandle", "setRotationScale", lemon::wrap(&SpriteHandle_setRotationScale1), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "degrees")
		.setParameterInfo(2, "scale");

	module.addNativeMethod("SpriteHandle", "setRotationScale", lemon::wrap(&SpriteHandle_setRotationScale2), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "degrees")
		.setParameterInfo(2, "scaleX")
		.setParameterInfo(3, "scaleY");

	module.addNativeMethod("SpriteHandle", "setTransform", lemon::wrap(&SpriteHandle_setTransform), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "transform11")
		.setParameterInfo(2, "transform12")
		.setParameterInfo(3, "transform21")
		.setParameterInfo(4, "transform22");

	module.addNativeMethod("SpriteHandle", "setPriorityFlag", lemon::wrap(&SpriteHandle_setPriorityFlag), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "priorityFlag");

	module.addNativeMethod("SpriteHandle", "setCoordinateSpace", lemon::wrap(&SpriteHandle_setCoordinateSpace), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "space");

	module.addNativeMethod("SpriteHandle", "setUseGlobalComponentTint", lemon::wrap(&SpriteHandle_setUseGlobalComponentTint), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "enable");

	module.addNativeMethod("SpriteHandle", "setBlendMode", lemon::wrap(&SpriteHandle_setBlendMode), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "blendMode");

	module.addNativeMethod("SpriteHandle", "setPaletteOffset", lemon::wrap(&SpriteHandle_setPaletteOffset), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "paletteOffset");

	module.addNativeMethod("SpriteHandle", "setTintColor", lemon::wrap(&SpriteHandle_setTintColor), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "red")
		.setParameterInfo(2, "green")
		.setParameterInfo(3, "blue")
		.setParameterInfo(4, "alpha");

	module.addNativeMethod("SpriteHandle", "setOpacity", lemon::wrap(&SpriteHandle_setOpacity), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "opacity");

	module.addNativeMethod("SpriteHandle", "setAddedColor", lemon::wrap(&SpriteHandle_setAddedColor), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "red")
		.setParameterInfo(2, "green")
		.setParameterInfo(3, "blue");

	module.addNativeMethod("SpriteHandle", "setSpriteTag", lemon::wrap(&SpriteHandle_setSpriteTag), defaultFlags)
		.setParameterInfo(0, "this")
		.setParameterInfo(1, "spriteTag")
		.setParameterInfo(2, "px")
		.setParameterInfo(3, "py");
}
