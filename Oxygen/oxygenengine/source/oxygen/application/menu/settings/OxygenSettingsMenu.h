/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/menu/loui/LouiVerticalLayout.h"
#include "oxygen/drawing/DrawerTexture.h"

class SettingsSubMenu;


class OxygenSettingsMenu : public loui::Widget
{
public:
	void init();

	inline bool shouldBeOpen() const  { return mShouldBeOpen; }
	void setOpen(bool open)  { mShouldBeOpen = open; }

	virtual void update(loui::UpdateInfo& updateInfo) override;
	virtual void render(loui::RenderInfo& renderInfo) override;

protected:
	virtual void applyLayouting() override;

private:
	struct SubMenu
	{
		enum class Type
		{
			//GENERAL,
			DISPLAY,
			AUDIO,
			//CONTROLS,
			_NUM
		};

		Type mType;
		std::string mDisplayName;
		loui::Button mOverviewButton;
		SettingsSubMenu* mSubMenu = nullptr;
	};

	static const int NUM_SUBMENUS = (int)SubMenu::Type::_NUM;

private:
	void setupSubMenu(SubMenu::Type type, std::string_view displayName);

private:
	loui::VerticalLayout mOverviewLayout;
	SubMenu mSubMenus[NUM_SUBMENUS];

	DrawerTexture mBackground;

	bool mShouldBeOpen = false;
	float mVisibility = 0.0f;
};
