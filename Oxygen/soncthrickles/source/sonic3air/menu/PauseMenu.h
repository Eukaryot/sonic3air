/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"


class PauseMenu : public GameMenuBase
{
public:
	PauseMenu();
	~PauseMenu();

	virtual BaseState getBaseState() const override;
	virtual void onFadeIn() override;
	virtual bool canBeRemoved() override;

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	inline void enableRestart(bool enable)  { mRestartEnabled = enable; }

	void onReturnFromOptions();

private:
	enum class State
	{
		INACTIVE,
		APPEAR,
		SHOW,
		DIALOG_RESTART,
		DIALOG_EXIT,
		DISAPPEAR_RESUME,
		DISAPPEAR_EXIT
	};

private:
	void resumeGame();
	void exitGame();

private:
	GameMenuEntries mMenuEntries;
	GameMenuEntries mDialogEntries;

	State mState = State::INACTIVE;
	float mVisibility = 0.0f;
	float mDialogVisibility = 0.0f;
	float mTimeShown = 0.0f;
	bool mScreenshotMode = false;
	bool mRestartEnabled = false;
};
