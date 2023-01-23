/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class SkippableCutsceneWindow : public GuiBase
{
public:
	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void show(bool visible);
	inline bool canBeRemoved() const  { return !mShouldBeVisible && mVisibility <= 0.0f; }

private:
	bool mShouldBeVisible = false;
	float mVisibility = 0.0f;
	float mAnimationTimer = 0.0f;
};
