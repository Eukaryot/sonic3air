/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/GameMenuBase.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/audio/AudioOut.h"

#include "oxygen/application/input/InputManager.h"


GameMenuEntries::Option& GameMenuEntries::Entry::addOptionRef(const std::string& text, uint32 value)
{
	Option& option = vectorAdd(mOptions);
	option.mText = text;
	option.mIndex = mOptions.size() - 1;
	option.mValue = value;
	return option;
}

GameMenuEntries::Entry& GameMenuEntries::Entry::addOption(const std::string& text, uint32 value)
{
	addOptionRef(text, value);
	return *this;
}

GameMenuEntries::Entry& GameMenuEntries::Entry::addNumberOptions(int minValue, int maxValue, int step)
{
	for (int value = minValue; value <= maxValue; value += step)
		addOptionRef(std::to_string(value), value);
	return *this;
}

GameMenuEntries::Entry& GameMenuEntries::Entry::addPercentageOptions(int minValue, int maxValue, int step)
{
	for (int value = minValue; value <= maxValue; value += step)
		addOptionRef(std::to_string(value) + '%', value);
	return *this;
}

GameMenuEntries::Option* GameMenuEntries::Entry::getOptionByValue(uint32 value)
{
	for (size_t i = 0; i < mOptions.size(); ++i)
	{
		if (mOptions[i].mValue == value)
			return &mOptions[i];
	}
	return nullptr;
}

void GameMenuEntries::Entry::setVisible(bool visible)
{
	// Note: This should not be mixed with "setEnabled"
	mVisibility = visible ? Visibility::VISIBLE : Visibility::INVISIBLE;
}

void GameMenuEntries::Entry::setEnabled(bool enabled)
{
	// Note: This should not be mixed with "setVisible"
	mVisibility = enabled ? Visibility::VISIBLE : Visibility::DISABLED;
}

bool GameMenuEntries::Entry::setSelectedIndexByValue(uint32 value)
{
	if (mOptions.empty())
		return false;

	// Check for closest match
	int difference = 0x7fffffff;
	for (size_t i = 0; i < mOptions.size(); ++i)
	{
		const int diff = std::abs((int)(mOptions[i].mValue - value));
		if (diff < difference)
		{
			mSelectedIndex = i;
			if (diff == 0)
				return true;	// Return true instantly, as this is an exact match
			difference = diff;
		}
	}

	// Selected an entry, but it's not an exact match
	return false;
}

void GameMenuEntries::Entry::sanitizeSelectedIndex()
{
	if (mSelectedIndex >= mOptions.size())
	{
		mSelectedIndex = mOptions.empty() ? 0 : (mOptions.size() - 1);
	}
}



void GameMenuEntries::reserve(size_t count)
{
	mEntries.reserve(count);
}

void GameMenuEntries::resize(size_t count)
{
	mEntries.resize(count);
}

GameMenuEntries::Entry& GameMenuEntries::addEntry(const std::string& text, uint32 data)
{
	Entry& entry = vectorAdd(mEntries);
	entry.mText = text;
	entry.mData = data;
	return entry;
}

GameMenuEntries::Entry* GameMenuEntries::getEntryByData(uint32 data)
{
	for (Entry& entry : mEntries)
	{
		if (entry.mData == data)
			return &entry;
	}
	return nullptr;
}

void GameMenuEntries::insert(const Entry& toCopy, size_t index)
{
	mEntries.insert(mEntries.begin() + index, toCopy);
}

void GameMenuEntries::erase(size_t index)
{
	mEntries.erase(mEntries.begin() + index);
}

GameMenuEntries::UpdateResult GameMenuEntries::update()
{
	// Sanity checks
	if (mEntries.empty())
		return UpdateResult::NONE;
	sanitizeSelectedIndex();

	// Evaluate input
	if (!FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT))
	{
		const InputManager::ControllerScheme& controller = InputManager::instance().getController(0);

		int entryChange = 0;
		if (mNavigation == Navigation::VERTICAL)
		{
			if (controller.Up.justPressedOrRepeat())
				--entryChange;
			if (controller.Down.justPressedOrRepeat())
				++entryChange;
		}
		else
		{
			if (controller.Left.justPressedOrRepeat())
				--entryChange;
			if (controller.Right.justPressedOrRepeat())
				++entryChange;
		}

		int optionChange = 0;
		if (mNavigation == Navigation::VERTICAL)
		{
			if (controller.Left.justPressedOrRepeat())
				--optionChange;
			if (controller.Right.justPressedOrRepeat())
				++optionChange;
		}
		else
		{
			if (controller.Up.justPressedOrRepeat())
				--optionChange;
			if (controller.Down.justPressedOrRepeat())
				++optionChange;
		}

		if (entryChange != 0)
		{
			if (entryChange < 0)
			{
				for (int tries = 0; tries < 100; ++tries)
				{
					if (mSelectedEntryIndex > 0)
					{
						--mSelectedEntryIndex;
					}
					else
					{
						mSelectedEntryIndex = (int)mEntries.size() - 1;
					}

					// Continue for invisible and unavailable entries, so they get skipped
					if (mEntries[mSelectedEntryIndex].mVisibility == Visibility::VISIBLE)
						break;
				}
			}
			else
			{
				for (int tries = 0; tries < 100; ++tries)
				{
					if (mSelectedEntryIndex < (int)mEntries.size() - 1)
					{
						++mSelectedEntryIndex;
					}
					else
					{
						mSelectedEntryIndex = 0;
					}

					// Continue for invisible and unavailable entries, so they get skipped
					if (mEntries[mSelectedEntryIndex].mVisibility == Visibility::VISIBLE)
						break;
				}
			}
			return UpdateResult::ENTRY_CHANGED;
		}

		if (optionChange != 0)
		{
			if (optionChange < 0)
			{
				auto& entry = mEntries[mSelectedEntryIndex];
				if (!entry.mOptions.empty())
				{
					int index = (int)entry.mSelectedIndex;
					for (int tries = 0; tries < 100; ++tries)
					{
						if (index <= 0)
							break;

						--index;
						if (entry.mOptions[index].mVisible)		// Continue for invisible options, so they get skipped
						{
							entry.mSelectedIndex = index;
							return UpdateResult::OPTION_CHANGED;
						}
					}
				}
			}
			else
			{
				auto& entry = mEntries[mSelectedEntryIndex];
				if (!entry.mOptions.empty())
				{
					int index = (int)entry.mSelectedIndex;
					for (int tries = 0; tries < 100; ++tries)
					{
						if (index >= (int)entry.mOptions.size() - 1)
							break;

						++index;
						if (entry.mOptions[index].mVisible)		// Continue for invisible options, so they get skipped
						{
							entry.mSelectedIndex = index;
							return UpdateResult::OPTION_CHANGED;
						}
					}
				}
			}
		}
	}
	return UpdateResult::NONE;
}

bool GameMenuEntries::setSelectedIndexByValue(uint32 value)
{
	if (mEntries.empty())
		return false;

	// Check for closest match
	int difference = 0x7fffffff;
	for (size_t i = 0; i < mEntries.size(); ++i)
	{
		const int diff = std::abs((int)(mEntries[i].mData - value));
		if (diff < difference)
		{
			mSelectedEntryIndex = (int)i;
			if (diff == 0)
				return true;	// Return true instantly, as this is an exact match
			difference = diff;
		}
	}

	// Selected an entry, but it's not an exact match
	return false;
}

void GameMenuEntries::sanitizeSelectedIndex()
{
	if (mSelectedEntryIndex >= (int)mEntries.size())
	{
		mSelectedEntryIndex = mEntries.empty() ? 0 : (int)(mEntries.size() - 1);
	}
}



void GameMenuBase::playMenuSound(uint8 sfxId)
{
	// Potentially suited menu sounds:
	//  - 0x4a = grab
	//  - 0x55 = high-pitched bip
	//  - 0x5b = short bip
	//  - 0x5c = error
	//  - 0x63 = acknowledged (Starpost sound)
	//  - 0x68 = unlocked ("can now be Super")
	//  - 0x6b = funny bip
	//  - 0xac = continue
	//  - 0xad = melodic ding
	//  - 0xaf = starting
	//  - 0xb2 = error
	//  - 0xb7 = selection change (from Data Select)
	//  - 0xb8 = unlocked (emerald sound)
	AudioOut::instance().playAudioDirect(sfxId, AudioOut::SoundRegType::SOUND, AudioOut::CONTEXT_MENU + AudioOut::CONTEXT_SOUND);
}

GameMenuBase::GameMenuBase()
{
}

GameMenuBase::~GameMenuBase()
{
}

void GameMenuBase::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);
}



void GameMenuScrolling::setCurrentSelection(int selectionY1, int selectionY2)
{
	mCurrentSelectionY1 = (float)selectionY1;
	mCurrentSelectionY2 = (float)selectionY2;
}

void GameMenuScrolling::setCurrentSelection(float selectionY1, float selectionY2)
{
	mCurrentSelectionY1 = selectionY1;
	mCurrentSelectionY2 = selectionY2;
}

int GameMenuScrolling::getScrollOffsetYInt() const
{
	return roundToInt(mScrollOffsetY);
}

void GameMenuScrolling::update(float timeElapsed)
{
	const float maxScrollOffset = std::max(mCurrentSelectionY1, 0.0f);
	const float minScrollOffset = mCurrentSelectionY2 - mVisibleAreaHeight;
	const float targetY = clamp(mScrollOffsetY, minScrollOffset, maxScrollOffset);

	// Still scrolling at all?
	if (targetY == mScrollOffsetY)
	{
		mScrollingFast = false;
		return;
	}

	// Switch to fast scrolling if it's a long way to go; or even skip part of it
	if (std::fabs(targetY - mScrollOffsetY) > 500.0f)
	{
		mScrollOffsetY = clamp(mScrollOffsetY, targetY - 500.0f, targetY + 500.0f);
	}
	if (std::fabs(targetY - mScrollOffsetY) > 100.0f)
	{
		mScrollingFast = true;
	}

	const float maxStep = timeElapsed * (mScrollingFast ? 1200.0f : 450.0f);
	if (mScrollOffsetY < targetY)
	{
		mScrollOffsetY = std::min(mScrollOffsetY + maxStep, targetY);
	}
	else
	{
		mScrollOffsetY = std::max(mScrollOffsetY - maxStep, targetY);
	}
}
