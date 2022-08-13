/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/menu/GameMenuBase.h"


namespace option
{
	enum Option
	{
		_TAB_SELECTION,

		// Display
		WINDOW_MODE,
		WINDOW_MODE_STARTUP,
		RENDERER,
		FRAME_SYNC,
		UPSCALING,
		BACKDROP,
		FILTERING,
		SCANLINES,
		BG_BLUR,
		PERFORMANCE_DISPLAY,

		// Audio
		AUDIO_VOLUME,
		MUSIC_VOLUME,
		SOUND_VOLUME,
		SOUNDTRACK,
		SOUND_TEST,
		TITLE_THEME,
		EXTRA_LIFE_JINGLE,
		INVINCIBILITY_THEME,
		SUPER_THEME,
		MINIBOSS_THEME,
		KNUCKLES_THEME,
		LEVELMUSIC_CNZ1,
		LEVELMUSIC_CNZ2,
		LEVELMUSIC_ICZ1,
		LEVELMUSIC_ICZ2,
		LEVELMUSIC_LBZ1,
		LEVELMUSIC_LBZ2,
		HPZ_MUSIC,
		SSZ_BOSSTRACKS,
		FBZ2_MIDBOSS_TRACK,
		OUTRO_MUSIC,
		COMPETITION_MENU_MUSIC,
		CONTINUE_SCREEN_MUSIC,
		CONTINUE_MUSIC,
		UNDERWATER_AUDIO,

		// Visuals
		ROTATION,
		TIME_DISPLAY,
		LIVES_DISPLAY,
		SPEEDUP_AFTER_IMAGES,
		FAST_RUN_ANIM,
		ANTI_FLICKER,
		CAMERA_OUTRUN,
		EXTENDED_CAMERA,
		ICZ_NIGHTTIME,
		MONITOR_STYLE,
		SPECIAL_STAGE_VISUALS,
		SPECIAL_STAGE_RING_COUNT,

		// Gameplay
		LEVEL_LAYOUTS,
		AIZ_BLIMPSEQUENCE,
		LBZ_BIGARMS,
		SOZ_GHOSTSPAWN,
		LRZ2_BOSS,
		MAINTAIN_SHIELDS,
		TIMEATTACK_GHOSTS,
		TIMEATTACK_INSTANTRESTART,

		// Controls
		CONTROLLER_PLAYER_1,
		CONTROLLER_PLAYER_2,
		CONTROLLER_AUTOASSIGN,
		CONTROLLER_SETUP,
		DROP_DASH,
		SUPER_PEELOUT,
		SUPER_CANCEL,
		INSTA_SHIELD,
		HYPER_TAILS,
		HYPER_DASH_CONTROLS,
		SUPER_SONIC_ABILITY,
		MONITOR_BEHAVIOR,
		TAILS_ASSIST,
		TAILS_FLIGHT_CANCEL,
		NO_CONTROL_LOCK,
		VGAMEPAD_OPACITY,
		VGAMEPAD_SETUP,
		VGAMEPAD_DPAD_SIZE,
		VGAMEPAD_BUTTONS_SIZE,

		// Tweaks
		DEBUG_MODE,
		TITLE_SCREEN,
		GAME_SPEED,
		INFINITE_LIVES,
		INFINITE_TIME,
		SHIELD_TYPES,
		BUBBLE_SHIELD_BOUNCE,
		RANDOM_MONITORS,
		RANDOM_SPECIALSTAGES,
		SPECIAL_STAGE_REPEAT,
		REGION,

		_CHECK_FOR_UPDATE,
		RELEASE_CHANNEL,
		_OPEN_HOMEPAGE,
		_OPEN_MANUAL,
		_BACK,
		_NUM
	};
}


class OptionEntry
{
public:
	enum class Type
	{
		UNDEFINED = 0,		// Undefined type
		SETTING,			// Bound to a setting in SharedDatabase
		SETTING_BITMASK,	// Same as above, but treat values as bitmasks
		CONFIG_INT,			// Bound to an int value
		CONFIG_ENUM_8,		// Bound to an enum value with uint8 size
		CONFIG_PERCENT,		// Bound to a float value in 0.0f...1.0f represented by percent values 0...100 in the game menu entry
		MOD_SETTING			// Bound to a mod setting
	};

public:
	void loadValue();
	void applyValue();

public:
	option::Option mOptionId = option::_NUM;
	GameMenuEntry* mGameMenuEntry = nullptr;

	Type mType = Type::UNDEFINED;
	SharedDatabase::Setting::Type mSetting = SharedDatabase::Setting::INVALID;
	void* mValuePointer = nullptr;
};
