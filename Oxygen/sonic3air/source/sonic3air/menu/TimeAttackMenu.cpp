/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/TimeAttackMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/Game.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/data/TimeAttackData.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/simulation/Simulation.h"


namespace
{
	enum class CharacterOption
	{
		// Do not make changes here
		SONIC_CLASSIC	 = 0x10,
		SONIC_MAXCONTROL = 0x11,
		TAILS			 = 0x20,
		KNUCKLES		 = 0x30
	};
}


TimeAttackMenu::TimeAttackMenu(MenuBackground& menuBackground) :
	mMenuBackground(&menuBackground)
{
	// Build up menu structure
	{
		mMenuEntries.reserve(4);

		mZoneEntry = &mMenuEntries.addEntry();
		mActEntry = &mMenuEntries.addEntry();
		const auto& zones = SharedDatabase::getAllZones();
		for (size_t zoneIndex = 0; zoneIndex < zones.size(); ++zoneIndex)
		{
			const SharedDatabase::Zone& zone = zones[zoneIndex];
			const int acts = zone.mActsTimeAttack;
			if (acts > 0)
			{
				uint16 zoneId = zone.mInternalIndex << 8;
				mZoneEntry->addOption(zone.mDisplayName, zoneId);
				if (acts == 2)
				{
					mActEntry->addOption("Act 1", zoneId);
					mActEntry->addOption("Act 2", zoneId + 1);
				}
				else
				{
					mActEntry->addOption("Single Act", zoneId);
				}
			}
		}

		mCharacterEntry = &mMenuEntries.addEntry();
		mCharacterEntry->addOption("Sonic",    (uint32)CharacterOption::SONIC_CLASSIC);
		mCharacterEntry->addOption("Sonic - Max Control", (uint32)CharacterOption::SONIC_MAXCONTROL);
		mCharacterEntry->addOption("Tails",    (uint32)CharacterOption::TAILS);
		mCharacterEntry->addOption("Knuckles", (uint32)CharacterOption::KNUCKLES);

		mMenuEntries.addEntry("Back", 0x10);
	}

	// Set defaults
	mMenuEntries.mSelectedEntryIndex = 0;
	mZoneEntry->mSelectedIndex = 0;
	mCharacterEntry->mSelectedIndex = 0;
}

TimeAttackMenu::~TimeAttackMenu()
{
}

GameMenuBase::BaseState TimeAttackMenu::getBaseState() const
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

void TimeAttackMenu::setBaseState(BaseState baseState)
{
	switch (baseState)
	{
		case BaseState::INACTIVE: mState = State::INACTIVE;  break;
		case BaseState::FADE_IN:  mState = State::APPEAR;  break;
		case BaseState::SHOW:	  mState = State::SHOW;  break;
		case BaseState::FADE_OUT: mState = State::FADE_TO_MENU;  break;
	}
}

void TimeAttackMenu::onFadeIn()
{
	mState = State::APPEAR;

	mMenuBackground->showPreview(true);
	mMenuBackground->startTransition(MenuBackground::Target::LIGHT);

	// Reset this
	mBestTimesForCharacters = 0xff;
	mBestTimesForZoneAct = 0xffff;

	AudioOut::instance().stopSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_MUSIC);

	// Play "Data Select" music inside this menu
	AudioOut::instance().setMenuMusic(0x2f);
}

bool TimeAttackMenu::canBeRemoved()
{
	return (mState == State::INACTIVE && mVisibility <= 0.0f);
}

void TimeAttackMenu::initialize()
{
	// Update Max Control unlocking
	GameMenuEntry::Option* option = mCharacterEntry->getOptionByValue((uint32)CharacterOption::SONIC_MAXCONTROL);
	RMX_CHECK(nullptr != option, "Option for Max Control not found", );
	if (nullptr != option)
	{
		option->mVisible = PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_SUPER_PEELOUT);
	}
}

void TimeAttackMenu::deinitialize()
{
}

void TimeAttackMenu::keyboard(const rmx::KeyboardEvent& ev)
{
}

void TimeAttackMenu::update(float timeElapsed)
{
	GameMenuBase::update(timeElapsed);

	// Don't react to input during transitions
	if (mState == State::SHOW)
	{
		// Update menu entries
		const GameMenuEntries::UpdateResult result = mMenuEntries.update();
		if (result != GameMenuEntries::UpdateResult::NONE)
		{
			playMenuSound(0x5b);

			if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED)
			{
				if (mMenuEntries.mSelectedEntryIndex == 1)	// Act entry was changed
				{
					mPreferredAct = mActEntry->selected().mValue % 2;
					mZoneEntry->setSelectedIndexByValue(mActEntry->selected().mValue & 0xff00);
				}
				else	// Zone entry was changed
				{
					mActEntry->setSelectedIndexByValue(mZoneEntry->selected().mValue + mPreferredAct);
				}
			}
		}

		if (!FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT))
		{
			const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);
			const uint32 selectedData = mMenuEntries.selected().mData;

			if (keys.Start.justPressed() || keys.A.justPressed() || keys.X.justPressed())
			{
				if (selectedData == 0x10)
				{
					backToMainMenu();
				}
				else
				{
					triggerStartGame();
				}
			}
			else if (keys.B.justPressed())
			{
				backToMainMenu();
			}
		}
	}

	const uint16 zoneAndAct = (uint16)mActEntry->selected().mValue;
	mMenuBackground->setPreviewZoneAndAct(zoneAndAct >> 8, zoneAndAct % 2);

	if (mBestTimesForZoneAct != mActEntry->selected().mValue || mBestTimesForCharacters != mCharacterEntry->selected().mValue)
	{
		mBestTimesForZoneAct = mActEntry->selected().mValue;
		mBestTimesForCharacters = mCharacterEntry->selected().mValue;

		// Get entries from the time attack table
		TimeAttackData::Table* timeAttackTable = TimeAttackData::getTable(mBestTimesForZoneAct, mBestTimesForCharacters);
		if (nullptr == timeAttackTable)
		{
			std::wstring recBaseFilename;
			const std::wstring recordingsDir = TimeAttackData::getSavePath(mBestTimesForZoneAct, mBestTimesForCharacters, &recBaseFilename);

			if (!recordingsDir.empty())
			{
				timeAttackTable = &TimeAttackData::loadTable(mBestTimesForZoneAct, mBestTimesForCharacters, recordingsDir + L"/records.json");
			}
		}

		mBestTimes.clear();
		if (nullptr != timeAttackTable)
		{
			mBestTimes.reserve(timeAttackTable->mEntries.size());
			for (auto& entry : timeAttackTable->mEntries)
			{
				mBestTimes.emplace_back(TimeAttackData::getTimeString(entry.mTime, 1));
			}
		}
	}

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
			if (mState == State::FADE_TO_GAME)
			{
				startGame();
			}
			mState = State::INACTIVE;
		}
	}
}

void TimeAttackMenu::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();

	int anchorX = 200;
	float alpha = 1.0f;
	if (mState != State::SHOW && mState != State::FADE_TO_GAME)
	{
		anchorX += roundToInt((1.0f - mVisibility) * 200.0f);
		alpha = mVisibility;
	}

	// Title text
	drawer.printText(global::mSonicFontC, Recti(anchorX, 4, 0, 18), "TIME ATTACK", 5, Color(1.0f, 1.0f, 1.0f, alpha));

	// Menu entries
	const int positionY[] = { 116, 138, 160, 198 };
	for (size_t line = 0; line < mMenuEntries.size(); ++line)
	{
		const auto& entry = mMenuEntries[line];
		const std::string& text = entry.mOptions.empty() ? entry.mText : entry.mOptions[entry.mSelectedIndex].mText;
		const bool canGoLeft  = entry.mOptions.empty() ? false : (entry.mSelectedIndex > 0);
		const bool canGoRight = entry.mOptions.empty() ? false : (entry.mSelectedIndex < entry.mOptions.size() - 1);

		const int py = positionY[line];
		const bool isSelected = ((int)line == mMenuEntries.mSelectedEntryIndex);
		const Color color = (isSelected) ? Color(1.0f, 1.0f, 0.0f, alpha) : Color(1.0f, 1.0f, 1.0f, alpha * 0.9f);

		int arrowAnimOffset = 0;
		if (isSelected)
		{
			arrowAnimOffset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
			arrowAnimOffset = (arrowAnimOffset > 3) ? (6 - arrowAnimOffset) : arrowAnimOffset;
		}

		int px = 200;
		if (mState != State::SHOW && mState != State::FADE_TO_GAME)
		{
			const int lineOffset = (mState < State::SHOW) ? (int)(mMenuEntries.size() - 1 - line) : (int)line;
			px = 200 + roundToInt(saturate(1.0f - alpha - lineOffset * 0.15f) * 200.0f);
		}

		if (canGoLeft)
			drawer.printText(global::mOxyfontRegular, Recti(px - 145 - arrowAnimOffset, py, 10, 10), "<", 5, color);
		if (canGoRight)
			drawer.printText(global::mOxyfontRegular, Recti(px + 15 + arrowAnimOffset, py, 10, 10), ">", 5, color);

		drawer.printText(global::mOxyfontRegular, Recti(px - 160, py, 200, 10), text, 5, color);
	}

	for (int i = 0; i < 5; ++i)
	{
		Recti rect(anchorX + 90, 118 + i * 18, 80, 18);
		if (i < (int)mBestTimes.size())
			drawer.printText(global::mOxyfontRegular, rect, mBestTimes[i], 5, Color(1.0f, 1.0f, 1.0f, alpha));
		else
			drawer.printText(global::mOxyfontRegular, rect, "-' --\" --", 5, Color(0.9f, 0.9f, 0.9f, alpha));
	}

	drawer.performRendering();
}


void TimeAttackMenu::triggerStartGame()
{
	playMenuSound(0xaf);
	GameApp::instance().getGameView().startFadingOut();
	mState = State::FADE_TO_GAME;
}

void TimeAttackMenu::startGame()
{
	// Init simulation
	const uint8 characters = clamp(mCharacterEntry->selected().mValue >> 4, 1, 3);
	Game::instance().startIntoLevel(Game::Mode::TIME_ATTACK, mCharacterEntry->selected().mValue, mActEntry->selected().mValue, characters);
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();

	mMenuEntries.mSelectedEntryIndex = 0;
}

void TimeAttackMenu::backToMainMenu()
{
	playMenuSound(0xad);
	mMenuBackground->openMainMenu();
	mState = State::FADE_TO_MENU;
}
