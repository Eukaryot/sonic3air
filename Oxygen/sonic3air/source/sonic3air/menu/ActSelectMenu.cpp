/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/ActSelectMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/Game.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/simulation/Simulation.h"


ActSelectMenu::ActSelectMenu(MenuBackground& menuBackground) :
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
			const int acts = zone.mActsNormal;
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
					if (zoneId == 0x1600)
						mActEntry->addOption("Single Act", 0x1601);		// Special handling for HPZ
					else
						mActEntry->addOption("Single Act", zoneId);
				}
			}
		}

		mCharacterEntry = &mMenuEntries.addEntry();
		mCharacterEntry->addOption("Sonic & Tails", 0x00);
		mCharacterEntry->addOption("Sonic", 0x01);
		mCharacterEntry->addOption("Tails", 0x02);
		mCharacterEntry->addOption("Knuckles", 0x03);

		mMenuEntries.addEntry("Back", 0x10);
	}

	// Set defaults
	mMenuEntries.mSelectedEntryIndex = 0;
	mZoneEntry->mSelectedIndex = 0;
	mCharacterEntry->mSelectedIndex = 0;
}

ActSelectMenu::~ActSelectMenu()
{
}

GameMenuBase::BaseState ActSelectMenu::getBaseState() const
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

void ActSelectMenu::setBaseState(BaseState baseState)
{
	switch (baseState)
	{
		case BaseState::INACTIVE: mState = State::INACTIVE;  break;
		case BaseState::FADE_IN:  mState = State::APPEAR;  break;
		case BaseState::SHOW:	  mState = State::SHOW;  break;
		case BaseState::FADE_OUT: mState = State::FADE_TO_MENU;  break;
	}
}

void ActSelectMenu::onFadeIn()
{
	// Update DDZ
	mZoneEntry->getOptionByValue(0x0c00)->mVisible = PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_DOOMSDAY_ZONE);
	mActEntry->getOptionByValue(0x0c00)->mVisible = PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_DOOMSDAY_ZONE);

	mState = State::APPEAR;

	mMenuBackground->showPreview(true);
	mMenuBackground->startTransition(MenuBackground::Target::LIGHT);

	if (PlayerProgress::instance().isSecretUnlocked(SharedDatabase::Secret::SECRET_KNUX_AND_TAILS))
	{
		if (mCharacterEntry->mOptions.size() < 5)
		{
			mCharacterEntry->addOption("Knuckles & Tails", 0x04);
		}
	}

	AudioOut::instance().stopSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_MUSIC);

	// Play "Data Select" music inside this menu
	AudioOut::instance().setMenuMusic(0x2f);
}

bool ActSelectMenu::canBeRemoved()
{
	return (mState == State::INACTIVE && mVisibility <= 0.0f);
}

void ActSelectMenu::initialize()
{
	mState = State::APPEAR;
	mVisibility = 0.0f;
}

void ActSelectMenu::deinitialize()
{
}

void ActSelectMenu::keyboard(const rmx::KeyboardEvent& ev)
{
}

void ActSelectMenu::update(float timeElapsed)
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
	mMenuBackground->setPreviewZoneAndAct(zoneAndAct >> 8, (zoneAndAct == 0x1601) ? 0 : (zoneAndAct % 2));	// Special handling for HPZ

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

void ActSelectMenu::render()
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
	drawer.printText(global::mSonicFontC, Recti(anchorX, 4, 0, 18), "ACT SELECT", 5, Color(1.0f, 1.0f, 1.0f, alpha));

	// Menu entries
	const int positionY[] = { 116, 138, 160, 198 };
	for (size_t line = 0; line < mMenuEntries.size(); ++line)
	{
		const auto& entry = mMenuEntries[line];
		const std::string& text = entry.mOptions.empty() ? entry.mText : entry.mOptions[entry.mSelectedIndex].mText;
		const bool canGoLeft  = entry.mOptions.empty() ? false : (entry.getPreviousVisibleIndex() != entry.mSelectedIndex);
		const bool canGoRight = entry.mOptions.empty() ? false : (entry.getNextVisibleIndex() != entry.mSelectedIndex);

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
			drawer.printText(global::mOxyfontRegular, Recti(px - 55 - arrowAnimOffset, py, 10, 10), "<", 5, color);
		if (canGoRight)
			drawer.printText(global::mOxyfontRegular, Recti(px + 105 + arrowAnimOffset, py, 10, 10), ">", 5, color);

		drawer.printText(global::mOxyfontRegular, Recti(px - 70, py, 200, 10), text, 5, color);
	}

	// Show character selection
	{
		drawer.drawSprite(Vec2i(anchorX - 122, 159), rmx::getMurmur2_64("charselectionbox"), Color(1.0f, 1.0f, 1.0f, alpha));

		static const uint64 charSpriteKey[5] =
		{
			rmx::getMurmur2_64("menu_characters_sonic_tails"),		// Sonic & Tails
			rmx::getMurmur2_64("menu_characters_sonic"),			// Sonic
			rmx::getMurmur2_64("menu_characters_tails"),			// Tails
			rmx::getMurmur2_64("menu_characters_knuckles"),			// Knuckles
			rmx::getMurmur2_64("menu_characters_knuckles_tails")	// Knuckles & Tails
		};
		const uint32 charSelection = clamp(mCharacterEntry->selected().mValue, 0, 4);
		drawer.drawSprite(Vec2i(anchorX - 125, 156), charSpriteKey[charSelection], Color(1.0f, 1.0f, 1.0f, alpha));
	}

	drawer.performRendering();
}

void ActSelectMenu::triggerStartGame()
{
	playMenuSound(0xaf);
	GameApp::instance().getGameView().startFadingOut();
	mState = State::FADE_TO_GAME;
}

void ActSelectMenu::startGame()
{
	// Init simulation
	Game::instance().startIntoLevel(Game::Mode::ACT_SELECT, 0, mActEntry->selected().mValue, (uint8)mCharacterEntry->selected().mValue);
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();

	mMenuEntries.mSelectedEntryIndex = 0;
}

void ActSelectMenu::backToMainMenu()
{
	playMenuSound(0xad);
	mMenuBackground->openMainMenu();
	mState = State::FADE_TO_MENU;
}
