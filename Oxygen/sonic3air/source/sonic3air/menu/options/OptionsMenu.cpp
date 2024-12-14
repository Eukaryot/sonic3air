/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/options/OptionsMenu.h"
#include "sonic3air/menu/options/ControllerSetupMenu.h"
#include "sonic3air/menu/options/OptionsMenuEntries.h"
#include "sonic3air/menu/entries/GeneralMenuEntries.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/Game.h"
#include "sonic3air/version.inc"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/overlays/TouchControlsOverlay.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/helper/Utils.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/simulation/Simulation.h"


namespace
{
	struct ConditionalOption
	{
		int mOptionId = 0;
		bool mHideInGame = false;
		bool mDependsOnSecret = false;
		SharedDatabase::Secret::Type mSecret = (SharedDatabase::Secret::Type)0xff;

		inline ConditionalOption(int optionId, bool hideInGame) : mOptionId(optionId), mHideInGame(hideInGame) {}
		inline ConditionalOption(int optionId, bool hideInGame, SharedDatabase::Secret::Type secret) : mOptionId(optionId), mHideInGame(hideInGame), mDependsOnSecret(true), mSecret(secret) {}

		bool shouldBeVisible(bool enteredFromIngame) const
		{
			if (mHideInGame && enteredFromIngame)
				return false;
			if (mDependsOnSecret && !PlayerProgress::instance().mUnlocks.isSecretUnlocked(mSecret))
				return false;
			return true;
		}
	};

	// Hide certain options depending on:
	//  - whether the options menu is opened from the pause menu (second parameter)
	//  - and/or depending on secrets (third parameter)
	static const std::vector<ConditionalOption> CONDITIONAL_OPTIONS =
	{
		ConditionalOption(option::SOUNDTRACK,				 true),
		ConditionalOption(option::SOUNDTRACK_DOWNLOAD,		 true),
		ConditionalOption(option::SOUND_TEST,				 true),
		ConditionalOption(option::TITLE_THEME,				 true),
		ConditionalOption(option::OUTRO_MUSIC,				 true),
		ConditionalOption(option::COMPETITION_MENU_MUSIC,	 true),

		ConditionalOption(option::ANTI_FLICKER,				 true),
		ConditionalOption(option::ICZ_NIGHTTIME,			 true),
		ConditionalOption(option::MONITOR_STYLE,			 true),

		ConditionalOption(option::LEVEL_LAYOUTS,			 true),
		ConditionalOption(option::AIZ_BLIMPSEQUENCE,		 true),
		ConditionalOption(option::LBZ_BIGARMS,				 true),
		ConditionalOption(option::SOZ_GHOSTSPAWN,			 true),
		ConditionalOption(option::LRZ2_BOSS,				 true),
		ConditionalOption(option::TIMEATTACK_GHOSTS,		 true),
		ConditionalOption(option::TIMEATTACK_INSTANTRESTART, true),

		ConditionalOption(option::DROP_DASH, 				 false, SharedDatabase::Secret::SECRET_DROPDASH),
		ConditionalOption(option::SUPER_PEELOUT,			 false, SharedDatabase::Secret::SECRET_SUPER_PEELOUT),

		ConditionalOption(option::DEBUG_MODE,				 false, SharedDatabase::Secret::SECRET_DEBUGMODE),
		ConditionalOption(option::TITLE_SCREEN,				 true,  SharedDatabase::Secret::SECRET_TITLE_SK),
		ConditionalOption(option::SHIELD_TYPES,				 true),
		ConditionalOption(option::RANDOM_MONITORS,			 true),
		ConditionalOption(option::MONITOR_BEHAVIOR,			 true),
		ConditionalOption(option::RANDOM_SPECIALSTAGES,		 true),
		ConditionalOption(option::SPECIAL_STAGE_REPEAT,		 true),
		ConditionalOption(option::REGION,					 true),
		ConditionalOption(option::GAME_SPEED,				 false, SharedDatabase::Secret::SECRET_GAME_SPEED)
	};
}


OptionsMenu::OptionsMenu(MenuBackground& menuBackground) :
	mMenuBackground(&menuBackground)
{
	mScrolling.setVisibleAreaHeight(224 - 30);	// Do not count the 30 pixels of the tab title as scrolling area

	mOptionsConfig.build();
	mOptionEntries.resize(option::_NUM);
	{
		ConfigurationImpl& config = ConfigurationImpl::instance();

		setupOptionEntryInt(option::RELEASE_CHANNEL,			&config.mGameServerImpl.mUpdateCheck.mReleaseChannel);

		setupOptionEntryEnum8(option::FRAME_SYNC,				&config.mFrameSync);

		setupOptionEntryBool(option::GHOST_SYNC,				&config.mGameServerImpl.mGhostSync.mEnabled);
		setupOptionEntryInt(option::GHOST_SYNC_RENDERING,		&config.mGameServerImpl.mGhostSync.mGhostRendering);
		setupOptionEntryBool(option::SHOW_CONTROLS_DISPLAY,		&config.mShowControlsDisplay);
		setupOptionEntryInt(option::SCRIPT_OPTIMIZATION,		&config.mScriptOptimizationLevel);
		setupOptionEntryInt(option::GAME_RECORDING_MODE,		&config.mGameRecorder.mRecordingMode);
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
		setupOptionEntryPercent(option::CONTROLLER_RUMBLE_P1,	&config.mControllerRumbleIntensity[0]);
		setupOptionEntryPercent(option::CONTROLLER_RUMBLE_P2,	&config.mControllerRumbleIntensity[1]);

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
		setupOptionEntry(option::HYPER_DASH_CONTROLS,		SharedDatabase::Setting::SETTING_HYPER_DASH_CONTROLS);
		setupOptionEntry(option::SUPER_SONIC_ABILITY,		SharedDatabase::Setting::SETTING_SUPER_SONIC_ABILITY);
		setupOptionEntry(option::MONITOR_BEHAVIOR,			SharedDatabase::Setting::SETTING_MONITOR_BEHAVIOR);
		setupOptionEntry(option::MAINTAIN_SHIELDS,			SharedDatabase::Setting::SETTING_MAINTAIN_SHIELDS);
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
		setupOptionEntry(option::FBZ2_MIDBOSS_TRACK,		SharedDatabase::Setting::SETTING_FBZ2_MIDBOSS_TRACK);
		setupOptionEntry(option::SSZ_BOSSTRACKS,			SharedDatabase::Setting::SETTING_SSZ_BOSS_TRACKS);
		setupOptionEntry(option::OUTRO_MUSIC,				SharedDatabase::Setting::SETTING_AUDIO_OUTRO);
		setupOptionEntry(option::COMPETITION_MENU_MUSIC,	SharedDatabase::Setting::SETTING_AUDIO_COMPETITION_MENU);
		setupOptionEntry(option::CONTINUE_SCREEN_MUSIC,		SharedDatabase::Setting::SETTING_AUDIO_CONTINUE_SCREEN);
		setupOptionEntry(option::CONTINUE_MUSIC,			SharedDatabase::Setting::SETTING_CONTINUE_MUSIC);
		setupOptionEntry(option::UNDERWATER_AUDIO,			SharedDatabase::Setting::SETTING_UNDERWATER_AUDIO);
		setupOptionEntry(option::REGION,					SharedDatabase::Setting::SETTING_REGION_CODE);
		setupOptionEntry(option::TIMEATTACK_GHOSTS,			SharedDatabase::Setting::SETTING_TIME_ATTACK_GHOSTS);
		setupOptionEntry(option::FIX_GLITCHES,				SharedDatabase::Setting::SETTING_FIX_GLITCHES);
		setupOptionEntry(option::DROP_DASH,					SharedDatabase::Setting::SETTING_DROPDASH);
		setupOptionEntry(option::SUPER_PEELOUT,				SharedDatabase::Setting::SETTING_SUPER_PEELOUT);
		setupOptionEntry(option::DEBUG_MODE,				SharedDatabase::Setting::SETTING_DEBUG_MODE);
		setupOptionEntry(option::TITLE_SCREEN,				SharedDatabase::Setting::SETTING_TITLE_SCREEN);

		setupOptionEntryBitmask(option::LEVELMUSIC_CNZ1,	SharedDatabase::Setting::SETTING_CNZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_CNZ2,	SharedDatabase::Setting::SETTING_CNZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_ICZ1,	SharedDatabase::Setting::SETTING_ICZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_ICZ2,	SharedDatabase::Setting::SETTING_ICZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_LBZ1,	SharedDatabase::Setting::SETTING_LBZ_PROTOTYPE_MUSIC);
		setupOptionEntryBitmask(option::LEVELMUSIC_LBZ2,	SharedDatabase::Setting::SETTING_LBZ_PROTOTYPE_MUSIC);
	}

	// Build up tab menu entries
	mTabMenuEntries.addEntry<OptionsMenuEntry>().initEntry("", option::_TAB_SELECTION)
		.addOption("MODS",     Tab::Id::MODS)
		.addOption("SYSTEM",   Tab::Id::SYSTEM)
		.addOption("DISPLAY",  Tab::Id::DISPLAY)
		.addOption("AUDIO",    Tab::Id::AUDIO)
		.addOption("VISUALS",  Tab::Id::VISUALS)
		.addOption("GAMEPLAY", Tab::Id::GAMEPLAY)
		.addOption("CONTROLS", Tab::Id::CONTROLS)
		.addOption("TWEAKS",   Tab::Id::TWEAKS);

	for (int i = 0; i < Tab::Id::_NUM; ++i)
	{
		GameMenuEntries& entries = mTabs[i].mMenuEntries;
		entries.reserve(20);
		entries.addEntry();		// Dummy entry representing the title in menu navigation
	}

	// Mods tab needs to be rebuilt each time again, see "initialize"

	createOptionMenuTab(Tab::Id::SYSTEM,	mOptionsConfig.mSystemOptions);
	createOptionMenuTab(Tab::Id::DISPLAY,	mOptionsConfig.mDisplayOptions);
	createOptionMenuTab(Tab::Id::AUDIO,		mOptionsConfig.mAudioOptions);
	createOptionMenuTab(Tab::Id::VISUALS,	mOptionsConfig.mVisualsOptions);
	createOptionMenuTab(Tab::Id::GAMEPLAY,	mOptionsConfig.mGameplayOptions);
	createOptionMenuTab(Tab::Id::CONTROLS,	mOptionsConfig.mControlsOptions);
	createOptionMenuTab(Tab::Id::TWEAKS,	mOptionsConfig.mTweaksOptions);

	// Post-processing
	for (int i = 1; i < Tab::Id::_NUM; ++i)		// Exclude Mods tab
	{
		GameMenuEntries& entries = mTabs[i].mMenuEntries;
		entries.addEntry<OptionsMenuEntry>().initEntry("Back", option::_BACK);

		for (size_t k = 0; k < entries.size(); ++k)
		{
			GameMenuEntry& entry = entries[k];
			OptionEntry& optionEntry = mOptionEntries[entry.mData];
			optionEntry.mOptionId = (option::Option)entry.mData;
			optionEntry.mGameMenuEntry = &entry;
		}
	}

	{
		// Gather unlockable entries
		const std::vector<option::Option> unlockables[2] =
		{
			{ option::DROP_DASH, option::SUPER_PEELOUT },						// Controls tab
			{ option::DEBUG_MODE, option::TITLE_SCREEN, option::GAME_SPEED }	// Tweaks tab
		};
		for (int k = 0; k < 2; ++k)
		{
			for (option::Option unlockable : unlockables[k])
			{
				mUnlockedSecretsEntries[k].push_back(mOptionEntries[unlockable].mGameMenuEntry);
			}
		}

		// Gather gamepad assignment entries
		mGamepadAssignmentEntries[0] = mOptionEntries[option::CONTROLLER_PLAYER_1].mGameMenuEntry;
		mGamepadAssignmentEntries[1] = mOptionEntries[option::CONTROLLER_PLAYER_2].mGameMenuEntry;
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
		case State::FADE_TO_GAME:  return BaseState::FADE_OUT;
		default:				   return BaseState::INACTIVE;
	}
}

void OptionsMenu::setBaseState(BaseState baseState)
{
	switch (baseState)
	{
		case BaseState::INACTIVE: mState = State::INACTIVE;  break;
		case BaseState::FADE_IN:  mState = State::APPEAR;  break;
		case BaseState::SHOW:	  mState = State::SHOW;  break;
		case BaseState::FADE_OUT: mState = State::FADE_TO_MENU;  break;
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
		addChild(*mControllerSetupMenu);
	}

	// Rebuild mods tab & mods option entries
	createModsTab();

	mSoundtrackDownloadMenuEntry->setVisible(mSoundtrackDownloadMenuEntry->shouldBeShown());

	// Fill sound test
	{
		mSoundTestAudioDefinitions.clear();
		const bool devModeEnabled = Configuration::instance().mDevMode.mEnabled;
		const auto& audioDefinitions = AudioOut::instance().getAudioCollection().getAudioDefinitions();
		for (const auto& [key, audioDefinition] : audioDefinitions)
		{
			bool visible = false;
			const AudioCollection::AudioDefinition::Visibility visibility = audioDefinition.mSoundTestVisibility;
			switch (visibility)
			{
				case AudioCollection::AudioDefinition::Visibility::ALWAYS_HIDDEN:	visible = false;  break;
				case AudioCollection::AudioDefinition::Visibility::ALWAYS_VISIBLE:	visible = true;   break;
				case AudioCollection::AudioDefinition::Visibility::DEV_MODE_ONLY:	visible = devModeEnabled;  break;

				default:
				{
					// By default, show music & jingles only, not sound effects
					if (audioDefinition.mType == AudioCollection::AudioDefinition::Type::MUSIC || audioDefinition.mType == AudioCollection::AudioDefinition::Type::JINGLE)
					{
						// Hide fast tracks
						visible = (devModeEnabled || !rmx::endsWith(audioDefinition.mKeyString, "_fast"));
					}
					break;
				}
			}

			if (visible)
			{
				mSoundTestAudioDefinitions.emplace_back(&audioDefinition);
			}
		}

		std::sort(mSoundTestAudioDefinitions.begin(), mSoundTestAudioDefinitions.end(),
			[](const AudioCollection::AudioDefinition* a, const AudioCollection::AudioDefinition* b) { return a->mKeyString < b->mKeyString; });

		GameMenuEntry& entry = *mOptionEntries[option::SOUND_TEST].mGameMenuEntry;
		entry.mOptions.clear();
		for (size_t index = 0; index < mSoundTestAudioDefinitions.size(); ++index)
		{
			entry.addOption(mSoundTestAudioDefinitions[index]->mKeyString, (uint32)index);
		}
		entry.sanitizeSelectedIndex();
	}

	// Fill gamepad lists
	refreshGamepadLists(true);

	mEnteredFromIngame = false;
	mOriginalScriptOptimizationLevel = Configuration::instance().mScriptOptimizationLevel;
}

void OptionsMenu::deinitialize()
{
	GameMenuBase::deinitialize();
}

void OptionsMenu::keyboard(const rmx::KeyboardEvent& ev)
{
	GameMenuEntry* entry = getSelectedGameMenuEntry();
	if (nullptr != entry)
		entry->keyboard(ev);

	GameMenuBase::keyboard(ev);
}

void OptionsMenu::textinput(const rmx::TextInputEvent& ev)
{
	GameMenuEntry* entry = getSelectedGameMenuEntry();
	if (nullptr != entry)
		entry->textinput(ev);
}

void OptionsMenu::update(float timeElapsed)
{
	mDeltaSecondsForRendering += timeElapsed;
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
		mOptionEntries[option::RENDERER].mGameMenuEntry->setSelectedIndexByValue((int)config.mRenderMethod);

		if (mActiveMenu == &mTabMenuEntries && (keys.Down.justPressedOrRepeat() || keys.Up.justPressedOrRepeat()))
		{
			// Switch from title to tab content
			mActiveMenu = &mTabs[mActiveTab].mMenuEntries;
			mActiveMenu->mSelectedEntryIndex = 0;
		}

		if (keys.X.hasChanged())
		{
			refreshControlsDisplay();
		}

		GameMenuEntries::UpdateResult result = GameMenuEntries::UpdateResult::NONE;
		if (keys.X.isPressed() && (keys.Up.justPressedOrRepeat() || keys.Down.justPressedOrRepeat()) && mActiveMenu->mNavigation == GameMenuEntries::Navigation::VERTICAL)
		{
			// Quick navigation while holding X
			const int entryChange = mActiveMenu->getEntryChangeByInput();
			if (entryChange != 0)
			{
				for (int tries = 0; tries < 100; ++tries)
				{
					if (!mActiveMenu->changeSelectedIndex(entryChange, false))
						break;

					// Stop when reaching certain types of entries
					if (mActiveMenu->mSelectedEntryIndex <= 0 || mActiveMenu->selected().mData == option::_BACK || mActiveMenu->selected().getMenuEntryType() == ModTitleMenuEntry::MENU_ENTRY_TYPE)
						break;

					// Also stop at the first entry of each category (including first option of a mods without any categories)
					const size_t previousIndex = mActiveMenu->getPreviousInteractableIndex(mActiveMenu->mSelectedEntryIndex);
					bool hasTitleEntryInBetween = false;
					for (size_t k = previousIndex; k < (size_t)mActiveMenu->mSelectedEntryIndex; ++k)
					{
						if (mActiveMenu->getEntries()[k]->getMenuEntryType() == TitleMenuEntry::MENU_ENTRY_TYPE ||
							mActiveMenu->getEntries()[k]->getMenuEntryType() == ModTitleMenuEntry::MENU_ENTRY_TYPE)
						{
							hasTitleEntryInBetween = true;
							break;
						}
					}
					if (hasTitleEntryInBetween)
						break;
				}
				result = GameMenuEntries::UpdateResult::ENTRY_CHANGED;
			}
		}
		else
		{
			// Update menu entries
			result = mActiveMenu->update();
		}

		if (result != GameMenuEntries::UpdateResult::NONE)
		{
			if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED && mActiveMenu == &mTabMenuEntries)
			{
				mActiveTab = mTabMenuEntries[option::_TAB_SELECTION].mSelectedIndex;
				playMenuSound(0xb7);
				refreshControlsDisplay();
			}
			else
			{
				playMenuSound(0x5b);

				if (result == GameMenuEntries::UpdateResult::ENTRY_CHANGED)
				{
					if (mActiveMenu != &mTabMenuEntries && mActiveMenu->mSelectedEntryIndex == 0)
					{
						// Switch from tab content to title
						mActiveMenu = &mTabMenuEntries;
					}
					refreshControlsDisplay();
				}
				else if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED && mActiveMenu != &mTabMenuEntries)
				{
					GameMenuEntry& selectedEntry = mActiveMenu->selected();
					if (selectedEntry.getMenuEntryType() == ModTitleMenuEntry::MENU_ENTRY_TYPE)
					{
						updateModExpandState(static_cast<ModTitleMenuEntry&>(selectedEntry));
						refreshControlsDisplay();
					}
					else
					{
						const uint32 selectedData = selectedEntry.mData;
						switch (selectedData)
						{
							case option::RELEASE_CHANNEL:
							{
								mOptionEntries[selectedData].applyValue();
								GameClient::instance().getUpdateCheck().reset();
								break;
							}

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
								if (AudioOut::instance().hasLoadedRemasteredSoundtrack())
								{
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
								}
								mSoundtrackDownloadMenuEntry->setVisible(mSoundtrackDownloadMenuEntry->shouldBeShown());
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

							case option::GAME_RECORDING_MODE:
							{
								mOptionEntries[selectedData].applyValue();
								Configuration::instance().evaluateGameRecording();
								break;
							}

							default:
							{
								// Apply change
								config.mWindowMode = (Configuration::WindowMode)mOptionEntries[option::WINDOW_MODE_STARTUP].mGameMenuEntry->selected().mValue;

								if (selectedData > option::_TAB_SELECTION && selectedData != option::_BACK)
								{
									mOptionEntries[selectedData].applyValue();

									if (selectedData >= option::CONTROLLER_RUMBLE_P1 && selectedData <= option::CONTROLLER_RUMBLE_P2)
									{
										InputManager::instance().setControllerRumbleForPlayer(selectedData - option::CONTROLLER_RUMBLE_P1, 1.0f, 1.0f, 300);
									}
									else if (selectedData >= option::VGAMEPAD_DPAD_SIZE && selectedData <= option::VGAMEPAD_BUTTONS_SIZE)
									{
										TouchControlsOverlay::instance().buildTouchControls();
									}
									else if (selectedData == option::FRAME_SYNC)
									{
										EngineMain::instance().setVSyncMode((Configuration::FrameSyncType)selectedEntry.selected().mValue);
									}
								}
								if (mEnteredFromIngame && !mShowedAudioWarningMessage && selectedData >= option::TITLE_THEME && selectedData <= option::OUTRO_MUSIC)
								{
									mAudioWarningMessageTimeout = 4.0f;
									mShowedAudioWarningMessage = true;
								}
								break;
							}
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
		const ButtonEffect buttonEffect = (keys.Start.justPressed() || keys.A.justPressed()) ? ButtonEffect::ACCEPT :
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
				GameMenuEntry& selectedEntry = tab.mMenuEntries.selected();

				if (selectedEntry.getMenuEntryType() == ModTitleMenuEntry::MENU_ENTRY_TYPE)
				{
					selectedEntry.mSelectedIndex = 1 - selectedEntry.mSelectedIndex;
					updateModExpandState(static_cast<ModTitleMenuEntry&>(selectedEntry));
				}
				else
				{
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

						case option::_CHECK_FOR_UPDATE:
						case option::RELEASE_CHANNEL:
						{
							UpdateCheck& updateCheck = GameClient::instance().getUpdateCheck();
							if (updateCheck.hasUpdate())
							{
								PlatformFunctions::openURLExternal(updateCheck.getResponse()->mUpdateInfoURL.empty() ? "https://sonic3air.org" : updateCheck.getResponse()->mUpdateInfoURL);
							}
							else
							{
								updateCheck.startUpdateCheck();
							}
							break;
						}

						case option::SOUNDTRACK_DOWNLOAD:
						{
							mSoundtrackDownloadMenuEntry->triggerButton();
							mSoundtrackDownloadMenuEntry->setVisible(mSoundtrackDownloadMenuEntry->shouldBeShown());

							// Restart music if remastered soundtrack was just loaded
							if (AudioOut::instance().hasLoadedRemasteredSoundtrack())
							{
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
							}
							break;
						}

						case option::_OPEN_HOMEPAGE:
						{
							PlatformFunctions::openURLExternal("https://sonic3air.org/");
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
	}

	// Enable / disable options
	//  -> Done here as the conditions can change at any time (incl. hotkeys)
	const bool isSoftware = (Configuration::instance().mRenderMethod == Configuration::RenderMethod::SOFTWARE);
	mOptionEntries[option::SCANLINES].mGameMenuEntry->setInteractable(!isSoftware && Configuration::instance().mFiltering < 3);
	mOptionEntries[option::FILTERING].mGameMenuEntry->setInteractable(!isSoftware);

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
			GameApp::instance().onFadedOutOptions();
			mState = State::INACTIVE;
		}
	}

	// Update warning message timeout
	if (mWarningMessageTimeout > 0.0f)
	{
		mWarningMessageTimeout = std::max(0.0f, mWarningMessageTimeout - timeElapsed);
	}
	if (mAudioWarningMessageTimeout > 0.0f)
	{
		mAudioWarningMessageTimeout = std::max(0.0f, mAudioWarningMessageTimeout - timeElapsed);
	}

	// Check for changes in connected gamepads
	refreshGamepadLists();

	// Uodate children at the end
	GameMenuBase::update(timeElapsed);
}

void OptionsMenu::render()
{
	Drawer& drawer = EngineMain::instance().getDrawer();

	OptionsMenuRenderContext renderContext;
	renderContext.mOptionsMenu = this;
	renderContext.mDrawer = &drawer;
	renderContext.mDeltaSeconds = std::min(mDeltaSecondsForRendering, 0.1f);
	mDeltaSecondsForRendering = 0.0f;

	int anchorX = 200;
	int anchorY = 0;
	float alpha = 1.0f;
	if (mState != State::SHOW && mState != State::FADE_TO_GAME)
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

		// Tab contents
		{
			drawer.pushScissor(Recti(0, anchorY + 30, (int)mRect.width, (int)mRect.height - anchorY - 30));

			const int minTabIndex = (int)std::floor(mActiveTabAnimated);
			const int maxTabIndex = (int)std::ceil(mActiveTabAnimated);

			for (int tabIndex = minTabIndex; tabIndex <= maxTabIndex; ++tabIndex)
			{
				Tab& tab = mTabs[tabIndex];
				const bool isModsTab = (tabIndex == Tab::Id::MODS);
				const float tabAlpha = alpha * (1.0f - std::fabs(tabIndex - mActiveTabAnimated));
				const int baseX = anchorX + roundToInt((tabIndex - mActiveTabAnimated) * 250);

				renderContext.mCurrentPosition.set(baseX, startY + 12);
				renderContext.mTabAlpha = tabAlpha;
				renderContext.mIsModsTab = isModsTab;

				for (size_t line = 1; line < tab.mMenuEntries.size(); ++line)
				{
					GameMenuEntry& entry = tab.mMenuEntries[line];
					if (!entry.isVisible())
					{
						// Skip hidden entries
						continue;
					}

					if (entry.getMenuEntryType() == TitleMenuEntry::MENU_ENTRY_TYPE)
					{
						if (!isTitleShown(tabIndex, (int)line))		// TODO: This check might be obsolete now thanks to the else part below (but that needs to be tested first)
						{
							// Skip this title
							continue;
						}
						else
						{
							// Automatically skip titles that don't have any real option below them
							bool valid = false;
							for (size_t nextLine = line + 1; nextLine < tab.mMenuEntries.size(); ++nextLine)
							{
								const GameMenuEntry& nextEntry = tab.mMenuEntries[nextLine];
								if (nextEntry.getMenuEntryType() == TitleMenuEntry::MENU_ENTRY_TYPE || nextEntry.mData == option::_BACK)
									break;
								if (nextEntry.isFullyInteractable())
								{
									valid = true;
									break;
								}
							}
							if (!valid)
								continue;
						}
					}

					const int currentAbsoluteY1 = renderContext.mCurrentPosition.y - startY;
					renderContext.mIsSelected = (mActiveMenu == &tab.mMenuEntries && (int)line == tab.mMenuEntries.mSelectedEntryIndex);

					// Render this game menu entry
					entry.performRenderEntry(renderContext);

					if (renderContext.mIsSelected)
					{
						// TODO: Add back in that selecting the first interactable entry scrolls up to the top
						const int currentAbsoluteY2 = renderContext.mCurrentPosition.y - startY;
						mScrolling.setCurrentSelection(currentAbsoluteY1 - 30, currentAbsoluteY2 + 45);
					}

					renderContext.mCurrentPosition.y += isModsTab ? 13 : 16;
				}
			}

			drawer.popScissor();
		}

		// Tab titles (must be rendered afterwards because it's meant to be on top)
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
					drawer.printText(global::mSonicFontC, Recti(px, py, 160, 20), text, 5, color2);
				}
			}

			if (canGoLeft)
				drawer.printText(global::mOxyfontRegular, Recti(center - arrowDistance, py + 6, 0, 10), "<", 5, color);
			if (canGoRight)
				drawer.printText(global::mOxyfontRegular, Recti(center + arrowDistance, py + 6, 0, 10), ">", 5, color);

			if (isSelected)
			{
				mScrolling.setCurrentSelection(0, py);
			}
		}

		int bottomY = 224;
		if (ConfigurationImpl::instance().mShowControlsDisplay)
		{
			bottomY = 230 - roundToInt(alpha * 16.0f);
			//const int innerIndentX = (global::mDataSelectBackground.getWidth() - 400) / 2;
			//for (int k = 0; k < 5; ++k)
			//{
			//	const int y = bottomY + k - 3;
			//	drawer.drawRect(Recti(0, y, 400, 1), global::mDataSelectBackground, Recti(innerIndentX, y, 400, 1), Color(1.0f, 1.0f, 1.0f, ((float)k + 0.5f) * 0.2f));
			//}
		}

		if (mEnteredFromIngame)
		{
			const char* message = nullptr;
			float visibility = 0.0f;

			if (mAudioWarningMessageTimeout > 0.0f)
			{
				visibility = saturate(mAudioWarningMessageTimeout / 0.3f);
				message = "Note: Music changes don't affect already playing tracks.";
			}
			else if (mWarningMessageTimeout > 0.0f)
			{
				visibility = saturate(mWarningMessageTimeout / 0.3f);
				message = "Note: Some options are hidden while in-game.";
			}

			if (visibility > 0.0f && nullptr != message)
			{
				bottomY -= 16;
				const Recti rect(0, bottomY + roundToInt((1.0f - visibility) * 20.0f), 400, 20);

				drawer.drawRect(rect, Color(1.0f, 0.75f, 0.5f, alpha * 0.95f));
				drawer.printText(global::mOxyfontSmall, rect - Vec2i(0, 2), message, 5, Color(1.0f, 0.9f, 0.8f, alpha));
				drawer.drawRect(Recti(rect.x, rect.y-1, rect.width, 1), Color(0.4f, 0.2f, 0.0f, alpha * 0.95f));
				drawer.drawRect(Recti(rect.x, rect.y-2, rect.width, 1), Color(0.9f, 0.9f, 0.9f, alpha * 0.9f));
				drawer.drawRect(Recti(rect.x, rect.y-3, rect.width, 1), Color(0.9f, 0.9f, 0.9f, alpha * 0.6f));
				drawer.drawRect(Recti(rect.x, rect.y-4, rect.width, 1), Color(0.9f, 0.9f, 0.9f, alpha * 0.3f));
			}
		}

		if (ConfigurationImpl::instance().mShowControlsDisplay)
		{
			mGameMenuControlsDisplay.render(drawer, alpha);
		}

		drawer.performRendering();
	}

	// Render children on top
	GameMenuBase::render();
}

void OptionsMenu::setupOptionsMenu(bool enteredFromIngame)
{
	mEnteredFromIngame = enteredFromIngame;

	for (const ConditionalOption& option : CONDITIONAL_OPTIONS)
	{
		OptionsMenuEntry& optionsMenuEntry = *static_cast<OptionsMenuEntry*>(mOptionEntries[option.mOptionId].mGameMenuEntry);
		const bool visible = option.shouldBeVisible(enteredFromIngame) && optionsMenuEntry.shouldBeShown();
		optionsMenuEntry.setVisible(visible);
	}

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_WEB)
	// These options don't work on Android, so hide them
	mOptionEntries[option::WINDOW_MODE].mGameMenuEntry->setVisible(false);
	mOptionEntries[option::WINDOW_MODE_STARTUP].mGameMenuEntry->setVisible(false);
#endif

	// Hide Mods and System tabs
	mTabMenuEntries[0].mOptions[Tab::Id::MODS].mVisible = !enteredFromIngame && mHasAnyModOptions;
	mTabMenuEntries[0].mOptions[Tab::Id::SYSTEM].mVisible = !enteredFromIngame;

	// Corrections in case a now hidden entry was previously selected
	{
		mTabMenuEntries[0].sanitizeSelectedIndex();
		mActiveTab = mTabMenuEntries[0].mSelectedIndex;
		mActiveTabAnimated = (float)mActiveTab;

		mTabs[mActiveTab].mMenuEntries.sanitizeSelectedIndex();
		if (mTabs[mActiveTab].mMenuEntries.mSelectedEntryIndex == 0)
		{
			mActiveMenu = &mTabMenuEntries;
		}
	}

	mWarningMessageTimeout = enteredFromIngame ? 4.0f : 0.0f;
	mAudioWarningMessageTimeout = 0.0f;
	mShowedAudioWarningMessage = false;

	refreshControlsDisplay();
}

void OptionsMenu::removeControllerSetupMenu()
{
}

const AudioCollection::AudioDefinition* OptionsMenu::getSoundTestAudioDefinition(uint32 index) const
{
	return ((size_t)index < mSoundTestAudioDefinitions.size()) ? mSoundTestAudioDefinitions[index] : nullptr;
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

void OptionsMenu::setupOptionEntryBool(option::Option optionId, bool* valuePointer)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::CONFIG_BOOL;
	optionEntry.mValuePointer = valuePointer;
}

void OptionsMenu::setupOptionEntryEnum8(option::Option optionId, void* valuePointer)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::CONFIG_ENUM_8;
	optionEntry.mValuePointer = valuePointer;
}

void OptionsMenu::setupOptionEntryPercent(option::Option optionId, float* valuePointer)
{
	OptionEntry& optionEntry = mOptionEntries[optionId];
	optionEntry.mOptionId = optionId;
	optionEntry.mType = OptionEntry::Type::CONFIG_PERCENT;
	optionEntry.mValuePointer = valuePointer;
}

void OptionsMenu::createOptionMenuTab(Tab::Id tabId, const OptionsConfig::OptionsTab& optionsTab)
{
	GameMenuEntries& entries = mTabs[tabId].mMenuEntries;

	// Go through all categories
	for (const OptionsConfig::OptionsCategory& category : optionsTab.mCategories)
	{
		entries.addEntry<TitleMenuEntry>().initEntry(category.mName);

		// Go through all settings in that category
		for (const OptionsConfig::Setting& setting : category.mSettings)
		{
			createOptionMenuEntry(entries, setting);
		}
	}
}

void OptionsMenu::createOptionMenuEntry(GameMenuEntries& entries, const OptionsConfig::Setting& setting)
{
	// Some settings require special handling, though most use the default
	switch (setting.mKey)
	{
		case option::_CHECK_FOR_UPDATE:
		{
			entries.addEntry<UpdateCheckMenuEntry>().initEntry(setting.mName, option::_CHECK_FOR_UPDATE);
			entries.addEntry<OptionsMenuEntry>()
				.setUseSmallFont(true)
				.initEntry("", option::RELEASE_CHANNEL)
				.addOptions(setting);
			break;
		}

		case option::GHOST_SYNC:
		{
			entries.addEntry<LabelMenuEntry>().initEntry("If enabled, Ghost Sync shares your position in the game and\nshows all other players online in the same stage as ghosts.", Color(0.8f, 0.8f, 1.0f));
			entries.addEntry<OptionsMenuEntry>()
				.setUseSmallFont(true)
				.initEntry(setting.mName, option::GHOST_SYNC)
				.addOptions(setting);
			break;
		}

		case option::GHOST_SYNC_RENDERING:
		{
			entries.addEntry<OptionsMenuEntry>()
				.setUseSmallFont(true)
				.initEntry("Ghost Display", option::GHOST_SYNC_RENDERING)
				.addOptions(setting);

			// TEST
			//  -> TODO: Needs support for a label text like "Channel" and possibly some explanation text as well
			//entries.addEntry<InputFieldMenuEntry>().initEntry(L"world");
			break;
		}

		case option::SCRIPT_OPTIMIZATION:
		{
			entries.addEntry<LabelMenuEntry>().initEntry("These settings are meant only for debugging very specific issues.\nIt's recommended to leave them at their default values.", Color(1.0f, 0.8f, 0.6f));

			GameMenuEntry& entry = entries.addEntry<AdvancedOptionMenuEntry>()
				.setDefaultValue(-1)
				.initEntry(setting.mName, option::SCRIPT_OPTIMIZATION)
				.addOptions(setting);
			break;
		}

		case option::GAME_RECORDING_MODE:
		{
			GameMenuEntry& entry = entries.addEntry<AdvancedOptionMenuEntry>()
				.setDefaultValue(-1)
				.initEntry(setting.mName, option::GAME_RECORDING_MODE)
				.addOptions(setting);
			break;
		}

		case option::SOUNDTRACK:
		{
			entries.addEntry<SoundtrackMenuEntry>().initEntry(setting.mName, option::SOUNDTRACK)
				.addOptions(setting);

			mSoundtrackDownloadMenuEntry = &entries.addEntry<SoundtrackDownloadMenuEntry>();
			mSoundtrackDownloadMenuEntry->initEntry("", option::SOUNDTRACK_DOWNLOAD);
			break;
		}

		default:
		{
			// Default handling: Create a simple menu entry for that setting
			entries.addEntry<OptionsMenuEntry>().initEntry(setting.mName, setting.mKey)
				.addOptions(setting);
			break;
		}
	}
}

void OptionsMenu::createModsTab()
{
	uint32 nextOptionId = option::_NUM + 1;
	mOptionEntries.resize(nextOptionId);

	Tab& tab = mTabs[Tab::Id::MODS];
	GameMenuEntries& entries = tab.mMenuEntries;
	entries.resize(1);

	const std::vector<Mod*>& activeMods = ModManager::instance().getActiveMods();
	for (int modIndex = (int)activeMods.size() - 1; modIndex >= 0; --modIndex)
	{
		Mod* mod = activeMods[modIndex];
		if (mod->mSettingCategories.empty())
			continue;

		// Check if mod  is expanded; if there's only one mod, that one is always expanded by default
		const bool visible = (mExpandedMods.count(mod->mLocalDirectoryHash) > 0 || activeMods.size() == 1);
		{
			ModTitleMenuEntry& entry = entries.addEntry<ModTitleMenuEntry>().initEntry(*mod);
			entry.addOption("", 0);		// Collapsed
			entry.addOption("", 1);		// Expanded
			entry.mSelectedIndex = visible ? 1 : 0;
			entry.mMarginBelow = visible ? 0 : -10;
		}
		bool isFirstTitle = true;

		for (Mod::SettingCategory& modSettingCategory : mod->mSettingCategories)
		{
			// Check for category change, and add a title if needed
			const std::string* titleText = nullptr;
			if (modSettingCategory.mDisplayName.empty())
			{
				if (mod->mSettingCategories.size() >= 2)
				{
					static const std::string OTHER_SETTINGS = "Other Settings";
					titleText = &OTHER_SETTINGS;
				}
			}
			else
			{
				titleText = &modSettingCategory.mDisplayName;
			}
			if (nullptr != titleText)
			{
				// Add title
				TitleMenuEntry& entry = entries.addEntry<TitleMenuEntry>().initEntry(*titleText);
				entry.setVisible(visible);
				entry.mMarginBelow += 3;
				if (isFirstTitle)
				{
					entry.mMarginAbove -= 11;
					isFirstTitle = false;
				}
			}

			for (Mod::Setting& modSetting : modSettingCategory.mSettings)
			{
				GameMenuEntry& entry = entries.addEntry<OptionsMenuEntry>().initEntry(modSetting.mDisplayName, nextOptionId);
				for (const Mod::Setting::Option& option : modSetting.mOptions)
				{
					entry.addOption(option.mDisplayName, option.mValue);
				}
				entry.setVisible(visible);

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
		GameMenuEntry& entry = entries[k];
		if (entry.getMenuEntryType() != ModTitleMenuEntry::MENU_ENTRY_TYPE)
		{
			OptionEntry& optionEntry = mOptionEntries[entry.mData];
			optionEntry.mGameMenuEntry = &entry;
		}
	}

	entries.addEntry<OptionsMenuEntry>().initEntry("Back", option::_BACK);

	mHasAnyModOptions = (nextOptionId > option::_NUM + 1);
	mTabMenuEntries[0].mSelectedIndex = mActiveTab;
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
			GameMenuEntry& entry = *mGamepadAssignmentEntries[playerIndex];
			const int32 preferredValue = InputManager::instance().getPreferredGamepadByJoystickInstanceId(playerIndex);
			const uint32 oldSelectedValue = (preferredValue >= 0) ? (uint32)preferredValue : entry.hasSelected() ? entry.selected().mValue : (uint32)-1;
			entry.mOptions.resize(1);	// First entry is the "None" entry

			for (const InputManager::RealDevice& gamepad : InputManager::instance().getGamepads())
			{
				std::string text = gamepad.getName();
				utils::shortenTextToFit(text, global::mOxyfontRegular, 135);
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
	// Special handling for first titles in Gameplay and Tweaks tabs, if no unlocks are available there yet
	if (line != 1)
		return true;

	const int index = (tabIndex == Tab::Id::CONTROLS) ? 0 : (tabIndex == Tab::Id::TWEAKS) ? 1 : -1;
	if (index == -1)
		return true;

	for (auto* entry : mUnlockedSecretsEntries[index])
	{
		if (entry->isFullyInteractable())
			return true;
	}
	return false;
}

GameMenuEntry* OptionsMenu::getSelectedGameMenuEntry()
{
	if (mActiveTab < Tab::Id::_NUM)
	{
		GameMenuEntries& menuEntries = mTabs[mActiveTab].mMenuEntries;
		if (menuEntries.mSelectedEntryIndex >= 0 && menuEntries.mSelectedEntryIndex < (int)menuEntries.size())
		{
			return &menuEntries[menuEntries.mSelectedEntryIndex];
		}
	}
	return nullptr;
}

void OptionsMenu::refreshControlsDisplay()
{
	GameMenuEntry& selectedEntry = mActiveMenu->selected();
	mGameMenuControlsDisplay.clear();

	const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);
	if (keys.X.isPressed())
	{
		mGameMenuControlsDisplay.addControl("Prev / Next category", false, "@input_icon_button_up", "@input_icon_button_down");
	}
	else
	{
		if (mActiveMenu == &mTabMenuEntries)
		{
			mGameMenuControlsDisplay.addControl("Switch Tab", false, "@input_icon_button_left", "@input_icon_button_right");
		}
		else if (selectedEntry.getMenuEntryType() == ModTitleMenuEntry::MENU_ENTRY_TYPE)
		{
			if (selectedEntry.mSelectedIndex == 0)
				mGameMenuControlsDisplay.addControl("Expand", false, "@input_icon_button_right");
			else
				mGameMenuControlsDisplay.addControl("Collapse", false, "@input_icon_button_left");
		}
		else if (selectedEntry.mOptions.size() >= 2)
		{
			mGameMenuControlsDisplay.addControl("Change", false, "@input_icon_button_left", "@input_icon_button_right");
			if (selectedEntry.mData == option::SOUND_TEST)
				mGameMenuControlsDisplay.addControl("Play", false, "@input_icon_button_A");
		}
		else
		{
			mGameMenuControlsDisplay.addControl("Select", false, "@input_icon_button_A");
		}

		if (mActiveTab == Tab::Id::MODS)
		{
			// Show only here, as that's where it's most relevent - however, it works in the other tabs as well
			mGameMenuControlsDisplay.addControl("Hold: Quick Nav", true, "@input_icon_button_X");
		}

		mGameMenuControlsDisplay.addControl("Back",	true, "@input_icon_button_B");
	}
}

void OptionsMenu::updateModExpandState(ModTitleMenuEntry& modTitleMenuEntry)
{
	const bool visible = (modTitleMenuEntry.selected().mValue > 0);
	const Mod& mod = modTitleMenuEntry.getMod();
	if (visible)
		mExpandedMods.insert(mod.mLocalDirectoryHash);
	else
		mExpandedMods.erase(mod.mLocalDirectoryHash);

	modTitleMenuEntry.mMarginBelow = visible ? 0 : -10;

	const std::vector<GameMenuEntry*>& entries = mActiveMenu->getEntries();
	for (int k = mActiveMenu->mSelectedEntryIndex + 1; k < (int)entries.size(); ++k)
	{
		if (entries[k]->getMenuEntryType() == ModTitleMenuEntry::MENU_ENTRY_TYPE || entries[k]->mData == option::_BACK)
			break;
		entries[k]->setVisible(visible);
	}
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

	// Apply script optimization level if it got changed
	if (mOriginalScriptOptimizationLevel != Configuration::instance().mScriptOptimizationLevel)
	{
		Application::instance().getSimulation().triggerFullScriptsReload();
	}

	GameApp::instance().onExitOptions();
	mState = mEnteredFromIngame ? State::FADE_TO_GAME : State::FADE_TO_MENU;
}
