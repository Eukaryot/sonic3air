/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/DebugSidePanelCategory.h"


CustomDebugSidePanelCategory::CustomDebugSidePanelCategory()
{
	mType = DebugSidePanelCategory::Type::CUSTOM;
}

void CustomDebugSidePanelCategory::onSetup()
{
	for (Option& option : mOptions)
	{
		option.mUpdated = false;
	}
	for (Entry& entry : mEntries)
	{
		entry.mUpdated = false;
		entry.mLines.clear();
	}
}

bool CustomDebugSidePanelCategory::addOption(std::string_view text, bool defaultValue)
{
	// Exists already?
	for (Option& option : mOptions)
	{
		if (option.mText == text)
		{
			option.mUpdated = true;
			return option.mChecked;
		}
	}

	// Create a new one
	if (mOptions.size() >= 32)
		return false;

	Option& option = vectorAdd(mOptions);
	option.mText = text;
	option.mChecked = defaultValue;
	option.mUpdated = true;
	option.mKey = 0x074244d9 + (uint64)(0x00502cad * mOptions.size());

	if (defaultValue)
		mOpenKeys.insert(option.mKey);

	return option.mChecked;
}

void CustomDebugSidePanelCategory::addEntry(uint64 key)
{
	for (Entry& entry : mEntries)
	{
		if (entry.mKey == key)
		{
			entry.mUpdated = true;
			mCurrentEntry = &entry;
			return;
		}
	}

	Entry& entry = vectorAdd(mEntries);
	entry.mKey = key;
	mCurrentEntry = &entry;
	mEntriesNeedSorting = true;
}

void CustomDebugSidePanelCategory::addLine(std::string_view text, int indent, const Color& color)
{
	if (nullptr == mCurrentEntry)
		return;

	Line& line = vectorAdd(mCurrentEntry->mLines);
	line.mText = text;
	line.mIndent = indent;
	line.mColor = color;
}

bool CustomDebugSidePanelCategory::isEntryHovered(uint64 key)
{
	for (Entry& entry : mEntries)
	{
		if (entry.mKey == key)
		{
			entry.mCanBeHovered = true;
			return entry.mIsHovered;
		}
	}
	return false;
}

void CustomDebugSidePanelCategory::buildCategoryContent(DebugSidePanel::Builder& builder, Drawer& drawer, uint64 mouseOverKey)
{
	// Add checkboxes for the options
	if (!mOptions.empty())
	{
		for (Option& option : mOptions)
		{
			if (!option.mUpdated)
				continue;

			option.mChecked = mOpenKeys.count(option.mKey);
			builder.addOption(option.mText, option.mChecked, Color::CYAN, 0, option.mKey);
		}
		builder.addSpacing(12);
	}

	// Sort entries if needed
	if (mEntriesNeedSorting)
	{
		std::sort(mEntries.begin(), mEntries.end(), [](Entry& a, Entry& b) { return a.mKey < b.mKey; } );
		mEntriesNeedSorting = false;
	}

	// Add entries
	for (Entry& entry : mEntries)
	{
		if (!entry.mUpdated)
			continue;

		uint64 key = DebugSidePanel::INVALID_KEY;
		if (entry.mCanBeHovered)
		{
			key = entry.mKey;
			entry.mIsHovered = (mouseOverKey == entry.mKey);
		}

		for (Line& line : entry.mLines)
		{
			builder.addLine(*String(0, "%s", line.mText.c_str()), line.mColor, line.mIndent, key);
		}
		builder.addSpacing(4);
	}
}
