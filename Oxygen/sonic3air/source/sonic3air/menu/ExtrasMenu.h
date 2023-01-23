/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"
#include "oxygen/drawing/DrawerTexture.h"

class MenuBackground;


class ExtrasMenu : public GameMenuBase
{
public:
	ExtrasMenu(MenuBackground& menuBackground);
	~ExtrasMenu();

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
		FADE_TO_MENU,
		FADE_TO_GAME
	};

private:
	void startCompetitionMode();
	void startBlueSphere();
	void startLevelSelect();
	void goBack();

private:
	MenuBackground* mMenuBackground = nullptr;
	DrawerTexture mTailsYawning;

	GameMenuEntries mTabMenuEntries;
	struct Tab
	{
		enum Id
		{
			EXTRAS		 = 0,
			SECRETS		 = 1,
			ACHIEVEMENTS = 2
		};

		GameMenuEntries mMenuEntries;
	};
	Tab mTabs[3];
	size_t mActiveTab = 0;
	float mActiveTabAnimated = 0.0f;
	GameMenuEntries* mActiveMenu = &mTabMenuEntries;

	State mState = State::INACTIVE;
	float mVisibility = 0.0f;
	GameMenuScrolling mScrolling;

	std::map<uint32, std::vector<std::string>> mDescriptionLinesCache;

	int mAchievementsCompleted = 0;
};
