/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
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

#include <lemon/program/ModuleBindingsBuilder.h>


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


	uint32 Color_fromHSVA(float hue, float saturation, float value, float alpha)
	{
		Color color;
		color.setFromHSV(Vec3f(hue, saturation, value));
		color.a = alpha;
		return color.getRGBA32();
	}

	uint32 Color_fromHSV(float hue, float saturation, float value)
	{
		return Color_fromHSVA(hue, saturation, value, 1.0f);
	}

	float Color_HSV_getHue(uint32 color)		{ return Color::fromRGBA32(color).getHSV().x; }
	float Color_HSV_getSaturation(uint32 color)	{ return Color::fromRGBA32(color).getHSV().y; }
	float Color_HSV_getValue(uint32 color)		{ return Color::fromRGBA32(color).getHSV().z; }

	uint32 Color_lerp(uint32 a, uint32 b, float factor)
	{
		return Color::interpolateColor(Color::fromRGBA32(a), Color::fromRGBA32(b), factor).getRGBA32();
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
		RenderParts::instance().getSpriteManager().setResetRenderItems(true);
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
		SpriteCache::ROMSpriteData romSpriteData;
		romSpriteData.mPatternsBaseAddress = sourceBase;
		romSpriteData.mTableAddress = words / 0x10;
		romSpriteData.mMappingOffset = mappingOffset;
		romSpriteData.mAnimationSprite = animationSprite;
		romSpriteData.mEncoding = SpriteCache::ROMSpriteEncoding::NONE;
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), romSpriteData, atex).mKey;
	}

	uint64 Renderer_setupCustomCharacterSprite(uint32 sourceBase, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		SpriteCache::ROMSpriteData romSpriteData;
		romSpriteData.mPatternsBaseAddress = sourceBase;
		romSpriteData.mTableAddress = tableAddress;
		romSpriteData.mMappingOffset = mappingOffset;
		romSpriteData.mAnimationSprite = animationSprite;
		romSpriteData.mEncoding = SpriteCache::ROMSpriteEncoding::CHARACTER;
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), romSpriteData, atex).mKey;
	}

	uint64 Renderer_setupCustomObjectSprite(uint32 sourceBase, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		SpriteCache::ROMSpriteData romSpriteData;
		romSpriteData.mPatternsBaseAddress = sourceBase;
		romSpriteData.mTableAddress = tableAddress;
		romSpriteData.mMappingOffset = mappingOffset;
		romSpriteData.mAnimationSprite = animationSprite;
		romSpriteData.mEncoding = SpriteCache::ROMSpriteEncoding::OBJECT;
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), romSpriteData, atex).mKey;
	}

	uint64 Renderer_setupKosinskiCompressedSprite2(uint32 sourceAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex, int16 indexOffset)
	{
		SpriteCache::ROMSpriteData romSpriteData;
		romSpriteData.mPatternsBaseAddress = sourceAddress;
		romSpriteData.mTableAddress = 0;
		romSpriteData.mMappingOffset = mappingOffset;
		romSpriteData.mAnimationSprite = animationSprite;
		romSpriteData.mEncoding = SpriteCache::ROMSpriteEncoding::KOSINSKI;
		romSpriteData.mIndexOffset = indexOffset;
		return SpriteCache::instance().setupSpriteFromROM(getEmulatorInterface(), romSpriteData, atex).mKey;
	}

	uint64 Renderer_setupKosinskiCompressedSprite1(uint32 sourceAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex)
	{
		return Renderer_setupKosinskiCompressedSprite2(sourceAddress, mappingOffset, animationSprite, atex, 0);
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

	void Renderer_drawRect(int32 px, int32 py, int32 width, int32 height, uint32 color, uint16 renderQueue, bool useWorldSpace)
	{
		RenderParts::instance().getSpriteManager().addRectangle(Recti(px, py, width, height), Color::fromRGBA32(color), renderQueue, useWorldSpace ? SpacesManager::Space::WORLD : SpacesManager::Space::SCREEN, false);
	}

	void Renderer_drawRect2(int32 px, int32 py, int32 width, int32 height, uint32 color, uint16 renderQueue, bool useWorldSpace, bool useGlobalComponentTint)
	{
		RenderParts::instance().getSpriteManager().addRectangle(Recti(px, py, width, height), Color::fromRGBA32(color), renderQueue, useWorldSpace ? SpacesManager::Space::WORLD : SpacesManager::Space::SCREEN, useGlobalComponentTint);
	}

	void Renderer_drawText(lemon::StringRef fontKey, int32 px, int32 py, lemon::StringRef text, uint32 tintColor, uint8 alignment, int8 spacing, uint16 renderQueue, bool useWorldSpace, bool useGlobalComponentTint)
	{
		RMX_CHECK(alignment >= 1 && alignment <= 9, "Invalid alignment " << alignment << " used for drawing text, fallback to alignment = 1", alignment = 1);
		if (fontKey.isValid() && text.isValid())
		{
			RenderParts::instance().getSpriteManager().addText(fontKey.getString(), fontKey.getHash(), Vec2i(px, py), text.getString(), text.getHash(), Color::fromRGBA32(tintColor), (int)alignment, (int)spacing, renderQueue, useWorldSpace ? SpacesManager::Space::WORLD : SpacesManager::Space::SCREEN, useGlobalComponentTint);
		}
	}

	void Renderer_drawText2(lemon::StringRef fontKey, int32 px, int32 py, lemon::StringRef text, uint32 tintColor, uint8 alignment, int8 spacing, uint16 renderQueue, bool useWorldSpace)
	{
		Renderer_drawText(fontKey, px, py, text, tintColor, alignment, spacing, renderQueue, useWorldSpace, false);
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

	void SpriteHandle_setRotationRadians(SpriteHandleWrapper spriteHandle, float radians)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = radians;
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setRotation(SpriteHandleWrapper spriteHandle, float degrees)
	{
		SpriteHandle_setRotationRadians(spriteHandle, degrees * (PI_FLOAT / 180.f));
	}

	void SpriteHandle_setRotation_u8(SpriteHandleWrapper spriteHandle, uint8 angle)
	{
		SpriteHandle_setRotationRadians(spriteHandle, (float)angle * (360.0f / 256.0f));
	}

	void SpriteHandle_setScaleXY(SpriteHandleWrapper spriteHandle, float scaleX, float scaleY)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mScale = Vec2f(scaleX, scaleY);
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setScaleUniform(SpriteHandleWrapper spriteHandle, float scale)
	{
		SpriteHandle_setScaleXY(spriteHandle, scale, scale);
	}

	void SpriteHandle_setScaleXY_s32(SpriteHandleWrapper spriteHandle, int32 scaleX, int32 scaleY)
	{
		SpriteHandle_setScaleXY(spriteHandle, (float)scaleX / 65536.0f, (float)scaleY / 65536.0f);
	}

	void SpriteHandle_setScaleUniform_s32(SpriteHandleWrapper spriteHandle, int32 scale)
	{
		SpriteHandle_setScaleUniform(spriteHandle, (float)scale / 65536.0f);
	}

	void SpriteHandle_setRotationScale1(SpriteHandleWrapper spriteHandle, float degrees, float scale)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = degrees * (PI_FLOAT / 180.0f);
			spriteHandleData->mScale = Vec2f(scale);
			spriteHandleData->mTransformation.setIdentity();
		}
	}

	void SpriteHandle_setRotationScale2(SpriteHandleWrapper spriteHandle, float degrees, float scaleX, float scaleY)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mRotation = degrees * (PI_FLOAT / 180.0f);
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

	void SpriteHandle_setTintColorInternal(SpriteHandleWrapper spriteHandle, const Color& color)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mTintColor = color;
		}
	}

	void SpriteHandle_setTintColor(SpriteHandleWrapper spriteHandle, float red, float green, float blue, float alpha)
	{
		SpriteHandle_setTintColorInternal(spriteHandle, Color(red, green, blue, alpha));
	}

	void SpriteHandle_setTintColor_u8(SpriteHandleWrapper spriteHandle, uint8 red, uint8 green, uint8 blue, uint8 alpha)
	{
		SpriteHandle_setTintColorInternal(spriteHandle, Color((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f));
	}

	void SpriteHandle_setTintColorRGBA(SpriteHandleWrapper spriteHandle, uint32 rgba)
	{
		SpriteHandle_setTintColorInternal(spriteHandle, Color::fromRGBA32(rgba));
	}

	void SpriteHandle_setOpacity(SpriteHandleWrapper spriteHandle, float opacity)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mTintColor.a = opacity;
		}
	}

	void SpriteHandle_setAddedColorInternal(SpriteHandleWrapper spriteHandle, const Color& color)
	{
		SpriteManager::SpriteHandleData* spriteHandleData = RenderParts::instance().getSpriteManager().getSpriteHandleData(spriteHandle.mHandle);
		if (nullptr != spriteHandleData)
		{
			spriteHandleData->mAddedColor = color;
		}
	}

	void SpriteHandle_setAddedColor(SpriteHandleWrapper spriteHandle, float red, float green, float blue)
	{
		SpriteHandle_setAddedColorInternal(spriteHandle, Color(red, green, blue, 0.0f));
	}

	void SpriteHandle_setAddedColor_u8(SpriteHandleWrapper spriteHandle, uint8 red, uint8 green, uint8 blue)
	{
		SpriteHandle_setAddedColorInternal(spriteHandle, Color((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f));
	}

	void SpriteHandle_setAddedColorRGB(SpriteHandleWrapper spriteHandle, uint32 rgb)
	{
		SpriteHandle_setAddedColorInternal(spriteHandle, Color::fromRGBA32(rgb << 8));	// With alpha bits = 0
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
	lemon::ModuleBindingsBuilder builder(module);

	// Data type
	SpriteHandleWrapper::mDataType = module.addDataType("SpriteHandle", lemon::BaseType::UINT_32);

	// Constants
	builder.addConstant<uint8>("BlendMode.OPAQUE",		   BlendMode::OPAQUE);
	builder.addConstant<uint8>("BlendMode.ALPHA",		   BlendMode::ALPHA);
	builder.addConstant<uint8>("BlendMode.ADDITIVE",	   BlendMode::ADDITIVE);
	builder.addConstant<uint8>("BlendMode.SUBTRACTIVE",	   BlendMode::SUBTRACTIVE);
	builder.addConstant<uint8>("BlendMode.MULTIPLICATIVE", BlendMode::MULTIPLICATIVE);
	builder.addConstant<uint8>("BlendMode.MINIMUM",		   BlendMode::MINIMUM);
	builder.addConstant<uint8>("BlendMode.MAXIMUM",		   BlendMode::MAXIMUM);

	// Functions
	{
		const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);
		const BitFlagSet<lemon::Function::Flag> compileTimeConstant(lemon::Function::Flag::ALLOW_INLINE_EXECUTION, lemon::Function::Flag::COMPILE_TIME_CONSTANT);


		// Screen size query
		builder.addNativeFunction("getScreenWidth", lemon::wrap(&getScreenWidth), defaultFlags);
		builder.addNativeFunction("getScreenHeight", lemon::wrap(&getScreenHeight), defaultFlags);
		builder.addNativeFunction("getScreenExtend", lemon::wrap(&getScreenExtend), defaultFlags);


		// Color
		builder.addNativeFunction("Color.fromHSV", lemon::wrap(&Color_fromHSV), compileTimeConstant)
			.setParameters("hue", "saturation", "value");

		builder.addNativeFunction("Color.fromHSV", lemon::wrap(&Color_fromHSVA), compileTimeConstant)
			.setParameters("hue", "saturation", "value", "alpha");

		builder.addNativeFunction("Color.HSV.getHue", lemon::wrap(&Color_HSV_getHue), compileTimeConstant)
			.setParameters("color");

		builder.addNativeFunction("Color.HSV.getSaturation", lemon::wrap(&Color_HSV_getSaturation), compileTimeConstant)
			.setParameters("color");

		builder.addNativeFunction("Color.HSV.getValue", lemon::wrap(&Color_HSV_getValue), compileTimeConstant)
			.setParameters("color");

		builder.addNativeFunction("Color.lerp", lemon::wrap(&Color_lerp), compileTimeConstant)
			.setParameters("colorA", "colorB", "factor");


		// VDP emulation
		builder.addNativeFunction("VDP.setupVRAMWrite", lemon::wrap(&VDP_setupVRAMWrite), defaultFlags)
			.setParameters("vramAddress");

		builder.addNativeFunction("VDP.setupVSRAMWrite", lemon::wrap(&VDP_setupVSRAMWrite), defaultFlags)
			.setParameters("vsramAddress");

		builder.addNativeFunction("VDP.setupCRAMWrite", lemon::wrap(&VDP_setupCRAMWrite), defaultFlags)
			.setParameters("cramAddress");

		builder.addNativeFunction("VDP.setWriteIncrement", lemon::wrap(&VDP_setWriteIncrement), defaultFlags)
			.setParameters("increment");

		builder.addNativeFunction("VDP.readData16", lemon::wrap(&VDP_readData16), defaultFlags);

		builder.addNativeFunction("VDP.readData32", lemon::wrap(&VDP_readData32), defaultFlags);

		builder.addNativeFunction("VDP.writeData16", lemon::wrap(&VDP_writeData16), defaultFlags)
			.setParameters("value");

		builder.addNativeFunction("VDP.writeData32", lemon::wrap(&VDP_writeData32), defaultFlags)
			.setParameters("value");

		builder.addNativeFunction("VDP.copyToVRAM", lemon::wrap(&VDP_copyToVRAM), defaultFlags)
			.setParameters("address", "bytes");

		builder.addNativeFunction("VDP.fillVRAMbyDMA", lemon::wrap(&VDP_fillVRAMbyDMA), defaultFlags)
			.setParameters("fillValue", "vramAddress", "bytes");

		builder.addNativeFunction("VDP.zeroVRAM", lemon::wrap(&VDP_zeroVRAM), defaultFlags)
			.setParameters("bytes");

		builder.addNativeFunction("VDP.copyToVRAMbyDMA", lemon::wrap(&VDP_copyToVRAMbyDMA), defaultFlags)
			.setParameters("sourceAddress", "vramAddress", "bytes");

		builder.addNativeFunction("VDP.copyToCRAMbyDMA", lemon::wrap(&VDP_copyToCRAMbyDMA), defaultFlags)
			.setParameters("sourceAddress", "vramAddress", "bytes");


		// VDP config
		builder.addNativeFunction("VDP.Config.setActiveDisplay", lemon::wrap(&VDP_Config_setActiveDisplay), defaultFlags)
			.setParameters("enable");

		builder.addNativeFunction("VDP.Config.setNameTableBasePlaneB", lemon::wrap(&VDP_Config_setNameTableBasePlaneB), defaultFlags)
			.setParameters("vramAddress");

		builder.addNativeFunction("VDP.Config.setNameTableBasePlaneA", lemon::wrap(&VDP_Config_setNameTableBasePlaneA), defaultFlags)
			.setParameters("vramAddress");

		builder.addNativeFunction("VDP.Config.setNameTableBasePlaneW", lemon::wrap(&VDP_Config_setNameTableBasePlaneW), defaultFlags)
			.setParameters("vramAddress");

		builder.addNativeFunction("VDP.Config.setBackdropColor", lemon::wrap(&VDP_Config_setBackdropColor), defaultFlags)
			.setParameters("paletteIndex");

		builder.addNativeFunction("VDP.Config.setVerticalScrolling", lemon::wrap(&VDP_Config_setVerticalScrolling), defaultFlags)
			.setParameters("verticalScrolling", "horizontalScrollMask");

		builder.addNativeFunction("VDP.Config.setRenderingModeConfiguration", lemon::wrap(&VDP_Config_setRenderingModeConfiguration), defaultFlags)
			.setParameters("shadowHighlightPalette");

		builder.addNativeFunction("VDP.Config.setHorizontalScrollTableBase", lemon::wrap(&VDP_Config_setHorizontalScrollTableBase), defaultFlags)
			.setParameters("vramAddress");

		builder.addNativeFunction("VDP.Config.setPlayfieldSizeInPatterns", lemon::wrap(&VDP_Config_setPlayfieldSizeInPatterns), defaultFlags)
			.setParameters("width", "height");

		builder.addNativeFunction("VDP.Config.setPlayfieldSizeInPixels", lemon::wrap(&VDP_Config_setPlayfieldSizeInPixels), defaultFlags)
			.setParameters("width", "height");

		builder.addNativeFunction("VDP.Config.setupWindowPlane", lemon::wrap(&VDP_Config_setupWindowPlane), defaultFlags)
			.setParameters("useWindowPlane", "splitY");

		builder.addNativeFunction("VDP.Config.setPlaneWScrollOffset", lemon::wrap(&VDP_Config_setPlaneWScrollOffset), defaultFlags)
			.setParameters("x", "y");

		builder.addNativeFunction("VDP.Config.setSpriteAttributeTableBase", lemon::wrap(&VDP_Config_setSpriteAttributeTableBase), defaultFlags)
			.setParameters("vramAddress");


		// Direct VRAM access
		builder.addNativeFunction("getVRAM", lemon::wrap(&getVRAM), defaultFlags)
			.setParameters("vramAddress");

		builder.addNativeFunction("setVRAM", lemon::wrap(&setVRAM), defaultFlags)
			.setParameters("vramAddress", "value");


		// Renderer functions
		builder.addNativeFunction("Renderer.setPaletteColor", lemon::wrap(&Renderer_setPaletteColor), defaultFlags)
			.setParameters("index", "color");

		builder.addNativeFunction("Renderer.setPaletteColorPacked", lemon::wrap(&Renderer_setPaletteColorPacked), defaultFlags)
			.setParameters("index", "color");

		builder.addNativeFunction("Renderer.enableSecondaryPalette", lemon::wrap(&Renderer_enableSecondaryPalette), defaultFlags)
			.setParameters("line");

		builder.addNativeFunction("Renderer.setSecondaryPaletteColorPacked", lemon::wrap(&Renderer_setSecondaryPaletteColorPacked), defaultFlags)
			.setParameters("index", "color");

		builder.addNativeFunction("Renderer.setScrollOffsetH", lemon::wrap(&Renderer_setScrollOffsetH), defaultFlags)
			.setParameters("setIndex", "lineNumber", "value");

		builder.addNativeFunction("Renderer.setScrollOffsetV", lemon::wrap(&Renderer_setScrollOffsetV), defaultFlags)
			.setParameters("setIndex", "rowNumber", "value");

		builder.addNativeFunction("Renderer.setHorizontalScrollNoRepeat", lemon::wrap(&Renderer_setHorizontalScrollNoRepeat), defaultFlags)
			.setParameters("setIndex", "enable");

		builder.addNativeFunction("Renderer.setVerticalScrollOffsetBias", lemon::wrap(&Renderer_setVerticalScrollOffsetBias), defaultFlags)
			.setParameters("bias");

		builder.addNativeFunction("Renderer.enableDefaultPlane", lemon::wrap(&Renderer_enableDefaultPlane), defaultFlags)
			.setParameters("planeIndex", "enabled");

		builder.addNativeFunction("Renderer.setupPlane", lemon::wrap(&Renderer_setupPlane), defaultFlags)
			.setParameters("px", "py", "width", "height", "planeIndex", "scrollOffsets", "renderQueue");

		builder.addNativeFunction("Renderer.resetCustomPlaneConfigurations", lemon::wrap(&Renderer_resetCustomPlaneConfigurations), defaultFlags);

		builder.addNativeFunction("Renderer.resetSprites", lemon::wrap(&Renderer_resetSprites), defaultFlags);

		builder.addNativeFunction("Renderer.drawVdpSprite", lemon::wrap(&Renderer_drawVdpSprite), defaultFlags)
			.setParameters("px", "py", "encodedSize", "patternIndex", "renderQueue");

		builder.addNativeFunction("Renderer.drawVdpSpriteWithAlpha", lemon::wrap(&Renderer_drawVdpSpriteWithAlpha), defaultFlags)
			.setParameters("px", "py", "encodedSize", "patternIndex", "renderQueue", "alpha");

		builder.addNativeFunction("Renderer.drawVdpSpriteTinted", lemon::wrap(&Renderer_drawVdpSpriteTinted), defaultFlags)
			.setParameters("px", "py", "encodedSize", "patternIndex", "renderQueue", "tintColor", "addedColor");

		builder.addNativeFunction("Renderer.hasCustomSprite", lemon::wrap(&Renderer_hasCustomSprite), defaultFlags)
			.setParameters("key");

		builder.addNativeFunction("Renderer.setupCustomUncompressedSprite", lemon::wrap(&Renderer_setupCustomUncompressedSprite), defaultFlags)
			.setParameters("sourceBase", "word", "mappingOffset", "animationSprite", "atex");

		builder.addNativeFunction("Renderer.setupCustomCharacterSprite", lemon::wrap(&Renderer_setupCustomCharacterSprite), defaultFlags)
			.setParameters("sourceBase", "tableAddress", "mappingOffset", "animationSprite", "atex");

		builder.addNativeFunction("Renderer.setupCustomObjectSprite", lemon::wrap(&Renderer_setupCustomObjectSprite), defaultFlags)
			.setParameters("sourceBase", "tableAddress", "mappingOffset", "animationSprite", "atex");

		builder.addNativeFunction("Renderer.setupKosinskiCompressedSprite", lemon::wrap(&Renderer_setupKosinskiCompressedSprite1), defaultFlags)
			.setParameters("sourceBase", "mappingOffset", "animationSprite", "atex");

		builder.addNativeFunction("Renderer.setupKosinskiCompressedSprite", lemon::wrap(&Renderer_setupKosinskiCompressedSprite2), defaultFlags)
			.setParameters("sourceBase", "mappingOffset", "animationSprite", "atex", "indexOffset");

		builder.addNativeFunction("Renderer.drawSprite", lemon::wrap(&Renderer_drawSprite1), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue");

		builder.addNativeFunction("Renderer.drawSprite", lemon::wrap(&Renderer_drawSprite2), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "angle", "alpha");

		builder.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "angle", "tintColor", "scale");

		builder.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted2), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "angle", "tintColor", "scale");

		builder.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted3), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "angle", "tintColor", "scaleX", "scaleY");

		builder.addNativeFunction("Renderer.drawSpriteTinted", lemon::wrap(&Renderer_drawSpriteTinted4), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "angle", "tintColor", "scaleX", "scaleY");

		builder.addNativeFunction("Renderer.drawSpriteTransformed", lemon::wrap(&Renderer_drawSpriteTransformed), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "tintColor", "transform11", "transform12", "transform21", "transform22");

		builder.addNativeFunction("Renderer.drawSpriteTransformed", lemon::wrap(&Renderer_drawSpriteTransformed2), defaultFlags)
			.setParameters("key", "px", "py", "atex", "flags", "renderQueue", "tintColor", "transform11", "transform12", "transform21", "transform22");

		builder.addNativeFunction("Renderer.extractCustomSprite", lemon::wrap(&Renderer_extractCustomSprite), defaultFlags)
			.setParameters("key", "categoryName", "spriteNumber", "atex");

		builder.addNativeFunction("Renderer.addSpriteMask", lemon::wrap(&Renderer_addSpriteMask), defaultFlags)
			.setParameters("px", "py", "width", "height", "renderQueue", "priorityFlag");

		builder.addNativeFunction("Renderer.addSpriteMaskWorld", lemon::wrap(&Renderer_addSpriteMaskWorld), defaultFlags)
			.setParameters("px", "py", "width", "height", "renderQueue", "priorityFlag");

		builder.addNativeFunction("Renderer.setLogicalSpriteSpace", lemon::wrap(&Renderer_setLogicalSpriteSpace), defaultFlags)
			.setParameters("space");

		builder.addNativeFunction("Renderer.clearSpriteTag", lemon::wrap(&Renderer_clearSpriteTag), defaultFlags);

		builder.addNativeFunction("Renderer.setSpriteTagWithPosition", lemon::wrap(&Renderer_setSpriteTagWithPosition), defaultFlags)
			.setParameters("spriteTag", "px", "py");

		builder.addNativeFunction("Renderer.drawRect", lemon::wrap(&Renderer_drawRect), defaultFlags)
			.setParameters("px", "py", "width", "height", "color", "renderQueue", "useWorldSpace");

		builder.addNativeFunction("Renderer.drawRect", lemon::wrap(&Renderer_drawRect2), defaultFlags)
			.setParameters("px", "py", "width", "height", "color", "renderQueue", "useWorldSpace", "useGlobalComponentTint");

		builder.addNativeFunction("Renderer.setScreenSize", lemon::wrap(&Renderer_setScreenSize), defaultFlags)
			.setParameters("width", "height");

		builder.addNativeFunction("Renderer.resetViewport", lemon::wrap(&Renderer_resetViewport), defaultFlags)
			.setParameters("renderQueue");

		builder.addNativeFunction("Renderer.setViewport", lemon::wrap(&Renderer_setViewport), defaultFlags)
			.setParameters("px", "py", "width", "height", "renderQueue");

		builder.addNativeFunction("Renderer.setGlobalComponentTint", lemon::wrap(&Renderer_setGlobalComponentTint), defaultFlags)
			.setParameters("tintR", "tintG", "tintB", "addedR", "addedG", "addedB");

		builder.addNativeFunction("Renderer.drawText", lemon::wrap(&Renderer_drawText2), defaultFlags)
			.setParameters("fontKey", "px", "py", "text", "tintColor", "alignment", "spacing", "renderQueue", "useWorldSpace");

		builder.addNativeFunction("Renderer.drawText", lemon::wrap(&Renderer_drawText), defaultFlags)
			.setParameters("fontKey", "px", "py", "text", "tintColor", "alignment", "spacing", "renderQueue", "useWorldSpace", "useGlobalComponentTint");

		builder.addNativeFunction("Renderer.getTextWidth", lemon::wrap(&Renderer_getTextWidth), defaultFlags)
			.setParameters("fontKey", "text");

		builder.addNativeFunction("setWorldSpaceOffset", lemon::wrap(&setWorldSpaceOffset), defaultFlags)
			.setParameters("px", "py");


		// Sprite handle
		builder.addNativeFunction("Renderer.addSpriteHandle", lemon::wrap(&Renderer_addSpriteHandle), defaultFlags)
			.setParameters("spriteKey", "px", "py", "renderQueue");

		builder.addNativeMethod("SpriteHandle", "setFlags", lemon::wrap(&SpriteHandle_setFlags), defaultFlags)
			.setParameters("this", "flags");

		builder.addNativeMethod("SpriteHandle", "setFlipX", lemon::wrap(&SpriteHandle_setFlipX), defaultFlags)
			.setParameters("this", "flipX");

		builder.addNativeMethod("SpriteHandle", "setFlipY", lemon::wrap(&SpriteHandle_setFlipY), defaultFlags)
			.setParameters("this", "flipY");

		builder.addNativeMethod("SpriteHandle", "setRotation", lemon::wrap(&SpriteHandle_setRotation), defaultFlags)
			.setParameters("this", "degrees");

		builder.addNativeMethod("SpriteHandle", "setRotationRadians", lemon::wrap(&SpriteHandle_setRotationRadians), defaultFlags)
			.setParameters("this", "radians");

		builder.addNativeMethod("SpriteHandle", "setRotation_u8", lemon::wrap(&SpriteHandle_setRotation_u8), defaultFlags)
			.setParameters("this", "angle");

		builder.addNativeMethod("SpriteHandle", "setScale", lemon::wrap(&SpriteHandle_setScaleUniform), defaultFlags)
			.setParameters("this", "scale");

		builder.addNativeMethod("SpriteHandle", "setScale", lemon::wrap(&SpriteHandle_setScaleXY), defaultFlags)
			.setParameters("this", "scaleX", "scaleY");

		builder.addNativeMethod("SpriteHandle", "setScale_s32", lemon::wrap(&SpriteHandle_setScaleUniform_s32), defaultFlags)
			.setParameters("this", "scale");

		builder.addNativeMethod("SpriteHandle", "setScale_s32", lemon::wrap(&SpriteHandle_setScaleXY_s32), defaultFlags)
			.setParameters("this", "scaleX", "scaleY");

		builder.addNativeMethod("SpriteHandle", "setRotationScale", lemon::wrap(&SpriteHandle_setRotationScale1), defaultFlags)
			.setParameters("this", "degrees", "scale");

		builder.addNativeMethod("SpriteHandle", "setRotationScale", lemon::wrap(&SpriteHandle_setRotationScale2), defaultFlags)
			.setParameters("this", "degrees", "scaleX", "scaleY");

		builder.addNativeMethod("SpriteHandle", "setTransform", lemon::wrap(&SpriteHandle_setTransform), defaultFlags)
			.setParameters("this", "transform11", "transform12", "transform21", "transform22");

		builder.addNativeMethod("SpriteHandle", "setPriorityFlag", lemon::wrap(&SpriteHandle_setPriorityFlag), defaultFlags)
			.setParameters("this", "priorityFlag");

		builder.addNativeMethod("SpriteHandle", "setCoordinateSpace", lemon::wrap(&SpriteHandle_setCoordinateSpace), defaultFlags)
			.setParameters("this", "space");

		builder.addNativeMethod("SpriteHandle", "setUseGlobalComponentTint", lemon::wrap(&SpriteHandle_setUseGlobalComponentTint), defaultFlags)
			.setParameters("this", "enable");

		builder.addNativeMethod("SpriteHandle", "setBlendMode", lemon::wrap(&SpriteHandle_setBlendMode), defaultFlags)
			.setParameters("this", "blendMode");

		builder.addNativeMethod("SpriteHandle", "setPaletteOffset", lemon::wrap(&SpriteHandle_setPaletteOffset), defaultFlags)
			.setParameters("this", "paletteOffset");

		builder.addNativeMethod("SpriteHandle", "setTintColor", lemon::wrap(&SpriteHandle_setTintColor), defaultFlags)
			.setParameters("this", "red", "green", "blue", "alpha");

		builder.addNativeMethod("SpriteHandle", "setTintColor_u8", lemon::wrap(&SpriteHandle_setTintColor_u8), defaultFlags)
			.setParameters("this", "red", "green", "blue", "alpha");

		builder.addNativeMethod("SpriteHandle", "setTintColorRGBA", lemon::wrap(&SpriteHandle_setTintColorRGBA), defaultFlags)
			.setParameters("this", "rgba");

		builder.addNativeMethod("SpriteHandle", "setOpacity", lemon::wrap(&SpriteHandle_setOpacity), defaultFlags)
			.setParameters("this", "opacity");

		builder.addNativeMethod("SpriteHandle", "setAddedColor", lemon::wrap(&SpriteHandle_setAddedColor), defaultFlags)
			.setParameters("this", "red", "green", "blue");

		builder.addNativeMethod("SpriteHandle", "setAddedColor_u8", lemon::wrap(&SpriteHandle_setAddedColor_u8), defaultFlags)
			.setParameters("this", "red", "green", "blue");

		builder.addNativeMethod("SpriteHandle", "setAddedColorRGB", lemon::wrap(&SpriteHandle_setAddedColorRGB), defaultFlags)
			.setParameters("this", "rgb");

		builder.addNativeMethod("SpriteHandle", "setSpriteTag", lemon::wrap(&SpriteHandle_setSpriteTag), defaultFlags)
			.setParameters("this", "spriteTag", "px", "py");
	}
}
