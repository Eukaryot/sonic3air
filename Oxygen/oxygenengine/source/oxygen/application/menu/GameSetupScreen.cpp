/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/GameSetupScreen.h"
#include "oxygen/application/EngineMain.h"


GameSetupScreen::GameSetupScreen()
{
}

GameSetupScreen::~GameSetupScreen()
{
}

void GameSetupScreen::initialize()
{
}

void GameSetupScreen::deinitialize()
{
}

void GameSetupScreen::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);
}

void GameSetupScreen::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();
	Font& font = EngineMain::getDelegate().getDebugFont(10);

	drawer.drawRect(getRect(), Color(0.1f, 0.2f, 0.4f));

	drawer.printText(font, getRect(), "Loading...", 5);

	drawer.performRendering();
}
