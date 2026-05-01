/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/helper/ScaledScreenRect.h"
#include "oxygen/menu/loui/LouiWidget.h"
#include <rmxmedia.h>


class OxygenMenu : public GuiBase, public SingleInstance<OxygenMenu>
{
public:
	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float deltaSeconds) override;
	virtual void render() override;

	bool isSideBarOpen() const;
	void openSideBar();
	void closeSideBar();

	void openSettingsMenu();
	void closeSettingsMenu();

private:
	void refreshMenuResolution();

private:
	enum class TriggeredAction
	{
		NONE,
		OPEN_SIDE_BAR,
		CLOSE_SIDE_BAR,
		OPEN_SETTINGS,
		CLOSE_SETTINGS,
	};

private:
	ScaledScreenRect mOxygenMenuViewport;
	DrawerTexture mOxygenMenuTexture;

	Vec2i mMenuResolution;
	Vec2i mUpscaledResolution;

	loui::UpdateInfo mUpdateInfo;
	loui::Widget mRootWidget;

	class OxygenSideBar* mSideBar = nullptr;
	class OxygenSettingsMenu* mSettingsMenu = nullptr;

	TriggeredAction mTriggeredAction = TriggeredAction::NONE;

	Recti mCoveredScreenRect;
};
