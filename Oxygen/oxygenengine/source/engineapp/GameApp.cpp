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
