/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/GameVisualizationsWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/Application.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/menu/imgui/ImGuiHelpers.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/LogDisplay.h"


GameVisualizationsWindow::GameVisualizationsWindow() :
	DevModeWindowBase("Visualizations", Category::GRAPHICS, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void GameVisualizationsWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(600.0f, 5.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(400.0f, 150.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();
	GameView& gameView = Application::instance().getGameView();
	VideoOut& videoOut = VideoOut::instance();

	// Game view size
	{
		if (!mUnappliedScreenX)
			mScreenSizeInput.x = videoOut.getScreenSize().x;
		if (!mUnappliedScreenY)
			mScreenSizeInput.y = videoOut.getScreenSize().y;

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Game Screen Size:  ");
		ImGui::SameLine();

		ImGui::PushItemWidth(40 * uiScale);
		if (ImGui::InputInt("##x", &mScreenSizeInput.x, 0, 0, 0))
		{
			mUnappliedScreenX = true;
		}
		ImGui::SameLine();
		ImGui::Text("x");
		ImGui::SameLine();
		if (ImGui::InputInt("##y", &mScreenSizeInput.y, 0, 0, 0))
		{
			mUnappliedScreenY = true;
		}
		ImGui::SameLine();
		ImGui::Text("pixels ");
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("Apply"))
		{
			mScreenSizeInput.x = clamp(mScreenSizeInput.x, 128, 1024);
			mScreenSizeInput.y = clamp(mScreenSizeInput.y, 128, 1024);
			videoOut.setScreenSize(mScreenSizeInput);
			mUnappliedScreenX = false;
			mUnappliedScreenY = false;
		}
	}

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
