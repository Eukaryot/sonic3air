/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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

					// Continue for invisible and non-interactable entries, so they get skipped
					if (mEntries[mSelectedEntryIndex]->isFullyInteractable())
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

					// Continue for invisible and non-interactable entries, so they get skipped
					if (mEntries[mSelectedEntryIndex]->isFullyInteractable())
						break;
				}
			}
			return UpdateResult::ENTRY_CHANGED;
		}

		if (optionChange != 0)
		{
			if (optionChange < 0)
			{
				GameMenuEntry& entry = *mEntries[mSelectedEntryIndex];
				const size_t newIndex = entry.getPreviousVisibleIndex();
				if (newIndex != entry.mSelectedIndex)
				{
					entry.mSelectedIndex = newIndex;
					return UpdateResult::OPTION_CHANGED;
				}
			}
			else
			{
				GameMenuEntry& entry = *mEntries[mSelectedEntryIndex];
				const size_t newIndex = entry.getNextVisibleIndex();
				if (newIndex != entry.mSelectedIndex)
				{
					entry.mSelectedIndex = newIndex;
					return UpdateResult::OPTION_CHANGED;
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



void GameMenuControlsDisplay::clear()
{
	mControls.clear();
}

void GameMenuControlsDisplay::addControl(std::string_view displayText, bool alignRight, std::string_view spriteName)
{
	addControl(displayText, alignRight, spriteName, "");
}

void GameMenuControlsDisplay::addControl(std::string_view displayText, bool alignRight, std::string_view spriteName, std::string_view additionalSpriteName)
{
	const uint64 spriteKey = rmx::getMurmur2_64(spriteName);
	for (Control& control : mControls)
	{
		if (!control.mSpriteKeys.empty() && spriteKey == control.mSpriteKeys[0])
		{
			control.mDisplayText = displayText;
			return;
		}
	}

	Control& newControl = vectorAdd(mControls);
	newControl.mDisplayText = displayText;
	newControl.mAlignRight = alignRight;
	newControl.mSpriteKeys.push_back(spriteKey);

	if (!additionalSpriteName.empty())
		newControl.mSpriteKeys.push_back(rmx::getMurmur2_64(additionalSpriteName));
}

void GameMenuControlsDisplay::render(Drawer& drawer, float visibility)
{
	Font& font = global::mOxyfontTinyRect;
	Vec2i pos(0, 217 + roundToInt((1.0f - visibility) * 16));

	// Background
	drawer.drawRect(Recti(pos.x, pos.y - 1, 400, 225 - pos.y), Color(0.0f, 0.0f, 0.0f));
//	for (int k = 0; k < 6; ++k)
//		drawer.drawRect(Recti(pos.x, pos.y - 7 + k, 400, 1), Color(0.0f, 0.0f, 0.0f, 0.1f * k - 0.05f));

	// Left-aligned entries
	pos.x += 16;
	for (Control& control : mControls)
	{
		if (control.mSpriteKeys.empty() || control.mAlignRight)
			continue;

		for (uint64 spriteKey : control.mSpriteKeys)
		{
			drawer.drawSprite(pos + Vec2i(4, 0), spriteKey);
			pos.x += 16;
		}
		drawer.printText(font, Vec2i(pos.x, pos.y + 1), control.mDisplayText, 4);
		pos.x += font.getWidth(control.mDisplayText) + 15;
	}

	// Right-aligned entries
	pos.x = 400 - 10;
	for (Control& control : mControls)
	{
		if (control.mSpriteKeys.empty() || !control.mAlignRight)
			continue;

		pos.x -= font.getWidth(control.mDisplayText);
		drawer.printText(font, Vec2i(pos.x, pos.y + 1), control.mDisplayText, 4);
		pos.x -= 15;
		for (uint64 spriteKey : control.mSpriteKeys)
		{
			drawer.drawSprite(pos + Vec2i(4, 0), spriteKey);
			pos.x -= 16;
		}
	}
}
