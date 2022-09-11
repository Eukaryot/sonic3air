/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"
#include "oxygen/drawing/DrawerTexture.h"

class MenuBackground;
class Mod;


class ModsMenu : public GameMenuBase
{
public:
	ModsMenu(MenuBackground& menuBackground);
	~ModsMenu();

	virtual BaseState getBaseState() const override;
	virtual void setBaseState(BaseState baseState) override;
	virtual void onFadeIn() override;
	virtual bool canBeRemoved() override;

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

private:
	enum class State
	{
		INACTIVE,
		APPEAR,
		SHOW,
		APPLYING_CHANGES,
		FADE_TO_MENU
	};

private:
	int getInfoOverlayHeight() const;
	bool applyModChanges(bool dryRun = false);
	void goBack();

private:
	MenuBackground* mMenuBackground = nullptr;

	struct ModResources
	{
		DrawerTexture mLargeIcon;
		DrawerTexture mSmallIcon;
		DrawerTexture mSmallIconGray;
	};
	std::map<Mod*, ModResources> mModResources;

	struct ModEntry
	{
		Mod* mMod = nullptr;
		ModResources* mModResources = nullptr;
		bool mMakeActive = false;
	};
	std::vector<ModEntry> mModEntries;
	std::vector<Mod*> mFailedMods;
	bool mHasAnyMods = false;

	struct Tab
	{
		GameMenuEntries mMenuEntries;
		GameMenuScrolling mScrolling;
	};
	Tab mTabs[2];
	size_t mActiveTab = 0;
	float mActiveTabAnimated = 0.0f;

	struct InfoOverlay
	{
		bool mShouldBeVisible = false;
		float mVisibility = 0.0f;
		Mod* mShownMod = nullptr;
		std::vector<std::wstring> mDescriptionLines;
	};
	InfoOverlay mInfoOverlay;

	State mState = State::INACTIVE;
	float mVisibility = 0.0f;
	float mFadeInDelay = 0.0f;
	uint32 mApplyingChangesFrameCounter = 0;
};
