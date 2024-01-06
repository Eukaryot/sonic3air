/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/GameMenuBase.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/audio/AudioOut.h"

#include "oxygen/application/input/InputManager.h"


void GameMenuEntry::performRenderEntry(RenderContext& renderContext)
{
	renderContext.mCurrentPosition.y += mMarginAbove;

	// Call virtual method
	renderEntry(renderContext);

	renderContext.mCurrentPosition.y += mMarginBelow;
}

GameMenuEntry::Option& GameMenuEntry::addOptionRef(const std::string& text, uint32 value)
{
	Option& option = vectorAdd(mOptions);
	option.mText = text;
	option.mIndex = mOptions.size() - 1;
	option.mValue = value;
	return option;
}

GameMenuEntry& GameMenuEntry::addOption(const std::string& text, uint32 value)
{
	addOptionRef(text, value);
	return *this;
}

GameMenuEntry& GameMenuEntry::addNumberOptions(int minValue, int maxValue, int step)
{
	for (int value = minValue; value <= maxValue; value += step)
		addOptionRef(std::to_string(value), value);
	return *this;
}

GameMenuEntry& GameMenuEntry::addPercentageOptions(int minValue, int maxValue, int step)
{
	for (int value = minValue; value <= maxValue; value += step)
		addOptionRef(std::to_string(value) + '%', value);
	return *this;
}

GameMenuEntry::Option* GameMenuEntry::getOptionByValue(uint32 value)
{
	for (size_t i = 0; i < mOptions.size(); ++i)
	{
		if (mOptions[i].mValue == value)
			return &mOptions[i];
	}
	return nullptr;
}

void GameMenuEntry::setVisible(bool visible)
{
	if (visible)
		mVisibilityFlags |= (uint8)VisibilityFlag::VISIBLE;
	else
		mVisibilityFlags &= ~(uint8)VisibilityFlag::VISIBLE;
}

void GameMenuEntry::setInteractable(bool interactable)
{
	if (interactable)
		mVisibilityFlags |= (uint8)VisibilityFlag::INTERACTABLE;
	else
		mVisibilityFlags &= ~(uint8)VisibilityFlag::INTERACTABLE;
}

bool GameMenuEntry::setSelectedIndexByValue(uint32 value)
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

bool GameMenuEntry::changeSelectedIndex(int change)
{
	if (change < 0)
	{
		const size_t newIndex = getPreviousVisibleIndex();
		if (newIndex != mSelectedIndex)
		{
			mSelectedIndex = newIndex;
			return true;
		}
	}
	else if (change > 0)
	{
		const size_t newIndex = getNextVisibleIndex();
		if (newIndex != mSelectedIndex)
		{
			mSelectedIndex = newIndex;
			return true;
		}
	}
	return false;
}

bool GameMenuEntry::sanitizeSelectedIndex(bool allowInvisibleEntries)
{
	if (mSelectedIndex >= mOptions.size())
	{
		if (mOptions.empty())
			return false;

		mSelectedIndex = mOptions.size() - 1;
	}

	if (!allowInvisibleEntries && !mOptions[mSelectedIndex].mVisible)
	{
		for (int index = (int)mSelectedIndex - 1; index >= 0; --index)
		{
			if (mOptions[index].mVisible)
			{
				mSelectedIndex = (size_t)index;
				return true;
			}
		}
		for (size_t index = mSelectedIndex + 1; index < mOptions.size(); ++index)
		{
			if (mOptions[index].mVisible)
			{
				mSelectedIndex = index;
				return true;
			}
		}
		return false;
	}
	return true;
}

size_t GameMenuEntry::getPreviousVisibleIndex() const
{
	if (mOptions.empty())
		return 0;

	size_t index = mSelectedIndex;
	if (index == 0)
	{
		// Can't go further back, check if the current index is visible
		if (mOptions[index].mVisible)
			return index;

		// Advance to a visible index
		const size_t lastIndex = mOptions.size() - 1;
		while (index < lastIndex)
		{
			++index;
			if (mOptions[index].mVisible)
				return index;
		}
		return 0;
	}
	else
	{
		// Go back to the previous visible index
		while (index > 0)
		{
			--index;
			if (mOptions[index].mVisible)
				return index;
		}
		return mSelectedIndex;
	}
}

size_t GameMenuEntry::getNextVisibleIndex() const
{
	if (mOptions.empty())
		return 0;

	size_t index = mSelectedIndex;
	const size_t lastIndex = mOptions.size() - 1;
	if (index >= lastIndex)
	{
		// Can't advance further, check if the current index is visible
		if (mOptions[index].mVisible)
			return index;

		// Go back to a visible index
		while (index > 0)
		{
			--index;
			if (mOptions[index].mVisible)
				return index;
		}
		return lastIndex;
	}
	else
	{
		// Advance to the next visible index
		while (index < lastIndex)
		{
			++index;
			if (mOptions[index].mVisible)
				return index;
		}
		return mSelectedIndex;
	}
}



GameMenuEntries::~GameMenuEntries()
{
	clear();
}

void GameMenuEntries::clear()
{
	for (GameMenuEntry* entry : mEntries)
		delete entry;
	mEntries.clear();
}

void GameMenuEntries::reserve(size_t count)
{
	mEntries.reserve(count);
}

void GameMenuEntries::resize(size_t count)
{
	mEntries.resize(count);
}

GameMenuEntry* GameMenuEntries::getEntryByData(uint32 data)
{
	for (GameMenuEntry* entry : mEntries)
	{
		if (entry->mData == data)
			return entry;
	}
	return nullptr;
}

GameMenuEntry& GameMenuEntries::addEntry(const std::string& text, uint32 data)
{
	GameMenuEntry* entry = new GameMenuEntry(text, data);
	mEntries.push_back(entry);
	return *entry;
}

void GameMenuEntries::insertCopy(const GameMenuEntry& toCopy, size_t index)
{
	GameMenuEntry* newEntry = new GameMenuEntry(toCopy);
	mEntries.insert(mEntries.begin() + index, newEntry);
}

void GameMenuEntries::insertByReference(GameMenuEntry& entry, size_t index)
{
	mEntries.insert(mEntries.begin() + index, &entry);
}

void GameMenuEntries::destroy(size_t index)
{
	delete mEntries[index];
	mEntries.erase(mEntries.begin() + index);
}

void GameMenuEntries::erase(size_t index)
{
	mEntries.erase(mEntries.begin() + index);
}

void GameMenuEntries::swapEntries(size_t indexA, size_t indexB)
{
	GameMenuEntry* tmp = mEntries[indexA];
	mEntries[indexA] = mEntries[indexB];
	mEntries[indexB] = tmp;
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
		const int entryChange = getEntryChangeByInput();
		if (entryChange != 0)
		{
			changeSelectedIndex(entryChange);
			return UpdateResult::ENTRY_CHANGED;
		}

		const int optionChange = getOptionChangeByInput();
		if (optionChange != 0)
		{
			if (mEntries[mSelectedEntryIndex]->changeSelectedIndex(optionChange))
			{
				return UpdateResult::OPTION_CHANGED;
			}
		}
	}
	return UpdateResult::NONE;
}

int GameMenuEntries::getEntryChangeByInput() const
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
	return entryChange;
}

int GameMenuEntries::getOptionChangeByInput() const
{
	const InputManager::ControllerScheme& controller = InputManager::instance().getController(0);
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
	return optionChange;
}

bool GameMenuEntries::setSelectedIndexByValue(uint32 value)
{
	if (mEntries.empty())
		return false;

	// Check for closest match
	int difference = 0x7fffffff;
	for (size_t i = 0; i < mEntries.size(); ++i)
	{
		const int diff = std::abs((int)(mEntries[i]->mData - value));
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

bool GameMenuEntries::changeSelectedIndex(int change, bool loop)
{
	const int originalIndex = mSelectedEntryIndex;
	if (change < 0)
	{
		mSelectedEntryIndex = getPreviousInteractableIndex(mSelectedEntryIndex, loop);
	}
	else if (change > 0)
	{
		mSelectedEntryIndex = getNextInteractableIndex(mSelectedEntryIndex, loop);
	}
	return (mSelectedEntryIndex != originalIndex);
}

bool GameMenuEntries::sanitizeSelectedIndex(bool allowNonInteractableEntries)
{
	if (mSelectedEntryIndex >= (int)mEntries.size())
	{
		if (mEntries.empty())
			return false;

		mSelectedEntryIndex = (int)mEntries.size() - 1;
	}

	if (!allowNonInteractableEntries && !mEntries[mSelectedEntryIndex]->isFullyInteractable())
	{
		for (int index = (int)mSelectedEntryIndex - 1; index >= 0; --index)
		{
			if (mEntries[index]->isFullyInteractable())
			{
				mSelectedEntryIndex = index;
				return true;
			}
		}
		for (size_t index = mSelectedEntryIndex + 1; index < mEntries.size(); ++index)
		{
			if (mEntries[index]->isFullyInteractable())
			{
				mSelectedEntryIndex = (int)index;
				return true;
			}
		}
		return false;
	}
	return true;
}

size_t GameMenuEntries::getPreviousInteractableIndex(size_t index, bool loop) const
{
	if (mEntries.empty())
		return 0;

	const size_t originalIndex = index;
	for (int tries = 0; tries < (int)mEntries.size(); ++tries)
	{
		if (index > 0)
			--index;
		else if (loop)
			index = mEntries.size() - 1;
		else
			break;	// Failed

		// Continue for invisible and non-interactable entries, so they get skipped
		if (mEntries[index]->isFullyInteractable())
		{
			// Found a valid index
			return index;
		}
	}
	return originalIndex;
}

size_t GameMenuEntries::getNextInteractableIndex(size_t index, bool loop) const
{
	if (mEntries.empty())
		return 0;

	const size_t initialIndex = index;
	for (int tries = 0; tries < (int)mEntries.size(); ++tries)
	{
		if (index < mEntries.size() - 1)
			++index;
		else if (loop)
			index = 0;
		else
			break;	// Failed

		// Continue for invisible and non-interactable entries, so they get skipped
		if (mEntries[index]->isFullyInteractable())
		{
			// Found a valid index
			return index;
		}
	}
	return initialIndex;
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
