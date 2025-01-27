/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/overlays/DebugSidePanel.h"
#include <functional>


class DebugSidePanelCategory
{
public:
	static const uint64 INVALID_KEY = 0xffffffffffffffffULL;

	enum class Type
	{
		INTERNAL,	// Provided by Oxygen Engine
		CUSTOM,		// Provided by scripts
	};

public:
	virtual ~DebugSidePanelCategory() {}

public:
	// Meta data
	size_t mIdentifier = 0;
	std::string mHeader;
	char mShortCharacter = 0;
	Type mType = Type::INTERNAL;

	// Runtime data
	int mScrollSize = 0;
	int mScrollPosition = 0;
	std::set<uint64> mOpenKeys;
	uint64 mChangedKey = INVALID_KEY;
};


class CustomDebugSidePanelCategory : public DebugSidePanelCategory
{
friend class CustomSidePanelWindow;

public:
	struct Option
	{
		std::string mText;
		bool mChecked = false;
		bool mUpdated = false;		// Set if updated this frame via "addOption"; otherwise don't show
		uint64 mKey = 0;
	};

	struct Line
	{
		std::string mText;
		int mIndent = 0;
		Color mColor = Color::WHITE;
	};

	struct Entry
	{
		std::vector<Line> mLines;
		bool mCanBeHovered = false;
		bool mIsHovered = false;
		bool mUpdated = false;		// Set if updated this frame via "addEntry"; otherwise don't show
		uint64 mKey = 0;
	};

public:
	CustomDebugSidePanelCategory();

	void onSetup();
	bool addOption(std::string_view text, bool defaultValue);
	void addEntry(uint64 key);
	void addLine(std::string_view text, int indent, const Color& color);
	bool isEntryHovered(uint64 key);

	inline bool isVisibleInDevModeWindow() const		 { return mVisibleInDevModeWindow; }
	inline void setVisibleInDevModeWindow(bool visible)	 { mVisibleInDevModeWindow = visible; }

	void buildCategoryContent(DebugSidePanel::Builder& builder, Drawer& drawer, uint64 mouseOverKey);

private:
	std::vector<Option> mOptions;
	std::vector<Entry> mEntries;

	Entry* mCurrentEntry = nullptr;
	bool mEntriesNeedSorting = false;
	bool mVisibleInDevModeWindow = false;
};
