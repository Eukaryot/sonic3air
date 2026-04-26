/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/menu/sidebar/OxygenSideBar.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/helper/ScaledScreenRect.h"
#include <rmxmedia.h>


class OxygenMenu : public GuiBase
{
public:
	inline bool isVisible() const  { return mIsVisible; }
	void setVisible(bool visible);

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float deltaSeconds) override;
	virtual void render() override;

private:
	int getMenuScale() const;

private:
	ScaledScreenRect mOxygenMenuViewport;
	DrawerTexture mOxygenMenuTexture;
	int mMenuScale = 1;

	loui::UpdateInfo mUpdateInfo;
	loui::Widget mRootWidget;

	OxygenSideBar mSideBar;

	bool mIsVisible = false;
	float mVisibility = 0.0f;
	Recti mCoveredScreenRect;
};
