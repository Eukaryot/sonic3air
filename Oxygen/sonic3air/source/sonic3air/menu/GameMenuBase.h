/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class GameMenuEntry
{
public:
	enum class VisibilityFlag : uint8
	{
		VISIBLE		 = 0x01,
		INTERACTABLE = 0x02,
	};

	struct Option
	{
		std::string mText;
		size_t mIndex = 0;
		uint32 mValue = 0;
		bool mVisible = true;
	};

	struct AnimationData	// Free-to-use data for anything animation related
	{
		float mVisibility = 0.0f;
		float mHighlight = 0.0f;
		Vec2f mOffset;
	};

	struct RenderContext
	{
		Drawer* mDrawer = nullptr;
		Vec2i mCurrentPosition;
		bool mIsSelected = false;
		float mDeltaSeconds = 0.0f;

		template<typename T> T& as()  { return *static_cast<T*>(this); }
	};

public:
	std::string mText;
	uint32 mData = 0;

	std::vector<Option> mOptions;
	size_t mSelectedIndex = 0;

	int mMarginAbove = 0;
	int mMarginBelow = 0;

	AnimationData mAnimation;

public:
	inline GameMenuEntry() {}
	inline GameMenuEntry(const std::string& text, uint32 data) : mText(text), mData(data) {}
	virtual ~GameMenuEntry() {}

	void performRenderEntry(RenderContext& renderContext);

	inline GameMenuEntry& initEntry(const std::string& text, uint32 data)  { mText = text; mData = data; return *this; }

	inline uint32 getMenuEntryType() const  { return mMenuEntryType; }

	Option& addOptionRef(const std::string& text, uint32 value = 0);
	GameMenuEntry& addOption(const std::string& text, uint32 value = 0);
	GameMenuEntry& addNumberOptions(int minValue, int maxValue, int step);
	GameMenuEntry& addPercentageOptions(int minValue, int maxValue, int step);
	Option* getOptionByValue(uint32 value);

	inline bool isVisible() const			{ return (mVisibilityFlags & (uint8)VisibilityFlag::VISIBLE) != 0; }
	inline bool isInteractable() const		{ return (mVisibilityFlags & (uint8)VisibilityFlag::INTERACTABLE) != 0; }
	inline bool isFullyInteractable() const	{ return isVisible() && isInteractable(); }
	void setVisible(bool visible);
	void setInteractable(bool interactable);

	inline bool hasSelected() const  { return mSelectedIndex < mOptions.size(); }
	inline const Option& selected() const  { return mOptions[mSelectedIndex]; }
	bool setSelectedIndexByValue(uint32 value);
	bool sanitizeSelectedIndex(bool allowInvisibleEntries = false);

	size_t getPreviousVisibleIndex() const;
	size_t getNextVisibleIndex() const;

public:
	virtual void keyboard(const rmx::KeyboardEvent& ev) {}
	virtual void textinput(const rmx::TextInputEvent& ev) {}

protected:
	virtual void renderEntry(RenderContext& renderContext) {}

protected:
	uint32 mMenuEntryType = 0;
	uint8 mVisibilityFlags = (uint8)VisibilityFlag::VISIBLE | (uint8)VisibilityFlag::INTERACTABLE;
};


class GameMenuEntries
{
public:
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
	~GameMenuEntries();

	inline const std::vector<GameMenuEntry*>& getEntries() const  { return mEntries; }

	void clear();
	void reserve(size_t count);
	void resize(size_t count);

	GameMenuEntry* getEntryByData(uint32 data);

	GameMenuEntry& addEntry(const std::string& text = "", uint32 data = 0);

	template<typename T> T& addEntry()
	{
		T* entry = new T();
		mEntries.push_back(entry);
		return *entry;
	}

	void insertCopy(const GameMenuEntry& toCopy, size_t index);
	void insertByReference(GameMenuEntry& entry, size_t index);
	void destroy(size_t index);
	void erase(size_t index);

	void swapEntries(size_t indexA, size_t indexB);

	inline bool empty() const	{ return mEntries.empty(); }
	inline size_t size() const	{ return mEntries.size(); }
	inline GameMenuEntry& operator[](size_t index)  { return *mEntries[index]; }
	inline const GameMenuEntry& operator[](size_t index) const  { return *mEntries[index]; }

	UpdateResult update();

	inline bool hasSelected() const  { return (mSelectedEntryIndex < (int)mEntries.size()); }
	inline GameMenuEntry& selected()  { return *mEntries[mSelectedEntryIndex]; }
	bool setSelectedIndexByValue(uint32 value);
	bool sanitizeSelectedIndex(bool allowNonInteractableEntries = false);

private:
	std::vector<GameMenuEntry*> mEntries;
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
	virtual void setBaseState(BaseState baseState) {}
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



// TODO: Move this into a helper cpp/h

class GameMenuControlsDisplay
{
public:
	void clear();
	void addControl(std::string_view displayText, bool alignRight, std::string_view spriteName);
	void addControl(std::string_view displayText, bool alignRight, std::string_view spriteName, std::string_view additionalSpriteName);

	void render(Drawer& drawer, float visibility = 1.0f);

private:
	struct Control
	{
		std::string mDisplayText;
		bool mAlignRight = false;
		std::vector<uint64> mSpriteKeys;
	};

	std::vector<Control> mControls;
};
