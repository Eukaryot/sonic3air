/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/GameSimWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/Application.h"
#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/simulation/Simulation.h"


GameSimWindow::GameSimWindow() :
	DevModeWindowBase("Game", Category::SIMULATION, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void GameSimWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(300.0f, 5.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	Simulation& simulation = Application::instance().getSimulation();

	if (simulation.getSpeed() == 0.0f)
	{
		if (ImGui::Button("Play", ImVec2(50 * uiScale, 0)))
		{
			simulation.setSpeed(1.0f);
		}
	}
	else
	{
		if (ImGui::Button("Pause", ImVec2(50 * uiScale, 0)))
		{
			simulation.setSpeed(0.0f);
		}
	}

	ImGui::SameLine();
	ImGui::Text("Game Speed:   %.2fx", simulation.getSpeed());

	if (ImGui::Button("0.05x"))
	{
		simulation.setSpeed(0.05f);
	}
	ImGui::SameLine();
	if (ImGui::Button("0.2x"))
	{
		simulation.setSpeed(0.2f);
	}
	ImGui::SameLine();
	if (ImGui::Button("1x"))
	{
		simulation.setSpeed(1.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("3x"))
	{
		simulation.setSpeed(3.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("5x"))
	{
		simulation.setSpeed(5.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("10x"))
	{
		simulation.setSpeed(10.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Max"))
	{
		simulation.setSpeed(1000.0f);
	}

	ImGui::Text("Frame #%d", simulation.getFrameNumber());

	ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
	if (ImGui::Button("Rewind x100"))
	{
		simulation.setRewind(100);
	}
	ImGui::SameLine();
	if (ImGui::Button("x10"))
	{
		simulation.setRewind(10);
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Step Back", ImGuiDir_Left))
	{
		simulation.setRewind(1);
	}
	ImGui::SameLine();
	ImGui::Text("Step");
	ImGui::SameLine();
	if (ImGui::ArrowButton("Step Forward", ImGuiDir_Right))
	{
		simulation.setNextSingleStep(true, false);
	}
	ImGui::PopItemFlag();
}

#endif
