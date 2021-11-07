/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/options/OptionsMenu.h"
#include "sonic3air/menu/options/ControllerSetupMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/Game.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/overlays/TouchControlsOverlay.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/helper/Utils.h"


namespace option
{
	static const std::string TEXT_NOT_AVAILABLE = "not available";

	static const std::vector<std::string> HINT_TEXTS =
	{
		"Have a look at the ", "#Manual", " for an overview",
		"$of ", "#controller setup", ", ", "#new character moves",
		"$and ", "#graphics troubleshooting"
	};
}


OptionsMenu::OptionsMenu(MenuBackground& menuBackground) :
	mMenuBackground(&menuBackground)
{
	mScrolling.setVisibleAreaHeight(224 - 30);	// Do not count the 30 pixels of the tab title as scrolling area

	mOptionEntries.resize(option::_NUM);
	{
		ConfigurationImpl& config = ConfigurationImpl::instance();

		setupOptionEntryInt(option::FRAME_SYNC,					&config.mFrameSync);
		setupOptionEntryInt(option::UPSCALING,					&config.mUpscaling);
		setupOptionEntryInt(option::BACKDROP,					&config.mBackdrop);
		setupOptionEntryInt(option::FILTERING,					&config.mFiltering);
		setupOptionEntryInt(option::SCANLINES,					&config.mScanlines);
		setupOptionEntryInt(option::BG_BLUR,					&config.mBackgroundBlur);
		setupOptionEntryInt(option::PERFORMANCE_DISPLAY,		&config.mPerformanceDisplay);
		setupOptionEntryInt(option::SOUNDTRACK,					&config.mActiveSoundtrack);
		setupOptionEntryInt(option::CONTROLLER_AUTOASSIGN,		&config.mAutoAssignGamepadPlayerIndex);
		setupOptionEntryInt(option::VGAMEPAD_DPAD_SIZE,			&config.mVirtualGamepad.mDirectionalPadSize);
		setupOptionEntryInt(option::VGAMEPAD_BUTTONS_SIZE,		&config.mVirtualGamepad.mFaceButtonsSize);
		setupOptionEntryInt(option::TIMEATTACK_INSTANTRESTART,	&config.mInstantTimeAttackRestart);
		setupOptionEntryInt(option::GAME_SPEED,					&config.mSimulationFrequency);

		setupOptionEntryPercent(option::AUDIO_VOLUME,			&config.mAudioVolume);
		setupOptionEntryPercent(option::MUSIC_VOLUME,			&config.mMusicVolume);
		setupOptionEntryPercent(option::SOUND_VOLUME,			&config.mSoundVolume);
		setupOptionEntryPercent(option::VGAMEPAD_OPACITY,		&config.mVirtualGamepad.mOpacity);

		setupOptionEntry(option::ROTATION,					SharedDatabase::Setting::SETTING_SMOOTH_ROTATION);
		setupOptionEntry(option::SPEEDUP_AFTER_IMAGES,		SharedDatabase::Setting::SETTING_SPEEDUP_AFTERIMGS);
		setupOptionEntry(option::FAST_RUN_ANIM,				SharedDatabase::Setting::SETTING_SUPERFAST_RUNANIM);
		setupOptionEntry(option::MONITOR_STYLE,				SharedDatabase::Setting::SETTING_MONITOR_STYLE);
		setupOptionEntry(option::TIME_DISPLAY,				SharedDatabase::Setting::SETTING_EXTENDED_HUD);
		setupOptionEntry(option::LIVES_DISPLAY,				SharedDatabase::Setting::SETTING_LIVES_DISPLAY);
		setupOptionEntry(option::SPECIAL_STAGE_VISUALS,		SharedDatabase::Setting::SETTING_BS_VISUAL_STYLE);
		setupOptionEntry(option::TAILS_ASSIST,				SharedDatabase::Setting::SETTING_TAILS_ASSIST_MODE);
		setupOptionEntry(option::TAILS_FLIGHT_CANCEL,		SharedDatabase::Setting::SETTING_CANCEL_FLIGHT);
		setupOptionEntry(option::NO_CONTROL_LOCK,			SharedDatabase::Setting::SETTING_NO_CONTROL_LOCK);
		setupOptionEntry(option::HYPER_TAILS,				SharedDatabase::Setting::SETTING_HYPER_TAILS);
		setupOptionEntry(option::SHIELD_TYPES,				SharedDatabase::Setting::SETTING_SHIELD_TYPES);
		setupOptionEntry(option::BUBBLE_SHIELD_BOUNCE,		SharedDatabase::Setting::SETTING_BUBBLE_SHIELD_BOUNCE);
		setupOptionEntry(option::SUPER_CANCEL,				SharedDatabase::Setting::SETTING_SUPER_CANCEL);
		setupOptionEntry(option::INSTA_SHIELD,				SharedDatabase::Setting::SETTING_INSTA_SHIELD);
		setupOptionEntry(option::LEVEL_LAYOUTS,				SharedDatabase::Setting::SETTING_LEVELLAYOUTS);
		setupOptionEntry(option::CAMERA_OUTRUN,				SharedDatabase::Setting::SETTING_CAMERA_OUTRUN);
		setupOptionEntry(option::EXTENDED_CAMERA,			SharedDatabase::Setting::SETTING_EXTENDED_CAMERA);
		setupOptionEntry(option::SPECIAL_STAGE_REPEAT,		SharedDatabase::Setting::SETTING_BS_REPEAT_ON_FAIL);
		setupOptionEntry(option::RANDOM_MONITORS,			SharedDatabase::Setting::SETTING_RANDOM_MONITORS);
		setupOptionEntry(option::RANDOM_SPECIALSTAGES,		SharedDatabase::Setting::SETTING_RANDOM_SPECIALSTAGES);
		setupOptionEntry(option::AIZ_BLIMPSEQUENCE,			SharedDatabase::Setting::SETTING_AIZ_BLIMPSEQUENCE);
		setupOptionEntry(option::LBZ_BIGARMS,				SharedDatabase::Setting::SETTING_LBZ_BIGARMS);
		setupOptionEntry(option::SOZ_GHOSTSPAWN,			SharedDatabase::Setting::SETTING_DISABLE_GHOST_SPAWN);
		setupOptionEntry(option::LRZ2_BOSS,					SharedDatabase::Setting::SETTING_LRZ2_BOSS);
		setupOptionEntry(option::INFINITE_LIVES,			SharedDatabase::Setting::SETTING_INFINITE_LIVES);
		setupOptionEntry(option::INFINITE_TIME,				SharedDatabase::Setting::SETTING_INFINITE_TIME);
		setupOptionEntry(option::SPECIAL_STAGE_RING_COUNT,	SharedDatabase::Setting::SETTING_BS_COUNTDOWN_RINGS);
		setupOptionEntry(option::ICZ_NIGHTTIME,				SharedDatabase::Setting::SETTING_ICZ_NIGHTTIME);
		setupOptionEntry(option::ANTI_FLICKER,				SharedDatabase::Setting::SETTING_GFX_ANTIFLICKER);
		setupOptionEntry(option::TITLE_THEME,				SharedDatabase::Setting::SETTING_AUDIO_TITLE_THEME);
		setupOptionEntry(option::EXTRA_LIFE_JINGLE,			SharedDatabase::Setting::SETTING_AUDIO_EXTRALIFE_JINGLE);
		setupOptionEntry(option::INVINCIBILITY_THEME,		SharedDatabase::Setting::SETTING_AUDIO_INVINCIBILITY_THEME);
		setupOptionEntry(option::SUPER_THEME,				SharedDatabase::Setting::SETTING_AUDIO_SUPER_THEME);
		setupOptionEntry(option::MINIBOSS_THEME,			SharedDatabase::Setting::SETTING_AUDIO_MINIBOSS_THEME);
		setupOptionEntry(option::KNUCKLES_THEME,			SharedDatabase::Setting::SETTING_AUDIO_KNUCKLES_THEME);
		setupOptionEntry(option::HPZ_MUSIC,					SharedDatabase::Setting::SETTING_AUDIO_HPZ_MUSIC);
		setupOptionEntry(option::SSZ_BOSSTRACKS,			SharedDatabase::Setting::SETTING_SSZ_BOSS_TRACKS);
		setupOptionEntry(option::OUTRO_MUSIC,				SharedDatabase::Setting::SETTING_AUDIO_OUTRO);
		setupOptionEntry(option::COMPETITION_MENU_MUSIC,	SharedDatabase::Setting::SETTING_AUDIO_COMPETITION_MENU);
		setupOptionEntry(option::CONTINUE_SCREEN_MUSIC,		SharedDatabase::Setting::SETTING_AUDIO_CONTINUE_SCREEN);
		setupOptionEntry(option::CONTINUE_MUSIC,			SharedDatabase::Setting::SETTING_CONTINUE_MUSIC);
		setupOptionEntry(option::UNDERWATER_AUDIO,			SharedDatabase::Setting::SETTING_UNDERWATER_AUDIO);
		setupOptionEntry(option::REGION,					SharedDatabase::Setting::SETTING_REGION_CODE);
		setupOptionEntry(option::TIMEATTACK_GHOSTS,			SharedDatabase::Setting::SETTING_TIME_ATTACK_GHOSTS);
		setupOptionEntry(option::DROP_DASH,					SharedDatabase::Setting::SETTING_DROPDASH);
		setupOptionEntry(option::SUPER_PEELOUT,				SharedDatabase::Setting::SETTING_SUPER_PEELOUT);
		setupOptionEntry(option::DEBUG_MODE,				SharedDatabase::Setting::SETTING_DEBUG_MODE);
		setupOptionEntry(option::TITLE_SCREEN,				SharedDatabase::Setting::SETTING_TITLE_SCREEN);

		//setupOptionEntry(option::LEVELMUSIC_CNZ,			SharedDatabase::Setting::SETTING_CNZ_PROTOTYPE_MUSIC);
		//setupOptionEntry(option::LEVELMUSIC_ICZ,			SharedDatabase::Setting::SETTING_ICZ_PROTOTYPE_MUSIC);
		//setupOptionEntry(option::LEVELMUSIC_LBZ,			SharedDatabase::Setting::SETTING_LBZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_CNZ1,	SharedDatabase::Setting::SETTING_CNZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_CNZ2,	SharedDatabase::Setting::SETTING_CNZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_ICZ1,	SharedDatabase::Setting::SETTING_ICZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_ICZ2,	SharedDatabase::Setting::SETTING_ICZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_LBZ1,	SharedDatabase::Setting::SETTING_LBZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_LBZ2,	SharedDatabase::Setting::SETTING_LBZ_PROTOTYPE_MUSIC);
	}

	// Build up tab menu entries
	mTabMenuEntries.addEntry("", option::_TAB_SELECTION)
		.addOption("MODS",     Tab::Id::MODS)
		.addOption("DISPLAY",  Tab::Id::DISPLAY)
		.addOption("AUDIO",    Tab::Id::AUDIO)
		.addOption("VISUALS",  Tab::Id::VISUALS)
		.addOption("GAMEPLAY", Tab::Id::GAMEPLAY)
		.addOption("CONTROLS", Tab::Id::CONTROLS)
		.addOption("TWEAKS",   Tab::Id::TWEAKS)
		.addOption("INFO",     Tab::Id::INFO);

	for (int i = 0; i < Tab::Id::_NUM; ++i)
	{
		GameMenuEntries& entries = mTabs[i].mMenuEntries;
		entries.reserve(20);
		entries.addEntry();		// Dummy entry representing the title in menu navigation
	}

	// Mods tab needs to be rebuilt each time again

	// Display tab
	{
		Tab& tab = mTabs[Tab::Id::DISPLAY];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "General";

		entries.addEntry("Renderer:", option::RENDERER)
			.addOption("Fail-Safe / Software", (uint32)Configuration::RenderMethod::SOFTWARE)
			.addOption("OpenGL Software", (uint32)Configuration::RenderMethod::OPENGL_SOFT)
			.addOption("OpenGL Hardware", (uint32)Configuration::RenderMethod::OPENGL_FULL);

		entries.addEntry("Frame Sync:", option::FRAME_SYNC)
			.addOption("V-Sync Off", 0)
			.addOption("V-Sync On", 1)
			.addOption("V-Sync + FPS Cap", 2);

		entries.addEntry("Upscaling:", option::UPSCALING)
			.addOption("Integer Scale", 1)
			.addOption("Aspect Fit", 0)
			.addOption("Stretch 50%", 2)
			.addOption("Stretch 100%", 3);
			//.addOption("Scale To Fill", 4);	// Works, but shouldn't be an option, as it looks a bit broken

		entries.addEntry("Backdrop:", option::BACKDROP)
			.addOption("Black", 0)
			.addOption("Classic Box 1", 1)
			.addOption("Classic Box 2", 2)
			.addOption("Classic Box 3", 3);

		entries.addEntry("Screen Filter:", option::FILTERING)
			.addOption("Sharp", 0)
			.addOption("Soft 1", 1)
			.addOption("Soft 2", 2)
			.addOption("xBRZ", 3)
			.addOption("HQ2x", 4)
			.addOption("HQ3x", 5)
			.addOption("HQ4x", 6);

		entries.addEntry("Scanlines:", option::SCANLINES)
			.addOption("Off", 0)
			.addOption("25%", 1)
			.addOption("50%", 2)
			.addOption("75%", 3)
			.addOption("100%", 4);

		entries.addEntry("Background Blur:", option::BG_BLUR)
			.addOption("Off", 0)
			.addOption("25%", 1)
			.addOption("50%", 2)
			.addOption("75%", 3)
			.addOption("100%", 4);

		titles[entries.size()] = "Window Mode";

		entries.addEntry("Current Screen:", option::WINDOW_MODE)
			.addOption("Windowed", 0)
			.addOption("Fullscreen", 1)
			.addOption("Exclusive Fullscreen", 2);

		entries.addEntry("Startup Screen:", option::WINDOW_MODE_STARTUP)
			.addOption("Windowed", 0)
			.addOption("Fullscreen", 1)
			.addOption("Exclusive Fullscreen", 2);

		titles[entries.size()] = "Performance Output";

		entries.addEntry("Show Performance:", option::PERFORMANCE_DISPLAY)
			.addOption("Off", 0)
			.addOption("Show Framerate", 1)
			.addOption("Full Profiling", 2);
	}

	// Audio tab
	{
		Tab& tab = mTabs[Tab::Id::AUDIO];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "Volume";

		const char* volumeName[] = { "Overall Volume:", "Music Volume:", "Sound Volume:" };
		for (int k = 0; k < 3; ++k)
		{
			GameMenuEntries::Entry& entry = entries.addEntry(volumeName[k], option::AUDIO_VOLUME + k);
			entry.addOption("Off", 0);
			for (int i = 5; i <= 100; i += 5)
				entry.addOption(*String(0, "%d %%", i), i);
		}

		titles[entries.size()] = "Soundtrack";

		entries.addEntry("Soundtrack Type:", option::SOUNDTRACK)
			.addOption("Emulated", 0)
			.addOption("Remastered", 1);

		entries.addEntry("Sound Test:", option::SOUND_TEST);	// Will be filled with content in "initialize()"

		titles[entries.size()] = "Theme Selection";

		entries.addEntry("Title Theme:", option::TITLE_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1);

		entries.addEntry("1-up Jingle:", option::EXTRA_LIFE_JINGLE)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("Pick by Zone", 0x10);

		entries.addEntry("Invincibility Theme:", option::INVINCIBILITY_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("Pick by Zone", 0x10);

		entries.addEntry("Super/Hyper Theme:", option::SUPER_THEME)
			.addOption("Normal level music", 0)
			.addOption("Fast level music", 1)
			.addOption("Sonic 2", 2)
			.addOption("Sonic 3", 3)
			.addOption("Sonic & Knuckles", 4)
			.addOption("S3 Prototype", 5);

		entries.addEntry("Mini-Boss Theme:", option::MINIBOSS_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("Pick by Zone", 0x10);

		entries.addEntry("Knuckles' Theme:", option::KNUCKLES_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("S3 Prototype", 2)
			.addOption("Pick by Zone", 0x10);

		titles[entries.size()] = "Level Music";

		entries.addEntry("Carnival Night Act 1:", option::LEVELMUSIC_CNZ1)
			.addOption("As Released", 0x00000001)
			.addOption("S3 Prototype", 0x80000001);

		entries.addEntry("Carnival Night Act 2:", option::LEVELMUSIC_CNZ2)
			.addOption("As Released", 0x00000002)
			.addOption("S3 Prototype", 0x80000002);

		entries.addEntry("IceCap Act 1:", option::LEVELMUSIC_ICZ1)
			.addOption("As Released", 0x00000001)
			.addOption("S3 Prototype", 0x80000001);

		entries.addEntry("IceCap Act 2:", option::LEVELMUSIC_ICZ2)
			.addOption("As Released", 0x00000002)
			.addOption("S3 Prototype", 0x80000002);

		entries.addEntry("Launch Base Act 1:", option::LEVELMUSIC_LBZ1)
			.addOption("As Released", 0x00000001)
			.addOption("S3 Prototype", 0x80000001);

		entries.addEntry("Launch Base Act 2:", option::LEVELMUSIC_LBZ2)
			.addOption("As Released", 0x00000002)
			.addOption("S3 Prototype", 0x80000002);

		titles[entries.size()] = "Music Selection";

		entries.addEntry("In Hidden Palace:", option::HPZ_MUSIC)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("S3 + S&K Mini-Boss", 2);

		entries.addEntry("Sky Sanctuary Bosses:", option::SSZ_BOSSTRACKS)
			.addOption("Normal Boss Music", 0)
			.addOption("Sonic 1 & 2 Tracks", 1);

		entries.addEntry("Outro Music:", option::OUTRO_MUSIC)
			.addOption("Sky Sanctuary", 0)
			.addOption("Sonic 3 Credits", 1)
			.addOption("S3 Prototype", 2);

		entries.addEntry("Competition Menu:", option::COMPETITION_MENU_MUSIC)
			.addOption("Sonic 3", 0)
			.addOption("S3 Prototype", 1);

		entries.addEntry("Continue Screen:", option::CONTINUE_SCREEN_MUSIC)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1);

		titles[entries.size()] = "Music Behavior";

		entries.addEntry("On Level (Re)Start:", option::CONTINUE_MUSIC)
			.addOption("Restart Music", 0)
			.addOption("Continue Music", 1);

		titles[entries.size()] = "Effects";

		entries.addEntry("Underwater Sound:", option::UNDERWATER_AUDIO)
			.addOption("Normal", 0)
			.addOption("Muffled", 1);
	}

	// Visuals tab
	{
		Tab& tab = mTabs[Tab::Id::VISUALS];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "Visual Enhancements";

		entries.addEntry("Character Rotation:", option::ROTATION)
			.addOption("Original", 0)
			.addOption("Smooth", 1);

		entries.addEntry("Time Display:", option::TIME_DISPLAY)
			.addOption("Original", 0)
			.addOption("Extended", 1);

		entries.addEntry("Lives Display:", option::LIVES_DISPLAY)
			.addOption("Auto", 0)
			.addOption("Classic", 1)
			.addOption("Mobile", 2);

		entries.addEntry("Speed Shoes Effect:", option::SPEEDUP_AFTER_IMAGES)
			.addOption("None (Original)", 0)
			.addOption("After-Images", 1);

		entries.addEntry("Fast Run Animation:", option::FAST_RUN_ANIM)
			.addOption("None (Original)", 0)
			.addOption("Peel-Out", 1);

		entries.addEntry("Flicker Effects:", option::ANTI_FLICKER)
			.addOption("As Original", 0)
			.addOption("Slightly Smoothed", 1)
			.addOption("Heavily Smoothed", 2);

		titles[entries.size()] = "Camera";

		entries.addEntry("Outrun Camera:", option::CAMERA_OUTRUN)
			.addOption("Off", 0)
			.addOption("On", 1);

		entries.addEntry("Extended Camera:", option::EXTENDED_CAMERA)
			.addOption("Off", 0)
			.addOption("On", 1);

		titles[entries.size()] = "Objects";

		entries.addEntry("Monitor Style:", option::MONITOR_STYLE)
			.addOption("Sonic 1 / 2", 1)
			.addOption("Sonic 3 & Knuckles", 0);

		titles[entries.size()] = "Color Changes";

		entries.addEntry("IceCap Startup Time:", option::ICZ_NIGHTTIME)
			.addOption("Daytime", 0)
			.addOption("Morning Dawn", 1);

		titles[entries.size()] = "Special Stages";

		entries.addEntry("Blue Spheres Style:", option::SPECIAL_STAGE_VISUALS)
			.addOption("Classic", 0)
			.addOption("Modernized", 3);

		entries.addEntry("Ring Counter:", option::SPECIAL_STAGE_RING_COUNT)
			.addOption("Counting Up", 0)
			.addOption("Counting Down", 1);
	}

	// Gameplay tab
	{
		Tab& tab = mTabs[Tab::Id::GAMEPLAY];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "Levels";

		entries.addEntry("Level Layouts:", option::LEVEL_LAYOUTS)
			.addOption("Sonic 3", 0)
			.addOption("Sonic 3 & Knuckles", 1)
			.addOption("Sonic 3 A.I.R.", 2);

		titles[entries.size()] = "Difficulty Changes";

		entries.addEntry("Angel Island Bombing:", option::AIZ_BLIMPSEQUENCE)
			.addOption("Original", 0)
			.addOption("Alternative", 1);

		entries.addEntry("Big Arms Boss Fight:", option::LBZ_BIGARMS)
			.addOption("Only Knuckles", 0)
			.addOption("All characters", 1);

		entries.addEntry("Sandopolis Ghosts:", option::SOZ_GHOSTSPAWN)
			.addOption("Disabled", 1)
			.addOption("Enabled", 0);

		entries.addEntry("Lava Reef Act 2 Boss:", option::LRZ2_BOSS)
			.addOption("8 hits", 1)
			.addOption("14 hits (original)", 0);

		titles[entries.size()] = "Time Attack";

		entries.addEntry("Max. Recorded Ghosts:", option::TIMEATTACK_GHOSTS)
			.addOption("Off", 0)
			.addOption("1", 1)
			.addOption("3", 3)
			.addOption("5", 5);

		entries.addEntry("Quick Restart:", option::TIMEATTACK_INSTANTRESTART)
			.addOption("Hold Y Button", 0)
			.addOption("Press Y Button", 1);
	}

	// Controls tab
	{
		Tab& tab = mTabs[Tab::Id::CONTROLS];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "Unlocked by Secrets";

		entries.addEntry("Sonic Drop Dash:", option::DROP_DASH)
			.addOption("Off", 0)
			.addOption("On", 1);

		entries.addEntry("Sonic Super Peel-Out:", option::SUPER_PEELOUT)
			.addOption("Off", 0)
			.addOption("On", 1);

		for (size_t i = 1; i < entries.size(); ++i)
		{
			mUnlockedSecretsEntries[0].push_back(&entries[i]);
		}

		titles[entries.size()] = "Controllers";

		entries.addEntry("Setup Keyboard & Game Controllers...", option::CONTROLLER_SETUP);		// This text here won't be used, see rendering

		for (int k = 0; k < 2; ++k)
		{
			GameMenuEntries::Entry& entry = entries.addEntry(*String(0, "Controller Player %d", k+1), option::CONTROLLER_PLAYER_1 + k);
			if (Application::instance().hasVirtualGamepad())
				entry.addOption("None (Touch only)", -1);
			else
				entry.addOption("None (Keyboard only)", -1);
			// Actual options will get filled in inside "refreshGamepadLists"
			mGamepadAssignmentEntries[k] = &entry;
		}

		entries.addEntry("Other controllers", option::CONTROLLER_AUTOASSIGN)
			.addOption("Not used", -1)
			.addOption("Assign to Player 1", 0)
			.addOption("Assign to Player 2", 1);

		if (Application::instance().hasVirtualGamepad())
		{
			titles[entries.size()] = "Virtual Gamepad";

			entries.addEntry("Visibility:",   option::VGAMEPAD_OPACITY).addPercentageOptions(0, 100, 10);
			entries.addEntry("D-Pad Size:",	  option::VGAMEPAD_DPAD_SIZE).addNumberOptions(50, 150, 10);
			entries.addEntry("Buttons Size:", option::VGAMEPAD_BUTTONS_SIZE).addNumberOptions(50, 150, 10);
			entries.addEntry("Set Touch Gamepad Layout...",	option::VGAMEPAD_SETUP);
		}

		titles[entries.size()] = "Abilities";

		entries.addEntry("Sonic Insta-Shield:", option::INSTA_SHIELD)
			.addOption("Off", 0)
			.addOption("On", 1);

		entries.addEntry("Tails Assist:", option::TAILS_ASSIST)
			.addOption("Off", 0)
			.addOption("Sonic 3 A.I.R. Style", 1)
			.addOption("Hybrid Style", 2)
			.addOption("Sonic Mania Style", 3);

		entries.addEntry("Tails Flight Cancel:", option::TAILS_FLIGHT_CANCEL)
			.addOption("Off", 0)
			.addOption("Down + Jump", 1);

		entries.addEntry("Roll Jump Control Lock:", option::NO_CONTROL_LOCK)
			.addOption("Locked (Classic)", 0)
			.addOption("Free Movement", 1);

		entries.addEntry("Bubble Shield Bounce:", option::BUBBLE_SHIELD_BOUNCE)
			.addOption("Sonic 3 Style", 0)
			.addOption("Sonic Mania Style", 1);

		titles[entries.size()] = "Super & Hyper Forms";

		entries.addEntry("Tails Super Forms:", option::HYPER_TAILS)
			.addOption("Only Super Tails", 0)
			.addOption("Super & Hyper Tails", 1);

		entries.addEntry("Super Cancel:", option::SUPER_CANCEL)
			.addOption("Off", 0)
			.addOption("On", 1);
	}

	// Tweaks tab
	{
		Tab& tab = mTabs[Tab::Id::TWEAKS];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "Unlocked by Secrets";

		entries.addEntry("Debug Mode:", option::DEBUG_MODE)
			.addOption("Off", 0)
			.addOption("On", 1);

		entries.addEntry("Title Screen:", option::TITLE_SCREEN)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1);

		entries.addEntry("Game Speed:", option::GAME_SPEED)
			.addOption("50 Hz (slower)", 50)
			.addOption("60 Hz (normal)", 60)
			.addOption("75 Hz (faster)", 75)
			.addOption("90 Hz (much faster)", 90)
			.addOption("120 Hz (ridiculous)", 120)
			.addOption("144 Hz (ludicrous)", 144);

		for (size_t i = 1; i < entries.size(); ++i)
		{
			mUnlockedSecretsEntries[1].push_back(&entries[i]);
		}

		titles[entries.size()] = "Accessibility";

		entries.addEntry("Infinite Lives:", option::INFINITE_LIVES)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);

		entries.addEntry("Infinite Time:", option::INFINITE_TIME)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);

		titles[entries.size()] = "Game Variety";

		entries.addEntry("Shields:", option::SHIELD_TYPES)
			.addOption("Classic Shield", 0)
			.addOption("Elemental Shields", 1)
			.addOption("Classic + Elemental", 2)
			.addOption("Upgradable Shields", 3);

		entries.addEntry("Randomized Monitors:", option::RANDOM_MONITORS)
			.addOption("Normal Monitors", 0)
			.addOption("Random Shields", 1)
			.addOption("Random Monitors", 2);

		titles[entries.size()] = "Special Stages";

		entries.addEntry("Special Stage Layouts:", option::RANDOM_SPECIALSTAGES)
			.addOption("Original", 0)
			.addOption("Randomly Generated", 1);

		entries.addEntry("On Fail:", option::SPECIAL_STAGE_REPEAT)
			.addOption("Advance to next", 0)
			.addOption("Do not advance", 1);

		titles[entries.size()] = "Region";

		entries.addEntry("Region Code:", option::REGION)
			.addOption("Western (\"Tails\")", 0x80)
			.addOption("Japan (\"Miles\")", 0x00);
	}

	// Info tab
	{
		Tab& tab = mTabs[Tab::Id::INFO];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& titles = tab.mTitles;

		titles[entries.size()] = "$Hints";
		
		entries.addEntry("Open Manual in web browser", option::_OPEN_MANUAL);
	}

	for (int i = 1; i < Tab::Id::_NUM; ++i)		// Exclude "Mods" tab
	{
		GameMenuEntries& entries = mTabs[i].mMenuEntries;
		entries.addEntry("Back", option::_BACK);

		for (size_t k = 0; k < entries.size(); ++k)
		{
			GameMenuEntries::Entry& entry = entries[k];
			OptionEntry& optionEntry = mOptionEntries[entry.mData];
			optionEntry.mOptionId = (option::Option)entry.mData;
			optionEntry.mGameMenuEntry = &entry;
		}
	}
}

OptionsMenu::~OptionsMenu()
{
}

GameMenuBase::BaseState OptionsMenu::getBaseState() const
{
	switch (mState)
	{
		case State::APPEAR:		   return BaseState::FADE_IN;
		case State::SHOW:		   return BaseState::SHOW;
		case State::FADE_TO_MENU:  return BaseState::FADE_OUT;
		default:				   return BaseState::INACTIVE;
	}
}

void OptionsMenu::onFadeIn()
{
	mState = State::APPEAR;

	mMenuBackground->showPreview(false);
	mMenuBackground->startTransition(MenuBackground::Target::LIGHT);

	const ConfigurationImpl& config = ConfigurationImpl::instance();
	mOptionEntries[option::WINDOW_MODE].mGameMenuEntry->setSelectedIndexByValue((int)Application::instance().getWindowMode());
	mOptionEntries[option::WINDOW_MODE_STARTUP].mGameMenuEntry->setSelectedIndexByValue((int)config.mWindowMode);
	mOptionEntries[option::RENDERER].mGameMenuEntry->setSelectedIndexByValue((int)config.mRenderMethod);

	for (OptionEntry& optionEntry : mOptionEntries)
	{
		optionEntry.loadValue();
	}

	// Show / hide options
	mOptionEntries[option::DROP_DASH].mGameMenuEntry->setVisible(PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_DROPDASH));
	mOptionEntries[option::SUPER_PEELOUT].mGameMenuEntry->setVisible(PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_SUPER_PEELOUT));
	mOptionEntries[option::DEBUG_MODE].mGameMenuEntry->setVisible(PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_DEBUGMODE));
	mOptionEntries[option::TITLE_SCREEN].mGameMenuEntry->setVisible(PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_TITLE_SK));
	mOptionEntries[option::GAME_SPEED].mGameMenuEntry->setVisible(PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_GAME_SPEED));
#if defined(PLATFORM_ANDROID)
	mOptionEntries[option::WINDOW_MODE].mGameMenuEntry->setVisible(false);
	mOptionEntries[option::WINDOW_MODE_STARTUP].mGameMenuEntry->setVisible(false);
#endif

	AudioOut::instance().setMenuMusic(0x2f);
	mPlayingSoundTest = nullptr;
}

bool OptionsMenu::canBeRemoved()
{
	return (mState == State::INACTIVE && mVisibility <= 0.0f);
}

void OptionsMenu::initialize()
{
	if (nullptr == mControllerSetupMenu)
	{
		mControllerSetupMenu = new ControllerSetupMenu(*this);
		addChild(mControllerSetupMenu);
	}

	// Mods tab & mods option entries
	{
		uint32 nextOptionId = option::_NUM + 1;
		mOptionEntries.resize(nextOptionId);

		Tab& tab = mTabs[Tab::Id::MODS];
		GameMenuEntries& entries = tab.mMenuEntries;
		std::map<size_t, std::string>& sections = tab.mSections;
		std::map<size_t, std::string>& titles = tab.mTitles;
		entries.resize(1);
		sections.clear();
		titles.clear();

		const std::vector<Mod*>& activeMods = ModManager::instance().getActiveMods();
		for (int modIndex = (int)activeMods.size() - 1; modIndex >= 0; --modIndex)
		{
			Mod* mod = activeMods[modIndex];
			if (mod->mSettingCategories.empty())
				continue;

			sections[entries.size()] = mod->mDisplayName;

			for (Mod::SettingCategory& modSettingCategory : mod->mSettingCategories)
			{
				if (modSettingCategory.mDisplayName.empty())
				{
					if (mod->mSettingCategories.size() >= 2)
					{
						titles[entries.size()] = "Other Settings";
					}
				}
				else
				{
					titles[entries.size()] = modSettingCategory.mDisplayName;
				}

				for (Mod::Setting& modSetting : modSettingCategory.mSettings)
				{
					GameMenuEntries::Entry& entry = entries.addEntry(modSetting.mDisplayName, nextOptionId);
					for (const Mod::Setting::Option& option : modSetting.mOptions)
					{
						entry.addOption(option.mDisplayName, option.mValue);
					}

					OptionEntry& optionEntry = vectorAdd(mOptionEntries);
					optionEntry.mOptionId = (option::Option)nextOptionId;
					optionEntry.mType = OptionEntry::Type::MOD_SETTING;
					optionEntry.mValuePointer = reinterpret_cast<void*>(&modSetting);
					++nextOptionId;
				}
			}
		}

		for (size_t k = 0; k < entries.size(); ++k)
		{
			GameMenuEntries::Entry& entry = entries[k];
			OptionEntry& optionEntry = mOptionEntries[entry.mData];
			optionEntry.mGameMenuEntry = &entry;
		}

		const bool anyModOptions = (nextOptionId > option::_NUM + 1);
		if (!anyModOptions && mActiveTab == Tab::Id::MODS)
		{
			++mActiveTab;
			mActiveTabAnimated = (float)mActiveTab;
		}
	
		mTabMenuEntries[0].mOptions[Tab::Id::MODS].mVisible = anyModOptions;
		mTabMenuEntries[0].mSelectedIndex = mActiveTab;
	}

	// Fill sound test
	{
		mSoundTestAudioDefinitions.clear();
		const auto& audioDefinitions = AudioOut::instance().getAudioCollection().getAudioDefinitions();
		for (const auto& pair : audioDefinitions)
		{
			if (pair.second.mType == AudioCollection::AudioDefinition::Type::MUSIC || pair.second.mType == AudioCollection::AudioDefinition::Type::JINGLE)
			{
				mSoundTestAudioDefinitions.emplace_back(&pair.second);
			}
		}

		std::sort(mSoundTestAudioDefinitions.begin(), mSoundTestAudioDefinitions.end(),
			[](const AudioCollection::AudioDefinition* a, const AudioCollection::AudioDefinition* b) { return a->mKeyString < b->mKeyString; });

		GameMenuEntries::Entry& entry = *mOptionEntries[option::SOUND_TEST].mGameMenuEntry;
		entry.mOptions.clear();
		for (size_t index = 0; index < mSoundTestAudioDefinitions.size(); ++index)
		{
			entry.addOption(mSoundTestAudioDefinitions[index]->mKeyString, (uint32)index);
		}
		entry.sanitizeSelectedIndex();
	}

	// Fill gamepad lists
	refreshGamepadLists(true);
}

void OptionsMenu::deinitialize()
{
	GameMenuBase::deinitialize();
}

void OptionsMenu::keyboard(const rmx::KeyboardEvent& ev)
{
	GameMenuBase::keyboard(ev);
}

void OptionsMenu::update(float timeElapsed)
{
	mActiveTabAnimated += clamp((float)mActiveTab - mActiveTabAnimated, -timeElapsed * 4.0f, timeElapsed * 4.0f);

	// Don't react to input during transitions (i.e. when state is not SHOW), or when child menu is active
	if (mState == State::SHOW && !mControllerSetupMenu->isVisible())
	{
		ConfigurationImpl& config = ConfigurationImpl::instance();
		const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);

		mOptionEntries[option::WINDOW_MODE].mGameMenuEntry->setSelectedIndexByValue((int)Application::instance().getWindowMode());
		mOptionEntries[option::FRAME_SYNC].loadValue();
		mOptionEntries[option::FILTERING].loadValue();
		mOptionEntries[option::BG_BLUR].loadValue();
		mOptionEntries[option::AUDIO_VOLUME].loadValue();

		if (mActiveMenu == &mTabMenuEntries && (keys.Down.justPressedOrRepeat() || keys.Up.justPressedOrRepeat()))
		{
			// Switch from title to tab content
			mActiveMenu = &mTabs[mActiveTab].mMenuEntries;
			mActiveMenu->mSelectedEntryIndex = 0;
		}

		// Update menu entries
		const GameMenuEntries::UpdateResult result = mActiveMenu->update();
		if (result != GameMenuEntries::UpdateResult::NONE)
		{
			if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED && mActiveMenu == &mTabMenuEntries)
			{
				mActiveTab = mTabMenuEntries[option::_TAB_SELECTION].mSelectedIndex;
				playMenuSound(0xb7);
			}
			else
			{
				playMenuSound(0x5b);

				if (result == GameMenuEntries::UpdateResult::ENTRY_CHANGED && mActiveMenu != &mTabMenuEntries && mActiveMenu->mSelectedEntryIndex == 0)
				{
					// Switch from tab content to title
					mActiveMenu = &mTabMenuEntries;
				}
				else if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED && mActiveMenu != &mTabMenuEntries)
				{
					const GameMenuEntries::Entry& selectedEntry = mActiveMenu->selected();
					const uint32 selectedData = selectedEntry.mData;
					switch (selectedData)
					{
						case option::WINDOW_MODE:
						{
							Application::instance().setWindowMode((Application::WindowMode)selectedEntry.selected().mValue);
							break;
						}

						case option::RENDERER:
						{
							EngineMain::instance().switchToRenderMethod((Configuration::RenderMethod)selectedEntry.selected().mValue);
							break;
						}

						case option::SOUNDTRACK:
						{
							// Change soundtrack and restart music
							config.mActiveSoundtrack = selectedEntry.selected().mValue;
							AudioOut::instance().stopSoundContext(AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_MUSIC);
							AudioOut::instance().onSoundtrackPreferencesChanged();
							if (nullptr == mPlayingSoundTest)
							{
								AudioOut::instance().restartMenuMusic();
							}
							else
							{
								playSoundtest(*mPlayingSoundTest);
							}
							break;
						}

						case option::CONTROLLER_PLAYER_1:
						case option::CONTROLLER_PLAYER_2:
						{
							const InputManager::RealDevice* gamepad = InputManager::instance().getGamepadByJoystickInstanceId(selectedEntry.selected().mValue);
							InputManager::instance().setPreferredGamepad((int)(selectedEntry.mData - option::CONTROLLER_PLAYER_1), gamepad);
							break;
						}

						case option::CONTROLLER_AUTOASSIGN:
						{
							mOptionEntries[selectedData].applyValue();
							InputManager::instance().updatePlayerGamepadAssignments();
							break;
						}

						default:
						{
							// Apply change
							config.mWindowMode = (Configuration::WindowMode)mOptionEntries[option::WINDOW_MODE_STARTUP].mGameMenuEntry->selected().mValue;

							if (selectedData > option::_TAB_SELECTION && selectedData != option::_BACK)
							{
								mOptionEntries[selectedData].applyValue();

								if (selectedData >= option::VGAMEPAD_DPAD_SIZE && selectedData <= option::VGAMEPAD_BUTTONS_SIZE)
								{
									TouchControlsOverlay::instance().buildTouchControls();
								}
								if (selectedData == option::FRAME_SYNC)
								{
									EngineMain::instance().setVSyncMode(selectedEntry.selected().mValue);
								}
							}
							break;
						}
					}
				}
			}
		}

		enum class ButtonEffect
		{
			NONE,
			ACCEPT,
			BACK
		};
		const ButtonEffect buttonEffect = (keys.Start.justPressed() || keys.A.justPressed() || keys.X.justPressed()) ? ButtonEffect::ACCEPT :
										  (keys.Back.justPressed() || keys.B.justPressed()) ? ButtonEffect::BACK : ButtonEffect::NONE;

		if (buttonEffect != ButtonEffect::NONE)
		{
			if (buttonEffect == ButtonEffect::BACK)
			{
				goBack();
			}
			else if (buttonEffect == ButtonEffect::ACCEPT && mActiveMenu != &mTabMenuEntries)
			{
				Tab& tab = mTabs[mActiveTab];
				const GameMenuEntries::Entry& selectedEntry = tab.mMenuEntries.selected();
				switch (selectedEntry.mData)
				{
					case option::SOUND_TEST:
					{
						playSoundtest(*mSoundTestAudioDefinitions[selectedEntry.selected().mValue]);
						break;
					}

					case option::CONTROLLER_SETUP:
					{
						playMenuSound(0x63);
						mControllerSetupMenu->setRect(mRect);
						mControllerSetupMenu->fadeIn();
						break;
					}

					case option::VGAMEPAD_SETUP:
					{
						InputManager::instance().setLastInputType(InputManager::InputType::TOUCH);
						TouchControlsOverlay::instance().enableConfigMode(true);
						break;
					}

					case option::_OPEN_MANUAL:
					{
						PlatformFunctions::openURLExternal("https://sonic3air.org/Manual.pdf");
						break;
					}

					case option::_BACK:
					{
						goBack();
						break;
					}

					default:
						break;
				}
			}
		}
	}

	// Enable / disable options
	//  -> Done here as the conditions can change at any time (incl. hotkeys)
	const bool isSoftware = (Configuration::instance().mRenderMethod == Configuration::RenderMethod::SOFTWARE);
	mOptionEntries[option::SCANLINES].mGameMenuEntry->setEnabled(!isSoftware && Configuration::instance().mFiltering < 3);
	mOptionEntries[option::FILTERING].mGameMenuEntry->setEnabled(!isSoftware);

	// Scrolling
	mScrolling.update(timeElapsed);

	// Fading in/out
	if (mState == State::APPEAR)
	{
		mVisibility = saturate(mVisibility + timeElapsed * 6.0f);
		if (mVisibility >= 1.0f)
		{
			mState = State::SHOW;
		}
	}
	else if (mState > State::SHOW)
	{
		mVisibility = saturate(mVisibility - timeElapsed * 6.0f);
		if (mVisibility <= 0.0f)
		{
			mState = State::INACTIVE;
		}
	}

	// Check for changes in connected gamepads
	refreshGamepadLists();

	// Uodate children at the end
	GameMenuBase::update(timeElapsed);
}

void OptionsMenu::render()
{
	Drawer& drawer = EngineMain::instance().getDrawer();

	int anchorX = 200;
	int anchorY = 0;
	float alpha = 1.0f;
	if (mState != State::SHOW)
	{
		anchorX += roundToInt((1.0f - mVisibility) * 300.0f);
		alpha = mVisibility;
	}
	if (mControllerSetupMenu->isVisible())
	{
		anchorY -= roundToInt(mControllerSetupMenu->getVisibility() * 80.0f);
		alpha *= (1.0f - mControllerSetupMenu->getVisibility());
	}

	if (alpha > 0.0f)
	{
		const int startY = anchorY + 30 - mScrolling.getScrollOffsetYInt();

		drawer.pushScissor(Recti(0, anchorY + 30, (int)mRect.width, (int)mRect.height - anchorY - 30));

		const int minTabIndex = (int)std::floor(mActiveTabAnimated);
		const int maxTabIndex = (int)std::ceil(mActiveTabAnimated);

		for (int tabIndex = minTabIndex; tabIndex <= maxTabIndex; ++tabIndex)
		{
			const Tab& tab = mTabs[tabIndex];
			const bool isModsTab = (tabIndex == Tab::Id::MODS);

			const float tabAlpha = alpha * (1.0f - std::fabs(tabIndex - mActiveTabAnimated));
			const int baseX = anchorX + roundToInt((tabIndex - mActiveTabAnimated) * 250);
			int py = startY + 12;
			const std::string* pendingTitle = nullptr;

			for (size_t line = 1; line < tab.mMenuEntries.size(); ++line)
			{
				int currentAbsoluteY1 = py - startY;

				const auto sectionIterator = tab.mSections.find(line);
				if (sectionIterator != tab.mSections.end())
				{
					py += 14;
					const int textWidth = global::mFont10.getWidth(sectionIterator->second);
					drawer.printText(global::mFont10, Recti(baseX - 140, py, 0, 10), sectionIterator->second, 4, Color(0.7f, 1.0f, 0.9f, tabAlpha));
					drawer.drawRect(Recti(baseX - 185, py + 4, 40, 1), Color(0.7f, 1.0f, 0.9f, tabAlpha));
					drawer.drawRect(Recti(baseX - 184, py + 5, 40, 1), Color(0.0f, 0.0f, 0.0f, tabAlpha * 0.75f));
					drawer.drawRect(Recti(baseX - 135 + textWidth, py + 4, 320 - textWidth, 1), Color(0.7f, 1.0f, 0.9f, tabAlpha));
					drawer.drawRect(Recti(baseX - 134 + textWidth, py + 5, 320 - textWidth, 1), Color(0.0f, 0.0f, 0.0f, tabAlpha * 0.75f));
					py += 20;

					if (line > 1)
					{
						// Refresh position for scrolling once after text, except if it was the first
						currentAbsoluteY1 = py - startY;
					}
				}

				{
					const auto titleIterator = tab.mTitles.find(line);
					if (titleIterator != tab.mTitles.end())
					{
						if (isTitleShown(tabIndex, (int)line))
						{
							pendingTitle = &titleIterator->second;
						}
					}
				}

				const auto& entry = tab.mMenuEntries[line];
				if (!entry.isVisible())
				{
					// Skip hidden entries
					continue;
				}

				const bool isBack = (entry.mData == option::_BACK);
				if (isBack)
				{
					pendingTitle = nullptr;
				}	 
				if (nullptr != pendingTitle)
				{
					if (*pendingTitle == "$Hints")
					{
						// Show hint text
						py += 16;
						int px = baseX - 152;
						for (size_t k = 0; k < option::HINT_TEXTS.size(); ++k)
						{
							const char* text = option::HINT_TEXTS[k].c_str();
							if (text[0] == '$')
							{
								px = baseX - 152;
								py += 12;
								++text;
							}
							const bool highlighted = (text[0] == '#');
							if (highlighted)
							{
								++text;
							}
							const int width = global::mFont7.getWidth(text);
							drawer.printText(global::mFont7, Recti(px, py, width, 10), text, 1, (highlighted) ? Color(0.5f, 1.0f, 0.75f, tabAlpha) : Color(0.8f, 0.8f, 0.8f, tabAlpha));
							px += width;
						}
						py += 16;
						py += 8;
					}
					else
					{
						// Show title
						py += (sectionIterator != tab.mSections.end()) ? 4 : 15;
						drawer.printText(global::mFont7, Recti(baseX, py, 0, 10), ("* " + *pendingTitle + " *"), 5, Color(0.6f, 0.8f, 1.0f, tabAlpha));
						py += 18;

						if (line > 1)
						{
							// Refresh position for scrolling once after text, except if it was the first
							currentAbsoluteY1 = py - startY;
						}
					}
					pendingTitle = nullptr;
				}

				const bool isSelected = (mActiveMenu == &tab.mMenuEntries && (int)line == tab.mMenuEntries.mSelectedEntryIndex);
				const bool isDisabled = !entry.isEnabled();

				Color color = isSelected ? Color::YELLOW : isDisabled ? Color(0.4f, 0.4f, 0.4f) : Color(1.0f, 1.0f, 1.0f);
				color.a *= tabAlpha;

				if (entry.mOptions.empty())
				{
					// Used for selectable entries, like "Back"
					if (entry.mData == option::_BACK)
					{
						py += 16;
					}

					if (entry.mData == option::CONTROLLER_SETUP)
					{
						drawer.printText(global::mFont10, Recti(baseX, py, 0, 10), Application::instance().hasKeyboard() ? "Setup Keyboard & Game Controllers..." : "Setup Game Controllers...", 5, color);
					}
					else
					{
						drawer.printText(global::mFont10, Recti(baseX, py, 0, 10), entry.mText, 5, color);
					}

					if (isSelected)
					{
						// Draw arrows
						const int halfTextWidth = global::mFont10.getWidth(entry.mText) / 2;
						const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
						const int arrowDistance = 16 + ((offset > 3) ? (6 - offset) : offset);
						drawer.printText(global::mFont10, Recti(baseX - halfTextWidth - arrowDistance, py, 0, 10), ">>", 5, color);
						drawer.printText(global::mFont10, Recti(baseX + halfTextWidth + arrowDistance, py, 0, 10), "<<", 5, color);
					}

					if (entry.mData == option::CONTROLLER_SETUP)
						py += 4;
				}
				else
				{
					// It's an actual options entry, with multiple options to choose from
					Font& font = isModsTab ? global::mFont5 : global::mFont10;

					const bool canGoLeft  = !isDisabled && (entry.mSelectedIndex > 0);
					const bool canGoRight = !isDisabled && (entry.mSelectedIndex < entry.mOptions.size() - 1);

					const int center = baseX + 88;
					int arrowDistance = 75;
					if (isSelected)
					{
						const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
						arrowDistance += ((offset > 3) ? (6 - offset) : offset);
					}

					// Description
					drawer.printText(font, Recti(baseX - 40, py, 0, 10), entry.mText, 6, color);

					// Value text
					if (entry.mData == option::SOUND_TEST && AudioOut::instance().isSoundIdModded(mSoundTestAudioDefinitions[entry.selected().mValue]->mKeyId))
					{
						drawer.printText(font, Recti(center - 80, py, 160, 10), entry.mOptions[entry.mSelectedIndex].mText + " (modded)", 5, color);
					}
					else
					{
						const std::string& text = (isDisabled && entry.mData != option::RENDERER) ? option::TEXT_NOT_AVAILABLE : entry.mOptions[entry.mSelectedIndex].mText;
						Font* valueFont = &font;
						drawer.printText(*valueFont, Recti(center - 80, py, 160, 10), text, 5, color);
					}

					if (canGoLeft)
						drawer.printText(font, Recti(center - arrowDistance, py, 0, 10), "<", 5, color);
					if (canGoRight)
						drawer.printText(font, Recti(center + arrowDistance, py, 0, 10), ">", 5, color);
				}

				if (isSelected)
				{
					const int currentAbsoluteY2 = py - startY;
					mScrolling.setCurrentSelection(currentAbsoluteY1 - 30, currentAbsoluteY2 + 45);
				}

				py += isModsTab ? 13 : 16;
			}
		}

		drawer.popScissor();

		// Tab titles
		{
			// Background
			drawer.drawRect(Recti(anchorX - 200, anchorY - 6, 400, 48), global::mOptionsTopBar, Color(1.0f, 1.0f, 1.0f, alpha));

			const int py = anchorY + 4;
			const auto& entry = mTabMenuEntries[0];
			const bool isSelected = (mActiveMenu == &mTabMenuEntries);
			const Color color = isSelected ? Color(1.0f, 1.0f, 0.0f, alpha) : Color(1.0f, 1.0f, 1.0f, alpha);

			const bool canGoLeft  = (entry.mSelectedIndex > 0 && entry.mOptions[entry.mSelectedIndex-1].mVisible);
			const bool canGoRight = (entry.mSelectedIndex < entry.mOptions.size() - 1);

			const int center = anchorX;
			int arrowDistance = 77;
			if (isSelected)
			{
				const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
				arrowDistance += ((offset > 3) ? (6 - offset) : offset);
			}

			// Show all tab titles
			for (size_t k = 0; k < entry.mOptions.size(); ++k)
			{
				if (entry.mOptions[k].mVisible)
				{
					const Color color2 = (k == entry.mSelectedIndex) ? color : Color(0.9f, 0.9f, 0.9f, alpha * 0.8f);
					const std::string& text = entry.mOptions[k].mText;
					const int px = roundToInt(((float)k - mActiveTabAnimated) * 180.0f) + center - 80;
					drawer.printText(global::mFont18, Recti(px, py, 160, 20), text, 5, color2);
				}
			}

			if (canGoLeft)
				drawer.printText(global::mFont10, Recti(center - arrowDistance, py + 6, 0, 10), "<", 5, color);
			if (canGoRight)
				drawer.printText(global::mFont10, Recti(center + arrowDistance, py + 6, 0, 10), ">", 5, color);

			if (isSelected)
			{
				mScrolling.setCurrentSelection(0, py);
			}
		}

		drawer.performRendering();
	}

	// Render children on top
	GameMenuBase::render();
}

void OptionsMenu::onEnteredFromIngame()
{
	// TODO: Disable options not available in-game
}

void OptionsMenu::removeControllerSetupMenu()
{
}

void OptionsMenu::setupOptionEntry(option::Option optionId, SharedDatabase::Setting::Type setting)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::SETTING;
	optionEntry.mSetting = setting;
}

void OptionsMenu::setupOptionEntryBitmask(option::Option optionId, SharedDatabase::Setting::Type setting)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::SETTING_BITMASK;
	optionEntry.mSetting = setting;
}

void OptionsMenu::setupOptionEntryInt(option::Option optionId, int* valuePointer)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::CONFIG_INT;
	optionEntry.mValuePointer = valuePointer;
}

void OptionsMenu::setupOptionEntryPercent(option::Option optionId, float* valuePointer)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::CONFIG_PERCENT;
	optionEntry.mValuePointer = valuePointer;
}

void OptionsMenu::playSoundtest(const AudioCollection::AudioDefinition& audioDefinition)
{
	mPlayingSoundTest = &audioDefinition;
	AudioOut::instance().stopSoundContext(AudioOut::CONTEXT_MENU);
	if (rmx::endsWith(audioDefinition.mKeyString, "_fast") && ConfigurationImpl::instance().mActiveSoundtrack == 0)
	{
		AudioOut::instance().enableAudioModifier(0, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_MUSIC, "_fast", 1.25f);
		AudioOut::instance().playAudioDirect(rmx::getMurmur2_64(audioDefinition.mKeyString.substr(0, audioDefinition.mKeyString.length() - 5)), (AudioOut::SoundRegType)audioDefinition.mType, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_MUSIC);
	}
	else
	{
		AudioOut::instance().disableAudioModifier(0, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_MUSIC);
		AudioOut::instance().playAudioDirect(audioDefinition.mKeyId, (AudioOut::SoundRegType)audioDefinition.mType, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_MUSIC);
	}
}

void OptionsMenu::refreshGamepadLists(bool forceUpdate)
{
	// Rebuild gamepad lists if needed
	const uint32 changeCounter = InputManager::instance().getGamepadsChangeCounter();
	if (mLastGamepadsChangeCounter != changeCounter || forceUpdate)
	{
		mLastGamepadsChangeCounter = changeCounter;
		for (int playerIndex = 0; playerIndex < 2; ++playerIndex)
		{
			GameMenuEntries::Entry& entry = *mGamepadAssignmentEntries[playerIndex];
			const int32 preferredValue = InputManager::instance().getPreferredGamepadByJoystickInstanceId(playerIndex);
			const uint32 oldSelectedValue = (preferredValue >= 0) ? (uint32)preferredValue : entry.hasSelected() ? entry.selected().mValue : (uint32)-1;
			entry.mOptions.resize(1);	// First entry is the "None" entry

			for (const InputManager::RealDevice& gamepad : InputManager::instance().getGamepads())
			{
				std::string text = gamepad.getName();
				utils::shortenTextToFit(text, global::mFont10, 135);
				entry.addOption(text, gamepad.mSDLJoystickInstanceId);
			}
			if (!entry.setSelectedIndexByValue(oldSelectedValue))
			{
				entry.mSelectedIndex = 0;
			}
		}
	}
}

bool OptionsMenu::isTitleShown(int tabIndex, int line) const
{
	// Special handling for first titles in Gameplay and Tweaks tabs, if no no unlocks are available there yet
	if (line != 1)
		return true;

	const int index = (tabIndex == Tab::Id::CONTROLS) ? 0 : (tabIndex == Tab::Id::TWEAKS) ? 1 : -1;
	if (index == -1)
		return true;

	for (auto* entry : mUnlockedSecretsEntries[index])
	{
		if (entry->isVisible())
			return true;
	}
	return false;
}

void OptionsMenu::goBack()
{
	playMenuSound(0xad);
	if (nullptr != mPlayingSoundTest && mPlayingSoundTest->mKeyId != 0x2f)
	{
		AudioOut::instance().stopSoundContext(AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_MUSIC);
	}

	// Save changes
	ModManager::instance().copyModSettingsToConfig();
	Configuration::instance().saveSettings();

	GameApp::instance().onExitOptions();
	mState = State::FADE_TO_MENU;
}
