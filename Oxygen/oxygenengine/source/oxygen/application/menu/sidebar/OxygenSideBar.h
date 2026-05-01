/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiVerticalLayout.h"
#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/drawing/DrawerTexture.h"


class OxygenSideBar : public loui::Widget
{
public:
	void init();

	inline bool shouldBeOpen() const  { return mShouldBeOpen; }
	void setOpen(bool open);

	virtual void update(loui::UpdateInfo& updateInfo) override;
	virtual void render(loui::RenderInfo& renderInfo) override;

private:
	loui::VerticalLayout mButtonLayout;
	DrawerTexture mBackground;

	loui::Button mContinueButton;
	loui::Button mSettingsButton;

	bool mShouldBeOpen = false;
	float mVisibility = 0.0f;
};
