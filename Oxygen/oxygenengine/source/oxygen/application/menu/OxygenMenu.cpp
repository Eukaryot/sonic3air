/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/OxygenMenu.h"
#include "oxygen/application/menu/MenuItems.h"
#include "oxygen/application/EngineMain.h"


OxygenMenu::OxygenMenu()
{
}

OxygenMenu::~OxygenMenu()
{
}

void OxygenMenu::initialize()
{
	Font& font = EngineMain::getDelegate().getDebugFont(10);

	mMenuItems.clear();
	{
		LabelMenuItem& item = mMenuItems.createItem<LabelMenuItem>();
		item.mText = "Title";
		item.mFont = &font;
	}
	{
		ButtonMenuItem& item = mMenuItems.createItem<ButtonMenuItem>();
		item.mText = "Button";
		item.mFont = &font;
	}
}

void OxygenMenu::deinitialize()
{
}

void OxygenMenu::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);

	mMenuItems.layoutItems(Recti(0, 0, (int)getRect().width, 0));
}

void OxygenMenu::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();
	drawer.drawRect(getRect(), Color(0.1f, 0.2f, 0.4f, 0.9f));

	mMenuItems.renderItems(drawer);

	drawer.performRendering();
}
