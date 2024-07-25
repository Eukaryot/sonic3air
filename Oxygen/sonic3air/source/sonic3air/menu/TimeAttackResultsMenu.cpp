/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/TimeAttackResultsMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/SharedResources.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/simulation/Simulation.h"


namespace
{
	String formatTime(int hundreds, bool allowShortSpace = false)
	{
		// Misusing the degree character '°' = 0xb0 for a short space; the font has to support this
		// TODO: Maybe use a different character for this...?
		return String(0, "%d'%c%02d\"%02d", hundreds / 6000, allowShortSpace ? (char)0xb0 : ' ', (hundreds / 100) % 60, hundreds % 100);
	}
}


TimeAttackResultsMenu::TimeAttackResultsMenu()
{
}

TimeAttackResultsMenu::~TimeAttackResultsMenu()
{
}

void TimeAttackResultsMenu::onFadeIn()
{
	// Build up menu structure
	{
		mMenuEntries.clear();
		mMenuEntries.reserve(2);
		mMenuEntries.addEntry("Retry", 0);
		mMenuEntries.addEntry("Exit", 1);
	}
	mMenuEntries.mSelectedEntryIndex = 0;
}

void TimeAttackResultsMenu::initialize()
{
	mTime = 0.0f;
	mGameStopped = false;
}

void TimeAttackResultsMenu::deinitialize()
{
	GameApp::instance().enableStillImageBlur(false);
}

void TimeAttackResultsMenu::keyboard(const rmx::KeyboardEvent& ev)
{
}

void TimeAttackResultsMenu::update(float timeElapsed)
{
	mTime += timeElapsed;

	// Update menu entries
	{
		const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);
		const GameMenuEntries::UpdateResult result = mMenuEntries.update();
		if (result != GameMenuEntries::UpdateResult::NONE)
		{
			playMenuSound(0x5b);
		}

		if (keys.Start.justPressed() || keys.A.justPressed() || keys.X.justPressed())
		{
			switch (mMenuEntries.selected().mData)
			{
				case 0:
				{
					GameApp::instance().restartTimeAttack();
					break;
				}

				case 1:
				{
					GameApp::instance().returnToMenu();
					break;
				}
			}
		}
		else if (keys.Y.justPressed())
		{
			GameApp::instance().restartTimeAttack();
		}
	}
}

void TimeAttackResultsMenu::render()
{
	Drawer& drawer = EngineMain::instance().getDrawer();

	constexpr float FADE1_TIME = 0.15f;
	constexpr float FADE2_TIME = 0.45f;

	if (mTime > FADE1_TIME)
	{
		if (!mGameStopped)
		{
			Application::instance().getSimulation().setSpeed(0.0f);
			mGameStopped = true;
			GameApp::instance().enableStillImageBlur(true, 0.5f);
		}

		Recti rect(mRect.width - global::mTimeAttackResultsBG.getWidth(), 0, global::mTimeAttackResultsBG.getWidth(), global::mTimeAttackResultsBG.getHeight());
		drawer.drawRect(rect, global::mTimeAttackResultsBG);

		const String text = String("Your time:   ") + formatTime(mYourTime, true);
		drawer.printText(global::mSonicFontC, Vec2i(76 + std::min(roundToInt(mTime * 500) - 160, 0), 97), text, 1);

		if (!mBetterTimes.empty())
		{
			const String text = (mBetterTimes.size() == 1) ? "Time to beat:" : "Times to beat:";
			const int px = 232 + ((int)mBetterTimes.size() + 1) * 2 + roundToInt(std::max(0.45f - mTime, 0.0f) * 750);
			const int py = 79 - ((int)mBetterTimes.size() + 1) * 16;
			drawer.printText(global::mOxyfontRegular, Vec2i(px, py), text, 1);
		}
		else if (!mWorseTimes.empty())
		{
			const String text = "NEW RECORD!";
			const int px = 238 + roundToInt(std::max(0.45f - mTime, 0.0f) * 750);
			const int py = 64;
			drawer.printText(global::mOxyfontRegular, Vec2i(px, py), text, 1, roundToInt(mTime * 8.0f) % 2 ? Color::YELLOW : Color::WHITE);
		}

		for (int index = 0; index < (int)mBetterTimes.size(); ++index)
		{
			const String text = formatTime(mBetterTimes[index]);
			const int px = 260 + ((int)mBetterTimes.size() - index) * 2 + roundToInt(std::max(0.6f + (float)index * 0.15f - mTime, 0.0f) * 750);
			const int py = 81 - ((int)mBetterTimes.size() - index) * 16;
			drawer.printText(global::mOxyfontRegular, Vec2i(px, py), text, 1);
		}

		for (int index = 0; index < (int)mWorseTimes.size(); ++index)
		{
			const String text = formatTime(mWorseTimes[index]);
			const int px = 252 - index * 2 + roundToInt(std::max(0.6f + (float)(mBetterTimes.size() + index) * 0.15f - mTime, 0.0f) * 750);
			const int py = 129 + index * 16;
			drawer.printText(global::mOxyfontRegular, Vec2i(px, py), text, 1);
		}

		// Menu entries
		for (size_t line = 0; line < mMenuEntries.size(); ++line)
		{
			const auto& entry = mMenuEntries[line];
			Color color = Color(0.7f, 0.8f, 1.0f, 0.8f);
			if ((int)line == mMenuEntries.mSelectedEntryIndex)
				color = (std::fmod(FTX::getTime() * 2.0f, 1.0f) < 0.5f) ? Color::YELLOW : Color::WHITE;

			drawer.printText(global::mSonicFontC, Recti(46 + (int)line * 2, 150 + (int)line * 28, 0, 20), entry.mText, 4, color);
		}
	}

	if (mTime < FADE1_TIME + FADE2_TIME)
	{
		float layerAlpha = (mTime < FADE1_TIME) ? (mTime / FADE1_TIME) : (1.0f - (mTime - FADE1_TIME) / FADE2_TIME);
		drawer.drawRect(mRect, Color(1.0f, 1.0f, 1.0f, layerAlpha));
	}

	drawer.performRendering();
}

void TimeAttackResultsMenu::setYourTime(int hundreds)
{
	mYourTime = hundreds;
	mBetterTimes.clear();
	mWorseTimes.clear();
}

void TimeAttackResultsMenu::addOtherTime(int hundreds)
{
	if (hundreds <= mYourTime)
	{
		if (mBetterTimes.size() < 3)
		{
			mBetterTimes.push_back(hundreds);
		}
	}
	else
	{
		if (mWorseTimes.size() < 3)
		{
			mWorseTimes.push_back(hundreds);
		}
	}
}
