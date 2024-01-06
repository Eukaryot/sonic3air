/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"


class TimeAttackResultsMenu : public GameMenuBase
{
public:
	TimeAttackResultsMenu();
	~TimeAttackResultsMenu();

	virtual void onFadeIn() override;

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void setYourTime(int hundreds);
	void addOtherTime(int hundreds);

private:
	GameMenuEntries mMenuEntries;

	float mTime = 0.0f;
	bool mGameStopped = false;

	int mYourTime = 0;
	std::vector<int> mBetterTimes;
	std::vector<int> mWorseTimes;
};
