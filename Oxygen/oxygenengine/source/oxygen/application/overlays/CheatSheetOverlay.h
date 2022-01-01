/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class CheatSheetOverlay : public GuiBase
{
public:
	CheatSheetOverlay();
	~CheatSheetOverlay();

	inline void show(bool enable) { mShouldBeShown = enable; }
	inline void toggle() { mShouldBeShown = !mShouldBeShown; }

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

private:
	bool mShouldBeShown = false;
	float mVisibility = 0.0f;
};
