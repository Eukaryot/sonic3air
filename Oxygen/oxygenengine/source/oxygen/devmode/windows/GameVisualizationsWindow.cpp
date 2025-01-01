/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/GameVisualizationsWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/Application.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/LogDisplay.h"


GameVisualizationsWindow::GameVisualizationsWindow() :
	DevModeWindowBase("Visualizations", Category::GAME_CONTROLS, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void GameVisualizationsWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(250.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(400.0f, 150.0f), ImGuiCond_FirstUseEver);

	GameView& gameView = Application::instance().getGameView();
	VideoOut& videoOut = VideoOut::instance();

	// Layer rendering
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Render Layers:");

		ImGuiHelpers::ScopedIndent si;

		const char* NAMES[] = { "Plane B", "Plane A", "VDP Sprites", "Custom Sprites" };
		for (int k = 0; k < 8; ++k)
		{
			ImGui::PushID(k);
			if (k % 4 == 0)
				ImGui::Text(k == 0 ? "Normal:  " : "Prio:        ");
			ImGui::SameLine();
			ImGui::Checkbox(NAMES[k % 4], &videoOut.getRenderParts().mLayerRendering[k]);
			ImGui::PopID();
		}
	}

	// Ground overlay
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Plane / VRAM Display:");

		const char* NAMES[] = { "None##1", "Plane B", "Plane A", "Plane W", "VRAM Patterns" };
		for (int k = 0; k < 5; ++k)
		{
			if (k > 0)
				ImGui::SameLine();
			if (ImGui::RadioButton(NAMES[k], gameView.accessDebugVisualizations().mDebugOutput == k - 1))
			{
				gameView.accessDebugVisualizations().mDebugOutput = k - 1;
			}
		}
	}

	// Ground overlay
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Ground Overlay:");

		if (ImGui::RadioButton("None##2", !gameView.accessDebugVisualizations().mEnabled))
		{
			gameView.accessDebugVisualizations().mEnabled = false;
		}

		ImGui::SameLine();
		if (ImGui::RadioButton("Collision", gameView.accessDebugVisualizations().mEnabled && gameView.accessDebugVisualizations().mMode == 0))
		{
			gameView.accessDebugVisualizations().mEnabled = true;
			gameView.accessDebugVisualizations().mMode = 0;
		}

		ImGui::SameLine();
		if (ImGui::RadioButton("Angles", gameView.accessDebugVisualizations().mEnabled && gameView.accessDebugVisualizations().mMode == 1))
		{
			gameView.accessDebugVisualizations().mEnabled = true;
			gameView.accessDebugVisualizations().mMode = 1;
		}
	}
}

#endif
