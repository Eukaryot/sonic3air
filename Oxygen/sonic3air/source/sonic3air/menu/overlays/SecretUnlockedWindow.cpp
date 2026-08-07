/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/menu/overlays/SecretUnlockedWindow.h"
#include "sonic3air/audio/AudioOut.h"

#include "oxygen/application/EngineMain.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/Utils.h"


void SecretUnlockedWindow::initialize()
{
}

void SecretUnlockedWindow::deinitialize()
{
}

void SecretUnlockedWindow::update(float timeElapsed)
{
	switch (mPhase)
	{
		case Phase::NONE:
			break;
		case Phase::TITLE_APPEAR:
			mPhaseTimer += timeElapsed / 0.3f;
			break;
		case Phase::TITLE_SHOW:
			mPhaseTimer += timeElapsed / 0.75f;
			break;
		case Phase::CONTENT_APPEAR:
			mPhaseTimer += timeElapsed / 0.3f;
			break;
		case Phase::CONTENT_SHOW:
			mPhaseTimer += timeElapsed / 5.0f;
			break;
		case Phase::DISAPPEAR:
			mPhaseTimer += timeElapsed / 0.2f;
			break;
	}

	if (mPhaseTimer >= 1.0f)
	{
		mPhaseTimer = 0.0f;
		if (mPhase == Phase::DISAPPEAR)
		{
			mPhase = Phase::NONE;
			if (mEnqueuedEntries.empty())
			{
				removeFromParent();
			}
			else
			{
				showInternal(mEnqueuedEntries.front());
				mEnqueuedEntries.pop_front();
			}
		}
		else
		{
			mPhase = (Phase)((int)mPhase + 1);
			if (mPhase == Phase::TITLE_SHOW && mShownEntry.mSoundId != 0)
			{
				AudioOut::instance().playAudioDirect(mShownEntry.mSoundId, AudioOut::SoundRegType::SOUND, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_SOUND);
			}
		}
	}
}

void SecretUnlockedWindow::render()
{
	if (mPhase == Phase::NONE)
		return;

	float titleVisibility = 0.0f;
	float contentVisibility = 0.0f;
	switch (mPhase)
	{
		case Phase::NONE:
			break;
		case Phase::TITLE_APPEAR:
			titleVisibility = mPhaseTimer;
			contentVisibility = 0.0f;
			break;
		case Phase::TITLE_SHOW:
			titleVisibility = 1.0f;
			contentVisibility = 0.0f;
			break;
		case Phase::CONTENT_APPEAR:
			titleVisibility = 1.0f;
			contentVisibility = mPhaseTimer;
			break;
		case Phase::CONTENT_SHOW:
			titleVisibility = 1.0f;
			contentVisibility = 1.0f;
			break;
		case Phase::DISAPPEAR:
			titleVisibility = 1.0f - mPhaseTimer;
			contentVisibility = 1.0f;
			break;
	}

	Drawer& drawer = EngineMain::instance().getDrawer();
	Font& font = global::mOxyfontRegular;

	const int numTextLines = (int)mShownTextLines.size();
	const int lineHeight = (font.getLineHeight() + 1);
	const int px = roundToInt(mRect.width - 240.0f * titleVisibility);
	const int py = roundToInt(mRect.height - interpolate(19.0f, 22.0f + numTextLines * lineHeight, contentVisibility));

	constexpr uint64 spriteKey = rmx::constMurmur2_64("unlock_window_bg");
	drawer.drawSprite(Vec2i(px, py), spriteKey);

	const Color titleColor = (mShownEntry.mEntryType == EntryType::ACHIEVEMENT) ? Color(0.6f, 0.8f, 1.0f) : Color(1.0f, 0.7f, 0.6f);
	drawer.printText(global::mSonicFontB, Recti(px + 55, py, 185, 20), mShownEntry.mTitle, 4, titleColor);

	for (int lineIndex = 0; lineIndex < numTextLines; ++lineIndex)
	{
		drawer.printText(font, Recti(px + 56 - lineIndex * 2, py + 19 + lineIndex * lineHeight, 190, lineHeight), mShownTextLines[lineIndex], 4, Color::WHITE);
	}
}

void SecretUnlockedWindow::show(EntryType entryType, const std::string& title, const std::string& content, uint8 soundId)
{
	Entry newEntry;
	newEntry.mEntryType = entryType;
	newEntry.mTitle = title;
	newEntry.mContent = content;
	newEntry.mSoundId = soundId;

	if (mPhase == Phase::NONE)
	{
		showInternal(newEntry);
	}
	else
	{
		// Enqueue entry
		mEnqueuedEntries.emplace_back(newEntry);
	}
}

void SecretUnlockedWindow::showInternal(Entry& entry)
{
	mPhase = Phase::TITLE_APPEAR;
	mPhaseTimer = 0.0f;

	std::swap(mShownEntry, entry);
	utils::splitTextIntoLines(mShownTextLines, mShownEntry.mContent, global::mOxyfontRegular, 170);
}
