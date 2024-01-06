/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/InputManager.h"


class DynamicSprites
{
public:
	static const uint64 INPUT_ICON_BUTTON_UP;
	static const uint64 INPUT_ICON_BUTTON_DOWN;
	static const uint64 INPUT_ICON_BUTTON_LEFT;
	static const uint64 INPUT_ICON_BUTTON_RIGHT;
	static const uint64 INPUT_ICON_BUTTON_A;
	static const uint64 INPUT_ICON_BUTTON_B;
	static const uint64 INPUT_ICON_BUTTON_X;
	static const uint64 INPUT_ICON_BUTTON_Y;
	static const uint64 INPUT_ICON_BUTTON_START;
	static const uint64 INPUT_ICON_BUTTON_BACK;
	static const uint64 INPUT_ICON_BUTTON_L;
	static const uint64 INPUT_ICON_BUTTON_R;

	struct GamepadStyle
	{
		uint64 mSpriteKeys[12];
		explicit GamepadStyle(const std::string& identifier);
	};
	static const GamepadStyle GAMEPAD_STYLES[3];

public:
	static uint64 getGamepadSpriteKey(size_t controlIndex);
	static uint64 getGamepadSpriteKey(size_t controlIndex, size_t style);

public:
	void updateSpriteRedirects();

private:
	InputManager::InputType mLastInputType = InputManager::InputType::NONE;
	int mLastGamepadVisualStyle = 0;
	uint32 mLastMappingsChangeCounter = 0;
	uint32 mLastSpriteCacheChangeCounter = 0;
};
