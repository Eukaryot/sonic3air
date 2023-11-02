/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/resources/DynamicSprites.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen/resources/SpriteCache.h"


const uint64 DynamicSprites::INPUT_ICON_BUTTON_UP    = rmx::getMurmur2_64("@input_icon_button_up");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_DOWN  = rmx::getMurmur2_64("@input_icon_button_down");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_LEFT  = rmx::getMurmur2_64("@input_icon_button_left");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_RIGHT = rmx::getMurmur2_64("@input_icon_button_right");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_A     = rmx::getMurmur2_64("@input_icon_button_A");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_B     = rmx::getMurmur2_64("@input_icon_button_B");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_X     = rmx::getMurmur2_64("@input_icon_button_X");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_Y     = rmx::getMurmur2_64("@input_icon_button_Y");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_START = rmx::getMurmur2_64("@input_icon_button_start");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_BACK  = rmx::getMurmur2_64("@input_icon_button_back");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_L     = rmx::getMurmur2_64("@input_icon_button_L");
const uint64 DynamicSprites::INPUT_ICON_BUTTON_R     = rmx::getMurmur2_64("@input_icon_button_R");

DynamicSprites::GamepadStyle::GamepadStyle(const std::string& identifier)
{
	mSpriteKeys[0]  = rmx::getMurmur2_64("input_icon_" + identifier + "_up");
	mSpriteKeys[1]  = rmx::getMurmur2_64("input_icon_" + identifier + "_down");
	mSpriteKeys[2]  = rmx::getMurmur2_64("input_icon_" + identifier + "_left");
	mSpriteKeys[3]  = rmx::getMurmur2_64("input_icon_" + identifier + "_right");
	mSpriteKeys[4]  = rmx::getMurmur2_64("input_icon_" + identifier + "_A");
	mSpriteKeys[5]  = rmx::getMurmur2_64("input_icon_" + identifier + "_B");
	mSpriteKeys[6]  = rmx::getMurmur2_64("input_icon_" + identifier + "_X");
	mSpriteKeys[7]  = rmx::getMurmur2_64("input_icon_" + identifier + "_Y");
	mSpriteKeys[8]  = rmx::getMurmur2_64("input_icon_" + identifier + "_start");
	mSpriteKeys[9]  = rmx::getMurmur2_64("input_icon_" + identifier + "_back");
	mSpriteKeys[10] = rmx::getMurmur2_64("input_icon_" + identifier + "_L");
	mSpriteKeys[11] = rmx::getMurmur2_64("input_icon_" + identifier + "_R");
}

const DynamicSprites::GamepadStyle DynamicSprites::GAMEPAD_STYLES[3] = { GamepadStyle("xbox"), GamepadStyle("ps"), GamepadStyle("switch") };


namespace
{
	uint64 getKeyboardIconSpriteKey(const InputManager::Control& control)
	{
		#define RETURN(key) \
			{ \
				static uint64 SPRITE_KEY = rmx::getMurmur2_64(key); \
				return SPRITE_KEY; \
			}

		for (const InputManager::ControlInput& controlInput : control.mInputs)
		{
			if (controlInput.mDevice->mType == InputConfig::DeviceType::KEYBOARD)
			{
				switch (controlInput.mIndex)
				{
					case 'a':  RETURN("input_icon_key_A");
					case 'b':  RETURN("input_icon_key_B");
					case 'c':  RETURN("input_icon_key_C");
					case 'd':  RETURN("input_icon_key_D");
					case 'e':  RETURN("input_icon_key_E");
					case 'f':  RETURN("input_icon_key_F");
					case 'g':  RETURN("input_icon_key_G");
					case 'h':  RETURN("input_icon_key_H");
					case 'i':  RETURN("input_icon_key_I");
					case 'j':  RETURN("input_icon_key_J");
					case 'k':  RETURN("input_icon_key_K");
					case 'l':  RETURN("input_icon_key_L");
					case 'm':  RETURN("input_icon_key_M");
					case 'n':  RETURN("input_icon_key_N");
					case 'o':  RETURN("input_icon_key_O");
					case 'p':  RETURN("input_icon_key_P");
					case 'q':  RETURN("input_icon_key_Q");
					case 'r':  RETURN("input_icon_key_R");
					case 's':  RETURN("input_icon_key_S");
					case 't':  RETURN("input_icon_key_T");
					case 'u':  RETURN("input_icon_key_U");
					case 'v':  RETURN("input_icon_key_V");
					case 'w':  RETURN("input_icon_key_W");
					case 'x':  RETURN("input_icon_key_X");
					case 'y':  RETURN("input_icon_key_Y");
					case 'z':  RETURN("input_icon_key_Z");

					case SDLK_TAB:		 RETURN("input_icon_key_tab");
					case SDLK_SPACE:	 RETURN("input_icon_key_space");
					case SDLK_RETURN:	 RETURN("input_icon_key_enter");
					case SDLK_BACKSPACE: RETURN("input_icon_key_back");
					case SDLK_ESCAPE:	 RETURN("input_icon_key_esc");
					case SDLK_LEFT:		 RETURN("input_icon_key_left");
					case SDLK_RIGHT:	 RETURN("input_icon_key_right");
					case SDLK_UP:		 RETURN("input_icon_key_up");
					case SDLK_DOWN:		 RETURN("input_icon_key_down");

					case '0':  RETURN("input_icon_key_0");
					case '1':  RETURN("input_icon_key_1");
					case '2':  RETURN("input_icon_key_2");
					case '3':  RETURN("input_icon_key_3");
					case '4':  RETURN("input_icon_key_4");
					case '5':  RETURN("input_icon_key_5");
					case '6':  RETURN("input_icon_key_6");
					case '7':  RETURN("input_icon_key_7");
					case '8':  RETURN("input_icon_key_8");
					case '9':  RETURN("input_icon_key_9");
				}
			}
		}

		// Fallback
		RETURN("input_icon_key_blank");
	}
}


uint64 DynamicSprites::getGamepadSpriteKey(size_t buttonIndex)
{
	return getGamepadSpriteKey(buttonIndex, ConfigurationImpl::instance().mGamepadVisualStyle);
}

uint64 DynamicSprites::getGamepadSpriteKey(size_t buttonIndex, size_t style)
{
	style = std::min<size_t>(style, 2);
	if (buttonIndex < 12)
		return GAMEPAD_STYLES[style].mSpriteKeys[buttonIndex];
	return 0;
}

void DynamicSprites::updateSpriteRedirects()
{
	InputManager& inputManager = InputManager::instance();
	const InputManager::InputType lastInputType = inputManager.getLastInputType();
	const int gamepadVisualStyle = clamp(ConfigurationImpl::instance().mGamepadVisualStyle, 0, 2);
	SpriteCache& spriteCache = SpriteCache::instance();
	if (mLastInputType == lastInputType && mLastGamepadVisualStyle == gamepadVisualStyle &&
		mLastMappingsChangeCounter == inputManager.getMappingsChangeCounter() && mLastSpriteCacheChangeCounter == spriteCache.getGlobalChangeCounter())
		return;

	mLastInputType = lastInputType;
	mLastGamepadVisualStyle = gamepadVisualStyle;
	mLastMappingsChangeCounter = inputManager.getMappingsChangeCounter();
	mLastSpriteCacheChangeCounter = spriteCache.getGlobalChangeCounter();

	const InputManager::ControllerScheme& keys = inputManager.getController(0);

	switch (mLastInputType)
	{
		case InputManager::InputType::KEYBOARD:
		{
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_UP,    getKeyboardIconSpriteKey(keys.Up));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_DOWN,  getKeyboardIconSpriteKey(keys.Down));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_LEFT,  getKeyboardIconSpriteKey(keys.Left));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_RIGHT, getKeyboardIconSpriteKey(keys.Right));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_A,     getKeyboardIconSpriteKey(keys.A));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_B,     getKeyboardIconSpriteKey(keys.B));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_X,     getKeyboardIconSpriteKey(keys.X));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_Y,     getKeyboardIconSpriteKey(keys.Y));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_START, getKeyboardIconSpriteKey(keys.Start));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_BACK,  getKeyboardIconSpriteKey(keys.Back));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_L,     getKeyboardIconSpriteKey(keys.L));
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_R,     getKeyboardIconSpriteKey(keys.R));
			break;
		}

		case InputManager::InputType::TOUCH:
		{
			static const uint64 INPUT_ICON_TOUCH_UP    = rmx::getMurmur2_64("input_icon_touch_up");
			static const uint64 INPUT_ICON_TOUCH_DOWN  = rmx::getMurmur2_64("input_icon_touch_down");
			static const uint64 INPUT_ICON_TOUCH_LEFT  = rmx::getMurmur2_64("input_icon_touch_left");
			static const uint64 INPUT_ICON_TOUCH_RIGHT = rmx::getMurmur2_64("input_icon_touch_right");
			static const uint64 INPUT_ICON_TOUCH_A     = rmx::getMurmur2_64("input_icon_touch_A");
			static const uint64 INPUT_ICON_TOUCH_B     = rmx::getMurmur2_64("input_icon_touch_B");
			static const uint64 INPUT_ICON_TOUCH_X     = rmx::getMurmur2_64("input_icon_touch_X");
			static const uint64 INPUT_ICON_TOUCH_Y     = rmx::getMurmur2_64("input_icon_touch_Y");
			static const uint64 INPUT_ICON_TOUCH_START = rmx::getMurmur2_64("input_icon_touch_start");
			static const uint64 INPUT_ICON_TOUCH_BACK  = rmx::getMurmur2_64("input_icon_touch_back");
			static const uint64 INPUT_ICON_TOUCH_L     = rmx::getMurmur2_64("input_icon_touch_L");
			static const uint64 INPUT_ICON_TOUCH_R     = rmx::getMurmur2_64("input_icon_touch_R");

			spriteCache.setupRedirect(INPUT_ICON_BUTTON_UP,    INPUT_ICON_TOUCH_UP);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_DOWN,  INPUT_ICON_TOUCH_DOWN);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_LEFT,  INPUT_ICON_TOUCH_LEFT);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_RIGHT, INPUT_ICON_TOUCH_RIGHT);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_A,     INPUT_ICON_TOUCH_A);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_B,     INPUT_ICON_TOUCH_B);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_X,     INPUT_ICON_TOUCH_X);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_Y,     INPUT_ICON_TOUCH_Y);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_START, INPUT_ICON_TOUCH_START);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_BACK,  INPUT_ICON_TOUCH_BACK);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_L,     INPUT_ICON_TOUCH_L);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_R,     INPUT_ICON_TOUCH_R);
			break;
		}

		default:
		case InputManager::InputType::GAMEPAD:
		{
			const size_t style = clamp(gamepadVisualStyle, 0, 2);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_UP,    GAMEPAD_STYLES[style].mSpriteKeys[0]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_DOWN,  GAMEPAD_STYLES[style].mSpriteKeys[1]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_LEFT,  GAMEPAD_STYLES[style].mSpriteKeys[2]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_RIGHT, GAMEPAD_STYLES[style].mSpriteKeys[3]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_A,     GAMEPAD_STYLES[style].mSpriteKeys[4]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_B,     GAMEPAD_STYLES[style].mSpriteKeys[5]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_X,     GAMEPAD_STYLES[style].mSpriteKeys[6]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_Y,     GAMEPAD_STYLES[style].mSpriteKeys[7]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_START, GAMEPAD_STYLES[style].mSpriteKeys[8]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_BACK,  GAMEPAD_STYLES[style].mSpriteKeys[9]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_L,     GAMEPAD_STYLES[style].mSpriteKeys[10]);
			spriteCache.setupRedirect(INPUT_ICON_BUTTON_R,     GAMEPAD_STYLES[style].mSpriteKeys[11]);
			break;
		}
	}
}
