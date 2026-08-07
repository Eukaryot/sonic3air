/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>

class GuiBase;
class GameMenuBase;


class GameMenuManager : public SingleInstance<GameMenuManager>
{
public:
	void initWithRoot(GuiBase& root);
	void updateMenus();

	void addMenu(GameMenuBase& menu);
	void forceRemoveAll();

private:
	GuiBase* mRoot = nullptr;

	std::set<GameMenuBase*> mActiveMenus;
	std::vector<GameMenuBase*> mMenusToBeRemoved;
};
