/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/MenuItems.h"
#include "oxygen/drawing/Drawer.h"


void LabelMenuItem::renderItem(Drawer& drawer)
{
	if (nullptr != mFont)
	{
		drawer.printText(*mFont, mRect, mText, 5);
	}
}


void ButtonMenuItem::renderItem(Drawer& drawer)
{
	if (nullptr != mFont)
	{
		drawer.printText(*mFont, mRect, mText, 5);
	}
}


void MenuItemContainer::clear()
{
	for (MenuItem* menuItem : mMenuItems)
	{
		delete menuItem;
	}
	mMenuItems.clear();
}

void MenuItemContainer::layoutItems(const Recti& outerRect)
{
	Recti rect = outerRect;
	for (MenuItem* menuItem : mMenuItems)
	{
		rect.height = 30;
		menuItem->mRect = rect;
		rect.y += rect.height;
	}
}

void MenuItemContainer::renderItems(Drawer& drawer)
{
	for (MenuItem* menuItem : mMenuItems)
	{
		menuItem->renderItem(drawer);
	}
}
