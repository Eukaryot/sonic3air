/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class GameMenuEntries
{
public:
	enum class Visibility
	{
		INVISIBLE,	// Entry or option is not visible and can't be selected
		DISABLED,	// Entry or option is greyed out and can't be selected
		VISIBLE,	// Entry or option is visible and can be selected
	};

	struct Option
	{
		std::string mText;
		size_t mIndex = 0;
		uint32 mValue = 0;
		bool mVisible = true;
	};

	struct Entry
	{
		struct AnimationData	// Free-to-use data for anything animation related
		{
			float mVisibility = 0.0f;
			float mHighlight = 0.0f;
			Vec2f mOffset;
		};

		std::string mText;
		uint32 mData = 0;
		std::vector<Option> mOptions;
		size_t mSelectedIndex = 0;
		Visibility mVisibility = Visibility::VISIBLE;
		AnimationData mAnimation;

		Option& addOptionRef(const std::string& text, uint32 value = 0);
		Entry& addOption(const std::string& text, uint32 value = 0);
		Entry& addNumberOptions(int minValue, int maxValue, int step);
		Entry& addPercentageOptions(int minValue, int maxValue, int step);
		Option* getOptionByValue(uint32 value);

		bool isVisible() const  { return mVisibility != Visibility::INVISIBLE; }
		bool isEnabled() const  { return mVisibility == Visibility::VISIBLE; }
		void setVisible(bool visible);
		void setEnabled(bool enabled);

		inline bool hasSelected() const  { return mSelectedIndex < mOptions.size(); }
		inline const Option& selected() const  { return mOptions[mSelectedIndex]; }
		bool setSelectedIndexByValue(uint32 value);
		void sanitizeSelectedIndex();
	};

	enum class Navigation
	{
		VERTICAL,
		HORIZONTAL
	};

	enum class UpdateResult
	{
		NONE,
		ENTRY_CHANGED,
		OPTION_CHANGED
	};

public:
	int mSelectedEntryIndex = 0;
	Navigation mNavigation = Navigation::VERTICAL;

public:
	inline const std::vector<Entry>& getEntries() const  { return mEntries; }

	inline void clear()  { mEntries.clear(); }
	void reserve(size_t count);
	void resize(size_t count);
	Entry& addEntry(const std::string& text = "", uint32 data = 0);
	Entry* getEntryByData(uint32 data);

	void insert(const Entry& toCopy, size_t index);
	void erase(size_t index);

	inline bool empty() const	{ return mEntries.empty(); }
	inline size_t size() const	{ return mEntries.size(); }
	inline Entry& operator[](size_t index)  { return mEntries[index]; }
	inline const Entry& operator[](size_t index) const  { return mEntries[index]; }

	UpdateResult update();

	inline bool hasSelected() const  { return (mSelectedEntryIndex < (int)mEntries.size()); }
	inline Entry& selected()  { return mEntries[mSelectedEntryIndex]; }
	bool setSelectedIndexByValue(uint32 value);
	void sanitizeSelectedIndex();

private:
	std::vector<Entry> mEntries;
};


class GameMenuBase : public GuiBase
{
public:
	enum class BaseState
	{
		INACTIVE,
		FADE_IN,
		SHOW,
		FADE_OUT
	};

public:
	static void playMenuSound(uint8 sfxId);

public:
	GameMenuBase();
	virtual ~GameMenuBase();

	virtual void update(float timeElapsed) override;

	virtual BaseState getBaseState() const { return BaseState::SHOW; }
	virtual void onFadeIn() {}
	virtual bool canBeRemoved() { return false; }
};


class GameMenuScrolling
{
public:
	inline void setVisibleAreaHeight(float height)  { mVisibleAreaHeight = height; }
	void setCurrentSelection(int selectionY1, int selectionY2);
	void setCurrentSelection(float selectionY1, float selectionY2);

	inline float getScrollOffsetY() const  { return mScrollOffsetY; }
	int getScrollOffsetYInt() const;
	void update(float timeElapsed);

public:
	float mVisibleAreaHeight = 224.0f;
	float mCurrentSelectionY1 = 0.0f;
	float mCurrentSelectionY2 = 0.0f;
	float mScrollOffsetY = 0.0f;
	bool mScrollingFast = false;
};
