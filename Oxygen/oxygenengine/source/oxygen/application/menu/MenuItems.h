/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Drawer;


struct MenuItem
{
	Recti mRect;

	virtual ~MenuItem() {}

	virtual void renderItem(Drawer& drawer) = 0;
};


struct LabelMenuItem : public MenuItem
{
	std::string mText;
	Font* mFont = nullptr;

	void renderItem(Drawer& drawer) override;
};


struct ButtonMenuItem : public MenuItem
{
	std::string mText;
	Font* mFont = nullptr;
	uint32 mData = 0;

	void renderItem(Drawer& drawer) override;
};


class MenuItemContainer
{
public:
	inline ~MenuItemContainer() { clear(); }

	void clear();

	template<typename T>
	T& createItem()
	{
		T* item = new T();
		mMenuItems.push_back(item);
		return *item;
	}

	void layoutItems(const Recti& outerRect);
	void renderItems(Drawer& drawer);

	std::vector<MenuItem*> mMenuItems;
};
