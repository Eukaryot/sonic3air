/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/options/OptionsConfig.h"
#include "oxygen/application/Application.h"


namespace
{
	struct ConfigBuilder
	{
		explicit ConfigBuilder(OptionsConfig::OptionsTab& optionsTab) :
			mOptionsTab(optionsTab)
		{
			mOptionsTab.mCategories.clear();
		}

		ConfigBuilder& addCategory(std::string_view name)
		{
			vectorAdd(mOptionsTab.mCategories).mName = name;
			return *this;
		}

		ConfigBuilder& addSetting(std::string_view name, option::Option key)
		{
			OptionsConfig::Setting& setting = vectorAdd(mOptionsTab.mCategories.back().mSettings);
			setting.mKey = key;
			setting.mName = name;
			return *this;
		}

		ConfigBuilder& addOption(std::string_view name, uint32 value)
		{
			OptionsConfig::Setting& setting = mOptionsTab.mCategories.back().mSettings.back();
			setting.mOptions.emplace_back(name, value);
			return *this;
		}

		ConfigBuilder& addNumberOptions(int minValue, int maxValue, int step, const char* postfix = "")
		{
			OptionsConfig::Setting& setting = mOptionsTab.mCategories.back().mSettings.back();
			for (int value = minValue; value <= maxValue; value += step)
				setting.mOptions.emplace_back(std::to_string(value) + postfix, value);
			return *this;
		}

		OptionsConfig::OptionsTab& mOptionsTab;
	};
}



void OptionsConfig::build()
{
	buildSystem();
	buildDisplay();
	buildAudio();
	buildVisuals();
	buildGameplay();
	buildControls();
	buildTweaks();
}

#define CATEGORY(_name_) configBuilder.addCategory(_name_);

void OptionsConfig::buildSystem()
{
	ConfigBuilder configBuilder(mSystemOptions);

#if !defined(PLATFORM_VITA)
	CATEGORY("Update")
	{
		configBuilder.addSetting("Check for updates", option::_CHECK_FOR_UPDATE)
			.addOption("Stable updates", 0)
			.addOption("Stable & preview", 1)
			.addOption("All incl. test builds", 2);
	}

	CATEGORY("Ghost Sync")
	{
		configBuilder.addSetting("Enable Ghost Sync", option::GHOST_SYNC)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);

		configBuilder.addSetting("Ghost Display", option::GHOST_SYNC_RENDERING)
			.addOption("Full opacity", 1)
			.addOption("Semi-transparent", 2)
			.addOption("Ghost Style", 3);
	}
#endif

#if defined(SUPPORT_IMGUI)
	CATEGORY("Data Management")
	{
		configBuilder.addSetting("Open File Browser", option::_OPEN_FILE_BROWSER);
	}
#endif

	CATEGORY("More Info")
	{
		configBuilder.addSetting("Open Game Homepage", option::_OPEN_HOMEPAGE);
		configBuilder.addSetting("Open Manual", option::_OPEN_MANUAL);
	}

	CATEGORY("Debugging")
	{
		configBuilder.addSetting("Script Optimization", option::SCRIPT_OPTIMIZATION)
			.addOption("Auto (Default)", -1)
			.addOption("Disabled", 0)
			.addOption("Basic", 1)
			.addOption("Full", 3);

		configBuilder.addSetting("Debug Game Recording", option::GAME_RECORDING_MODE)
			.addOption("Auto (Default)", -1)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);
	}

	CATEGORY("Dev Mode")
	{
		configBuilder.addSetting("Dev Mode", option::DEV_MODE)
			.addOption("Off", 0)
			.addOption("On", 1);
	}
}

void OptionsConfig::buildDisplay()
{
	ConfigBuilder configBuilder(mDisplayOptions);

	CATEGORY("General")
	{
		configBuilder.addSetting("Renderer:", option::RENDERER);
		const Configuration::RenderMethod highest = Configuration::getHighestSupportedRenderMethod();

	#if !defined(PLATFORM_VITA)
		configBuilder.addOption("Fail-Safe / Software", (uint32)Configuration::RenderMethod::SOFTWARE);
		if (highest >= Configuration::RenderMethod::OPENGL_SOFT)
			configBuilder.addOption("OpenGL Software", (uint32)Configuration::RenderMethod::OPENGL_SOFT);
		if (highest >= Configuration::RenderMethod::OPENGL_FULL)
			configBuilder.addOption("OpenGL Hardware", (uint32)Configuration::RenderMethod::OPENGL_FULL);
	#else
		// OpenGL Hardware does not work correctly on PSVita
		configBuilder.addOption("OpenGL Software", (uint32)Configuration::RenderMethod::OPENGL_SOFT);
	#endif

		configBuilder.addSetting("Frame Sync:", option::FRAME_SYNC)
			.addOption("V-Sync Off", 0)
			.addOption("V-Sync On", 1)
			.addOption("V-Sync + FPS Cap", 2);

		configBuilder.addSetting("Upscaling:", option::UPSCALING)
			.addOption("Integer Scale", 1)
			.addOption("Aspect Fit", 0)
			.addOption("Stretch 50%", 2)
			.addOption("Stretch 100%", 3);
			//.addOption("Scale To Fill", 4);	// Works, but shouldn't be an option, as it looks a bit broken

		configBuilder.addSetting("Backdrop:", option::BACKDROP)
			.addOption("Black", 0)
			.addOption("Classic Box 1", 1)
			.addOption("Classic Box 2", 2)
			.addOption("Classic Box 3", 3);

	#if !defined(PLATFORM_VITA)
		configBuilder.addSetting("Screen Filter:", option::FILTERING)
			.addOption("Sharp", 0)
			.addOption("Soft 1", 1)
			.addOption("Soft 2", 2)
			.addOption("xBRZ", 3)
			.addOption("HQ2x", 4)
			.addOption("HQ3x", 5)
			.addOption("HQ4x", 6);
	#else
		// High quality filters on the PSVITA is playing in slowmotion...
		configBuilder.addSetting("Screen Filter:", option::FILTERING)
			.addOption("Sharp", 0)
			.addOption("Soft 1", 1)
			.addOption("Soft 2", 2);
	#endif

		configBuilder.addSetting("Scanlines:", option::SCANLINES)
			.addOption("Off", 0)
			.addOption("25%", 1)
			.addOption("50%", 2)
			.addOption("75%", 3)
			.addOption("100%", 4);

		configBuilder.addSetting("Background Blur:", option::BG_BLUR)
			.addOption("Off", 0)
			.addOption("25%", 1)
			.addOption("50%", 2)
			.addOption("75%", 3)
			.addOption("100%", 4);
	}

	CATEGORY("Window Mode")
	{
	#if !defined(PLATFORM_VITA)
		configBuilder.addSetting("Current Screen:", option::WINDOW_MODE)
			.addOption("Windowed", 0)
			.addOption("Fullscreen", 1)
			.addOption("Exclusive Fullscreen", 2);

		configBuilder.addSetting("Startup Screen:", option::WINDOW_MODE_STARTUP)
			.addOption("Windowed", 0)
			.addOption("Fullscreen", 1)
			.addOption("Exclusive Fullscreen", 2);
	#else
		// These aren't supposed to show up on the Vita
		configBuilder.addSetting("Current Screen:", option::WINDOW_MODE)
			.addOption("Exclusive Fullscreen", 0);

		configBuilder.addSetting("Startup Screen:", option::WINDOW_MODE_STARTUP)
			.addOption("Exclusive Fullscreen", 0);
	#endif
	}

	CATEGORY("Performance Output")
	{
		configBuilder.addSetting("Show Performance:", option::PERFORMANCE_DISPLAY)
			.addOption("Off", 0)
			.addOption("Show Framerate", 1)
			.addOption("Full Profiling", 2);
	}
}

void OptionsConfig::buildAudio()
{
	ConfigBuilder configBuilder(mAudioOptions);

	CATEGORY("Volume")
	{
		const char* volumeName[] = { "Overall Volume:", "Music Volume:", "Sound Volume:" };
		for (int k = 0; k < 3; ++k)
		{
			configBuilder.addSetting(volumeName[k], (option::Option)(option::AUDIO_VOLUME + k))
				.addOption("Off", 0)
				.addNumberOptions(5, 100, 5, "%");
		}
	}

	CATEGORY("Soundtrack")
	{
		configBuilder.addSetting("Soundtrack Type:", option::SOUNDTRACK)
			.addOption("Emulated", 0)
			.addOption("Remastered", 1);

		configBuilder.addSetting("Sound Test:", option::SOUND_TEST);		// Will be filled with content in "OptionsMenu::initialize()"
	}

	CATEGORY("Theme Selection")
	{
		configBuilder.addSetting("Title Theme:", option::TITLE_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1);

		configBuilder.addSetting("1-up Jingle:", option::EXTRA_LIFE_JINGLE)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("Pick by Zone", 0x10);

		configBuilder.addSetting("Invincibility Theme:", option::INVINCIBILITY_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("Pick by Zone", 0x10);

		configBuilder.addSetting("Super/Hyper Theme:", option::SUPER_THEME)
			.addOption("Normal level music", 0)
			.addOption("Fast level music", 1)
			.addOption("Sonic 2", 2)
			.addOption("Sonic 3", 3)
			.addOption("Sonic & Knuckles", 4)
			.addOption("S3 Prototype", 5);

		configBuilder.addSetting("Mini-Boss Theme:", option::MINIBOSS_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("Pick by Zone", 0x10);

		configBuilder.addSetting("Knuckles' Theme:", option::KNUCKLES_THEME)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("S3 Prototype", 2)
			.addOption("Pick by Zone", 0x10);
	}

	CATEGORY("Level Music")
	{
		configBuilder.addSetting("Carnival Night Act 1:", option::LEVELMUSIC_CNZ1)
			.addOption("As Released", 0x00000001)
			.addOption("S3 Prototype", 0x80000001);

		configBuilder.addSetting("Carnival Night Act 2:", option::LEVELMUSIC_CNZ2)
			.addOption("As Released", 0x00000002)
			.addOption("S3 Prototype", 0x80000002);

		configBuilder.addSetting("IceCap Act 1:", option::LEVELMUSIC_ICZ1)
			.addOption("As Released", 0x00000001)
			.addOption("S3 Prototype", 0x80000001);

		configBuilder.addSetting("IceCap Act 2:", option::LEVELMUSIC_ICZ2)
			.addOption("As Released", 0x00000002)
			.addOption("S3 Prototype", 0x80000002);

		configBuilder.addSetting("Launch Base Act 1:", option::LEVELMUSIC_LBZ1)
			.addOption("As Released", 0x00000001)
			.addOption("S3 Prototype", 0x80000001);

		configBuilder.addSetting("Launch Base Act 2:", option::LEVELMUSIC_LBZ2)
			.addOption("As Released", 0x00000002)
			.addOption("S3 Prototype", 0x80000002);
	}

	CATEGORY("Music Selection")
	{
		configBuilder.addSetting("FBZ Laser Trap Boss:", option::FBZ2_MIDBOSS_TRACK)
			.addOption("Mini-Boss Music", 1)
			.addOption("Main Boss Music", 0);

		configBuilder.addSetting("In Hidden Palace:", option::HPZ_MUSIC)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1)
			.addOption("S3 + S&K Mini-Boss", 2)
			.addOption("S3 Prototype", 3);

		configBuilder.addSetting("Sky Sanctuary Bosses:", option::SSZ_BOSSTRACKS)
			.addOption("Normal Boss Music", 0)
			.addOption("Sonic 1 & 2 Tracks", 1);

		configBuilder.addSetting("Outro Music:", option::OUTRO_MUSIC)
			.addOption("Sky Sanctuary", 0)
			.addOption("Sonic 3 Credits", 1)
			.addOption("S3 Prototype", 2);

		configBuilder.addSetting("Competition Menu:", option::COMPETITION_MENU_MUSIC)
			.addOption("Sonic 3", 0)
			.addOption("S3 Prototype", 1);

		configBuilder.addSetting("Continue Screen:", option::CONTINUE_SCREEN_MUSIC)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1);
	}

	CATEGORY("Music Behavior")
	{
		configBuilder.addSetting("On Level (Re)Start:", option::CONTINUE_MUSIC)
			.addOption("Restart Music", 0)
			.addOption("Continue Music", 1);
	}

	CATEGORY("Effects")
	{
		configBuilder.addSetting("Underwater Sound:", option::UNDERWATER_AUDIO)
			.addOption("Normal", 0)
			.addOption("Muffled", 1);
	}
}

void OptionsConfig::buildVisuals()
{
	ConfigBuilder configBuilder(mVisualsOptions);

	CATEGORY("Visual Enhancements")
	{
		configBuilder.addSetting("Character Rotation:", option::ROTATION)
			.addOption("Original", 0)
			.addOption("Smooth", 1)
			.addOption("Mania-Accurate", 2);

		configBuilder.addSetting("Time Display:", option::TIME_DISPLAY)
			.addOption("Original", 0)
			.addOption("Extended", 1);

		configBuilder.addSetting("Lives Display:", option::LIVES_DISPLAY)
			.addOption("Auto", 0)
			.addOption("Classic", 1)
			.addOption("Mobile", 2);

		configBuilder.addSetting("Speed Shoes Effect:", option::SPEEDUP_AFTER_IMAGES)
			.addOption("None (Original)", 0)
			.addOption("After-Images", 1);

		configBuilder.addSetting("Fast Run Animation:", option::FAST_RUN_ANIM)
			.addOption("None (Original)", 0)
			.addOption("Peel-Out", 1);

		configBuilder.addSetting("Flicker Effects:", option::ANTI_FLICKER)
			.addOption("As Original", 0)
			.addOption("Slightly Smoothed", 1)
			.addOption("Heavily Smoothed", 2);
	}

	CATEGORY("Camera")
	{
		configBuilder.addSetting("Outrun Camera:", option::CAMERA_OUTRUN)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("Extended Camera:", option::EXTENDED_CAMERA)
			.addOption("Off", 0)
			.addOption("On", 1);
	}

	CATEGORY("Objects")
	{
		configBuilder.addSetting("Monitor Style:", option::MONITOR_STYLE)
			.addOption("Sonic 1 / 2", 1)
			.addOption("Sonic 3 & Knuckles", 0);
	}

	CATEGORY("Color Changes")
	{
		configBuilder.addSetting("IceCap Startup Time:", option::ICZ_NIGHTTIME)
			.addOption("Daytime", 0)
			.addOption("Morning Dawn", 1);
	}

	CATEGORY("Special Stages")
	{
		configBuilder.addSetting("Blue Spheres Style:", option::SPECIAL_STAGE_VISUALS)
			.addOption("Classic", 0)
			.addOption("Modernized", 3);

		configBuilder.addSetting("Ring Counter:", option::SPECIAL_STAGE_RING_COUNT)
			.addOption("Counting Up", 0)
			.addOption("Counting Down", 1);
	}
}

void OptionsConfig::buildGameplay()
{
	ConfigBuilder configBuilder(mGameplayOptions);

	CATEGORY("Levels")
	{
		configBuilder.addSetting("Level Layouts:", option::LEVEL_LAYOUTS)
			.addOption("Sonic 3", 0)
			.addOption("Sonic 3 & Knuckles", 1)
			.addOption("Sonic 3 A.I.R.", 2);
	}

	CATEGORY("Difficulty Changes")
	{
		configBuilder.addSetting("Angel Island Bombing:", option::AIZ_BLIMPSEQUENCE)
			.addOption("Original", 0)
			.addOption("Alternative", 1);

		configBuilder.addSetting("Big Arms Boss Fight:", option::LBZ_BIGARMS)
			.addOption("Only Knuckles", 0)
			.addOption("All characters", 1);

		configBuilder.addSetting("Sandopolis Ghosts:", option::SOZ_GHOSTSPAWN)
			.addOption("Disabled", 1)
			.addOption("Enabled", 0);

		configBuilder.addSetting("Lava Reef Act 2 Boss:", option::LRZ2_BOSS)
			.addOption("8 hits", 1)
			.addOption("14 hits (original)", 0);

		configBuilder.addSetting("Keep Shield after Zone:", option::MAINTAIN_SHIELDS)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);
	}

	CATEGORY("Time Attack")
	{
		configBuilder.addSetting("Max. Recorded Ghosts:", option::TIMEATTACK_GHOSTS)
			.addOption("Off", 0)
			.addOption("1", 1)
			.addOption("3", 3)
			.addOption("5", 5);

		configBuilder.addSetting("Quick Restart:", option::TIMEATTACK_INSTANTRESTART)
			.addOption("Hold Y Button", 0)
			.addOption("Press Y Button", 1);
	}
}

void OptionsConfig::buildControls()
{
	ConfigBuilder configBuilder(mControlsOptions);

	CATEGORY("Unlocked by Secrets")
	{
		configBuilder.addSetting("Sonic Drop Dash:", option::DROP_DASH)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("Sonic Super Peel-Out:", option::SUPER_PEELOUT)
			.addOption("Off", 0)
			.addOption("On", 1);
	}

	CATEGORY("Controllers")
	{
		configBuilder.addSetting("Setup Keyboard & Game Controllers...", option::CONTROLLER_SETUP);		// This text here won't be used, see rendering

		for (int k = 0; k < InputManager::NUM_PLAYERS; ++k)
		{
			configBuilder.addSetting(*String(0, "Controller Player %d", k+1), (option::Option)(option::CONTROLLER_PLAYER_1 + k));
			if (Application::instance().hasVirtualGamepad())
				configBuilder.addOption("None (Touch only)", -1);
			else
				configBuilder.addOption("None (Keyboard only)", -1);
			// Actual options will get filled in inside "OptionsMenu::refreshGamepadLists"
		}

		configBuilder.addSetting("Other controllers", option::CONTROLLER_AUTOASSIGN)
			.addOption("Not used", -1)
			.addOption("Assign to Player 1", 0)
			.addOption("Assign to Player 2", 1)
			.addOption("Assign to Player 3", 2)
			.addOption("Assign to Player 4", 3);
	}

	if (Application::instance().hasVirtualGamepad())
	{
		CATEGORY("Virtual Gamepad")
		{
			configBuilder.addSetting("Visibility:", option::VGAMEPAD_OPACITY).addNumberOptions(0, 100, 10, "%");
			configBuilder.addSetting("D-Pad Size:",	option::VGAMEPAD_DPAD_SIZE).addNumberOptions(50, 150, 10);
			configBuilder.addSetting("Buttons Size:", option::VGAMEPAD_BUTTONS_SIZE).addNumberOptions(50, 150, 10);
			configBuilder.addSetting("Set Touch Gamepad Layout...", option::VGAMEPAD_SETUP);
		}
	}

	CATEGORY("Controller Rumble")
	{
		for (int k = 0; k < InputManager::NUM_PLAYERS; ++k)
		{
			configBuilder.addSetting(*String(0, "Rumble Player %d", k+1), (option::Option)(option::CONTROLLER_RUMBLE_P1 + k));
			configBuilder.addOption("Off", 0);
			for (int i = 20; i <= 100; i += 20)
				configBuilder.addOption(*String(0, "%d %%", i), i);
		}
	}

	CATEGORY("Abilities")
	{
		configBuilder.addSetting("Sonic Insta-Shield:", option::INSTA_SHIELD)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("Tails Assist:", option::TAILS_ASSIST)
			.addOption("Off", 0)
			.addOption("Sonic 3 A.I.R. Style", 1)
			.addOption("Hybrid Style", 2)
			.addOption("Sonic Mania Style", 3);

		configBuilder.addSetting("Tails Flight Cancel:", option::TAILS_FLIGHT_CANCEL)
			.addOption("Off", 0)
			.addOption("Down + Jump", 1);

		configBuilder.addSetting("Roll Jump Control Lock:", option::NO_CONTROL_LOCK)
			.addOption("Locked (Classic)", 0)
			.addOption("Free Movement", 1);

		configBuilder.addSetting("Bubble Shield Bounce:", option::BUBBLE_SHIELD_BOUNCE)
			.addOption("Sonic 3 Style", 0)
			.addOption("Sonic Mania Style", 1);
	}

	CATEGORY("Super & Hyper Forms")
	{
		configBuilder.addSetting("Tails Super Forms:", option::HYPER_TAILS)
			.addOption("Only Super Tails", 0)
			.addOption("Super & Hyper Tails", 1);

		configBuilder.addSetting("Super Cancel:", option::SUPER_CANCEL)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("Super Sonic Jump Ability:", option::SUPER_SONIC_ABILITY)
			.addOption("None (Original)", 0)
			.addOption("Shield", 1)
			.addOption("Super Dash", 2);

		configBuilder.addSetting("Sonic Hyper Dash:", option::HYPER_DASH_CONTROLS)
			.addOption("As Original", 0)
			.addOption("Only when D-pad held", 1);
	}
}

void OptionsConfig::buildTweaks()
{
	ConfigBuilder configBuilder(mTweaksOptions);

	CATEGORY("Unlocked by Secrets")
	{
		configBuilder.addSetting("Debug Mode:", option::DEBUG_MODE)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("Title Screen:", option::TITLE_SCREEN)
			.addOption("Sonic 3", 0)
			.addOption("Sonic & Knuckles", 1);

		configBuilder.addSetting("Game Speed:", option::GAME_SPEED)
			.addOption("50 Hz (slower)", 50)
			.addOption("60 Hz (normal)", 60)
			.addOption("75 Hz (faster)", 75)
			.addOption("90 Hz (much faster)", 90)
			.addOption("120 Hz (ridiculous)", 120)
			.addOption("144 Hz (ludicrous)", 144);
	}

	CATEGORY("Accessibility")
	{
		configBuilder.addSetting("Infinite Lives:", option::INFINITE_LIVES)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);

		configBuilder.addSetting("Infinite Time:", option::INFINITE_TIME)
			.addOption("Disabled", 0)
			.addOption("Enabled", 1);
	}

	CATEGORY("Game Variety")
	{
		configBuilder.addSetting("Shields:", option::SHIELD_TYPES)
			.addOption("Classic Shield", 0)
			.addOption("Elemental Shields", 1)
			.addOption("Classic + Elemental", 2)
			.addOption("Upgradable Shields", 3);

		configBuilder.addSetting("Randomized Monitors:", option::RANDOM_MONITORS)
			.addOption("Normal Monitors", 0)
			.addOption("Random Shields", 1)
			.addOption("Random Monitors", 2);

		configBuilder.addSetting("Monitor Behavior:", option::MONITOR_BEHAVIOR)
			.addOption("Default", 0)
			.addOption("Fall down when hit", 1);

		configBuilder.addSetting("Hidden Monitors:", option::HIDDEN_MONITOR_HINT)
			.addOption("No hint", 0)
			.addOption("Sparkle near signpost", 1);
	}

	CATEGORY("Special Stages")
	{
		configBuilder.addSetting("Special Stage Layouts:", option::RANDOM_SPECIALSTAGES)
			.addOption("Original", 0)
			.addOption("Randomly Generated", 1);

		configBuilder.addSetting("On Fail:", option::SPECIAL_STAGE_REPEAT)
			.addOption("Advance to next", 0)
			.addOption("Do not advance", 1);
	}

	CATEGORY("Region")
	{
		configBuilder.addSetting("Region Code:", option::REGION)
			.addOption("Western (\"Tails\")", 0x80)
			.addOption("Japan (\"Miles\")", 0x00);
	}

	CATEGORY("Speedrunning")
	{
		configBuilder.addSetting("Glitch Fixes:", option::FIX_GLITCHES)
			.addOption("No glitch fixes", 0)
			.addOption("Only basic fixes", 1)
			.addOption("All (recommended)", 2);
	}

	CATEGORY("Other Enhancements")
	{
		configBuilder.addSetting("Object Pushing Speed:", option::FASTER_PUSH)
			.addOption("Original", 0)
			.addOption("Faster", 1);

		configBuilder.addSetting("Score Tally Speed-Up:", option::LEVELRESULT_SCORE)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("LBZ Tube Transport:", option::LBZ_TUBETRANSPORT)
			.addOption("Original Speed", 0)
			.addOption("Faster", 1);

		configBuilder.addSetting("MHZ Elevator:", option::MHZ_ELEVATOR)
			.addOption("Original Speed", 0)
			.addOption("Faster", 1);

		configBuilder.addSetting("FBZ Door Opening:", option::FBZ_SCREWDOORS)
			.addOption("Original Speed", 0)
			.addOption("Faster", 1);

		configBuilder.addSetting("SOZ Pyramid Rising:", option::SOZ_PYRAMID)
			.addOption("Original Speed", 0)
			.addOption("Faster", 1);

		configBuilder.addSetting("AIZ Knuckles Intro:", option::AIZ_INTRO_KNUCKLES)
			.addOption("Off", 0)
			.addOption("On", 1);

		configBuilder.addSetting("FBZ Cylinder Behavior:", option::FBZ_ENTERCYLINDER)
			.addOption("Original", 0)
			.addOption("Can enter from top", 1);

		configBuilder.addSetting("Offscreen Player 2:", option::PLAYER2_OFFSCREEN)
			.addOption("Not shown", 0)
			.addOption("Show at border", 1);
	}
}

#undef CATEGORY
