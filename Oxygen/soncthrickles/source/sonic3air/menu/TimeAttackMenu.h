/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"

class MenuBackground;


class TimeAttackMenu : public GameMenuBase
{
public:
	TimeAttackMenu(MenuBackground& menuBackground);
	~TimeAttackMenu();

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
	void triggerStartGame();
	void startGame();
	void backToMainMenu();

private:
	MenuBackground* mMenuBackground = nullptr;

	GameMenuEntries mMenuEntries;
	GameMenuEntry* mZoneEntry = nullptr;
	GameMenuEntry* mActEntry = nullptr;
	GameMenuEntry* mCharacterEntry = nullptr;

	State mState = State::INACTIVE;
	float mVisibility = 0.0f;
	int mPreferredAct = 0;	// 0 or 1

	std::vector<std::string> mBestTimes;
	uint8 mBestTimesForCharacters = 0xff;
	uint16 mBestTimesForZoneAct = 0xffff;
};
