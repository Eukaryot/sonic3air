/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class EmulatorInterface;


namespace s3air
{
	void changePlanePatternRectAtex(EmulatorInterface& emulatorInterface, uint16 px, uint16 py, uint16 width, uint16 height, uint8 planeIndex, uint8 atex);

	void drawPlayerSprite(EmulatorInterface& emulatorInterface, uint8 characterIndex, const Vec2i& position, float moveDirectionRadians, uint16 animationSprite, uint8 flags, uint8 rotation, Color color, const uint16* globalFrameNumber = nullptr, Color offscreenColor = Color::TRANSPARENT, uint64 spriteTagBaseValue = 0);
	void drawPlayerSprite(EmulatorInterface& emulatorInterface, uint8 characterIndex, const Vec2i& position, const Vec2i& velocity, uint16 animationSprite, uint8 flags, uint8 rotation, Color color, const uint16* globalFrameNumber = nullptr, Color offscreenColor = Color::TRANSPARENT, uint64 spriteTagBaseValue = 0);
}
