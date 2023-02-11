/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/helper/GameUtils.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/rendering/parts/PlaneManager.h"
#include "oxygen/simulation/EmulatorInterface.h"


namespace s3air
{

	void changePlanePatternRectAtex(EmulatorInterface& emulatorInterface, uint16 px, uint16 py, uint16 width, uint16 height, uint8 planeIndex, uint8 atex)
	{
		PlaneManager& planeManager = VideoOut::instance().getRenderParts().getPlaneManager();

		uint16 minX = px / 8;
		uint16 maxX = (px + width + 7) / 8;
		uint16 minY = py / 8;
		uint16 maxY = (py + height + 7) / 8;

		const uint16 cameraX = emulatorInterface.readMemory16(0xffffee80) / 8;
		const uint16 cameraY = emulatorInterface.readMemory16(0xffffee84) / 8;

		minX = clamp(minX, cameraX, cameraX + 0x3f);
		maxX = clamp(maxX, cameraX, cameraX + 0x3f);
		minY = clamp(minY, cameraY, cameraY + 0x1f);
		maxY = clamp(maxY, cameraY, cameraY + 0x1f);

		for (uint16 y = minY; y < maxY; ++y)
		{
			for (uint16 x = minX; x < maxX; ++x)
			{
				const uint16 patternIndex = (x & 0x3f) + (y & 0x1f) * 0x40;

				uint16 pattern = planeManager.getPatternAtIndex(planeIndex, patternIndex);
				pattern = (pattern & 0x1fff) | ((uint16)(atex & 0x70) << 9);
				planeManager.setPatternAtIndex(planeIndex, patternIndex, pattern);
			}
		}
	}

	void drawPlayerSprite(EmulatorInterface& emulatorInterface, uint8 characterIndex, const Vec2i& position, float moveDirectionRadians, uint16 animationSprite, uint8 flags, uint8 rotation, const Color& color, const uint16* globalFrameNumber, bool enableOffscreen, uint64 spriteTagBaseValue)
	{
		const uint8 atex = 0x40 + characterIndex * 0x20;
		int px = position.x;
		int py = position.y;
		float angle = (float)rotation / 128.0f * PI_FLOAT;

		// Take level wrap into account
		if (emulatorInterface.readMemory16(0xffffee18) != 0)
		{
			const int levelHeight = emulatorInterface.readMemory16(0xffffeeaa) + 1;
			if (py > levelHeight / 2)
				py -= levelHeight;
			else if (py <= -levelHeight / 2)
				py += levelHeight;
		}

		bool showAtBorder = false;
		if (enableOffscreen)
		{
			const int width = VideoOut::instance().getScreenWidth();
			const int height = VideoOut::instance().getScreenHeight();
			float rx = (float)px / (float)(width-1) * 2.0f - 1.0f;
			float ry = (float)py / (float)(height-1) * 2.0f - 1.0f;

			float scale;
			if (std::abs(rx) > std::abs(ry))
			{
				scale = std::abs(rx) / 0.95f;
			}
			else
			{
				scale = std::abs(ry) / 0.95f;
			}

			if (scale > 1.0f)
			{
				showAtBorder = true;
				px = roundToInt((1.0f + rx / scale) / 2.0f * (width-1));
				py = roundToInt((1.0f + ry / scale) / 2.0f * (height-1));
			}
		}

		// Get character sprite key
		uint64 key = 0;
		{
			if (characterIndex == 0)
			{
				if (animationSprite >= 0x102)
				{
					key = rmx::getMurmur2_64(String(0, "sonic_peelout_%d", animationSprite - 0x102));
				}
				else if (animationSprite >= 0x100)
				{
					key = rmx::getMurmur2_64(String(0, "sonic_dropdash_%d", animationSprite - 0x100));
				}
				else
				{
					key = rmx::getMurmur2_64(String(0, "character_sonic_0x%02x", animationSprite));
				}
			}
			else if (characterIndex == 1)
			{
				key = rmx::getMurmur2_64(String(0, "character_tails_0x%02x", animationSprite));
			}
			else if (characterIndex == 2)
			{
				key = rmx::getMurmur2_64(String(0, "character_knuckles_0x%02x", animationSprite));
			}

			if (!SpriteCache::instance().hasSprite(key))
			{
				key = SharedDatabase::setupCharacterSprite(characterIndex, animationSprite, false);
				if (!SpriteCache::instance().hasSprite(key))
					key = 0;
			}
		}
		SpriteManager& spriteManager = VideoOut::instance().getRenderParts().getSpriteManager();
		const Vec2i renderPos(px, py);

		// Tails' tails sprite
		if (characterIndex == 1)
		{
			if (animationSprite >= 0x96 && animationSprite <= 0x98)
			{
				// For rolling, use the movement angle as tails angle
				angle = moveDirectionRadians;
				flags = 0;
			}

			uint8 tailsAnimSprite;
			if (nullptr == globalFrameNumber)
			{
				tailsAnimSprite = emulatorInterface.readMemory8(0xffffcc2c);
			}
			else
			{
				tailsAnimSprite = SharedDatabase::getTailsTailsAnimationSprite((uint8)animationSprite, *globalFrameNumber);
			}

			uint64 key = rmx::getMurmur2_64(String(0, "character_tails_tails_0x%02x", tailsAnimSprite));
			if (!SpriteCache::instance().hasSprite(key))
			{
				key = SharedDatabase::setupTailsTailsSprite(tailsAnimSprite);
				if (!SpriteCache::instance().hasSprite(key))
					key = 0;
			}

			if (key != 0)
			{
				spriteManager.setSpriteTagWithPosition(spriteTagBaseValue + 1, renderPos);
				if (showAtBorder)
					spriteManager.drawCustomSprite(key, renderPos, atex, flags | 0x40, 0xe000, color, angle, 0.4f);
				else
					spriteManager.drawCustomSprite(key, renderPos, atex, flags, 0x9eff, color, angle);
			}
		}

		// Main sprite
		if (key != 0)
		{
			spriteManager.setSpriteTagWithPosition(spriteTagBaseValue, renderPos);
			if (showAtBorder)
				spriteManager.drawCustomSprite(key, renderPos, atex, flags | 0x40, 0xe000, color, 0.0f, 0.4f);
			else
				spriteManager.drawCustomSprite(key, renderPos, atex, flags, 0x9eff, color);
		}
	}

	void drawPlayerSprite(EmulatorInterface& emulatorInterface, uint8 characterIndex, const Vec2i& position, const Vec2i& velocity, uint16 animationSprite, uint8 flags, uint8 rotation, const Color& color, const uint16* globalFrameNumber, bool enableOffscreen, uint64 spriteTagBaseValue)
	{
		drawPlayerSprite(emulatorInterface, characterIndex, position, std::atan2((float)velocity.y, (float)velocity.x), animationSprite, flags, rotation, color, globalFrameNumber, enableOffscreen, spriteTagBaseValue);
	}

}
