/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "engineapp/pch.h"
#include "engineapp/GameApp.h"

#include "oxygen/application/Application.h"
#include "oxygen/simulation/Simulation.h"


GameApp::GameApp()
{
}

GameApp::~GameApp()
{
}

void GameApp::initialize()
{
	Simulation& simulation = Application::instance().getSimulation();
	simulation.setRunning(true);
}

void GameApp::deinitialize()
{
}

void GameApp::mouse(const rmx::MouseEvent& ev)
{
	GuiBase::mouse(ev);
}

void GameApp::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);
}

void GameApp::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);
}

void GameApp::render()
{
	GuiBase::render();
}
