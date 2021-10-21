/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"


class SaveStateMenu : public GuiBase
{
public:
	SaveStateMenu();
	~SaveStateMenu();

	void init(bool forLoading);

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void textinput(const rmx::TextInputEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	inline bool isActive() const  { return mIsActive; }

private:
	struct Entry
	{
		enum class Type
		{
			SAVESTATE_EMULATOR	= 0,
			SAVESTATE_LOCAL		= 1,
			NEWSAVE				= 2,
			RESET				= 3
		};
		std::wstring mName;
		Type mType = Type::NEWSAVE;
		int mPaddingBefore = 0;
	};

private:
	void addEntry(const std::wstring& name, Entry::Type type, int padding = 0);
	const std::wstring& getSaveStatesDirByType(Entry::Type type);
	void setHighlightedIndex(uint32 highlightedIndex);
	void changeHighlightedIndex(int difference);
	void onAccept(bool loadingAllowed, bool savingAllowed);

private:
	bool mIsActive;
	bool mHadFirstUpdate = false;
	bool mForLoading;
	bool mEditing = false;

	std::vector<Entry> mEntries;
	std::wstring mSaveStateDirectory[2];

	uint32 mHighlightedIndex;
	std::wstring mHighlightedName;

	bool mHasPreview = false;
	DrawerTexture mPreview;

	Font mFont;
};
