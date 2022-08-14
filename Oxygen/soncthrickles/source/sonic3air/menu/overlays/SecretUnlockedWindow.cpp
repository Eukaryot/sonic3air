/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
		case PHASE_NONE:
			break;
		case PHASE_TITLE_APPEAR:
			mPhaseTimer += timeElapsed / 0.3f;
			break;
		case PHASE_TITLE_SHOW:
			mPhaseTimer += timeElapsed / 0.75f;
			break;
		case PHASE_CONTENT_APPEAR:
			mPhaseTimer += timeElapsed / 0.3f;
			break;
		case PHASE_CONTENT_SHOW:
			mPhaseTimer += timeElapsed / 5.0f;
			break;
		case PHASE_DISAPPEAR:
			mPhaseTimer += timeElapsed / 0.2f;
			break;
	}

	if (mPhaseTimer >= 1.0f)
	{
		mPhaseTimer = 0.0f;
		if (mPhase == PHASE_DISAPPEAR)
		{
			mPhase = PHASE_NONE;
			if (mEnqueuedEntries.empty())
			{
				getParent()->removeChild(this);
			}
			else
			{
				showInternal(mEnqueuedEntries.front());
				mEnqueuedEntries.pop_front();
			}
		}
		else
		{
			mPhase = (Phase)(mPhase + 1);
			if (mPhase == PHASE_TITLE_SHOW && mShownEntry.mSoundId != 0)
			{
				AudioOut::instance().playAudioDirect(mShownEntry.mSoundId, AudioOut::SoundRegType::SOUND, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_SOUND);
			}
		}
	}
}

void SecretUnlockedWindow::render()
{
	if (mPhase == PHASE_NONE)
		return;

	float titleVisibility = 0.0f;
	float contentVisibility = 0.0f;
	switch (mPhase)
	{
		case PHASE_NONE:
			break;
		case PHASE_TITLE_APPEAR:
			titleVisibility = mPhaseTimer;
			contentVisibility = 0.0f;
			break;
		case PHASE_TITLE_SHOW:
			titleVisibility = 1.0f;
			contentVisibility = 0.0f;
			break;
		case PHASE_CONTENT_APPEAR:
			titleVisibility = 1.0f;
			contentVisibility = mPhaseTimer;
			break;
		case PHASE_CONTENT_SHOW:
			titleVisibility = 1.0f;
			contentVisibility = 1.0f;
			break;
		case PHASE_DISAPPEAR:
			titleVisibility = 1.0f - mPhaseTimer;
			contentVisibility = 1.0f;
			break;
	}

	Drawer& drawer = EngineMain::instance().getDrawer();
	Font& font = global::mFont10;

	const int numTextLines = (int)mShownTextLines.size();
	const int lineHeight = (font.getLineHeight() + 1);

	Recti rect;
	rect.width = global::mPauseScreenLowerBG.getWidth();
	rect.height = global::mPauseScreenLowerBG.getHeight();
	rect.x = roundToInt(mRect.width - (float)rect.width * titleVisibility);
	rect.y = roundToInt(mRect.height - interpolate(19.0f, 22.0f + numTextLines * lineHeight, contentVisibility));

	drawer.drawRect(rect, global::mPauseScreenLowerBG);

	const Color titleColor = (mShownEntry.mEntryType == EntryType::ACHIEVEMENT) ? Color(0.6f, 0.8f, 1.0f) : Color(1.0f, 0.7f, 0.6f);
	drawer.printText(global::mFont7, Recti(rect.x + 55, rect.y, rect.width - 55, 20), mShownEntry.mTitle, 4, titleColor);

	for (int lineIndex = 0; lineIndex < numTextLines; ++lineIndex)
	{
		drawer.printText(font, Recti(rect.x + 56 - lineIndex * 2, rect.y + 19 + lineIndex * lineHeight, rect.width - 50, lineHeight), mShownTextLines[lineIndex], 4, Color::WHITE);
	}
}

void SecretUnlockedWindow::show(EntryType entryType, const std::string& title, const std::string& content, uint8 soundId)
{
	Entry newEntry;
	newEntry.mEntryType = entryType;
	newEntry.mTitle = title;
	newEntry.mContent = content;
	newEntry.mSoundId = soundId;

	if (mPhase == PHASE_NONE)
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
	mPhase = PHASE_TITLE_APPEAR;
	mPhaseTimer = 0.0f;

	std::swap(mShownEntry, entry);
	utils::splitTextIntoLines(mShownTextLines, mShownEntry.mContent, global::mFont10, 170);
}
