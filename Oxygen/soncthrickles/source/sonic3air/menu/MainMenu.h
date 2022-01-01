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


class MainMenu : public GameMenuBase
{
public:
	MainMenu(MenuBackground& menuBackground);
	~MainMenu();

	virtual BaseState getBaseState() const override;
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
		FADE_TO_TITLESCREEN,
		FADE_TO_DATASELECT,
		FADE_TO_SUBMENU,
		FADE_TO_EXIT
	};

private:
	void triggerStartNormalGame();
	void startNormalGame();
	void openActSelectMenu();
	void openTimeAttack();
	void openOptions();
	void openExtras();
	void openMods();
	void exitGame();

private:
	MenuBackground* mMenuBackground = nullptr;
	GameMenuEntries mMenuEntries;

	State mState = State::INACTIVE;
	float mVisibility = 0.0f;

	bool mCheckedModErrors = false;
	std::vector<std::string> mErrorLines;
};
