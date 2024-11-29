/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/SettingsWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/devmode/ImGuiIntegration.h"


SettingsWindow::SettingsWindow() :
	DevModeWindowBase("Settings")
{
}

void SettingsWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(350.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	if (ImGui::CollapsingHeader("Game View Window", 0))
	{
		ImGuiHelpers::ScopedIndent si;

		Configuration& config = Configuration::instance();
		ImGui::SliderFloat("Size", &config.mDevMode.mGameViewScale, 0.2f, 1.0f, "%.2f");

		ImGui::SliderFloat("Alignment X", &config.mDevMode.mGameViewAlignment.x, -1.0f, 1.0f, "%.3f");
		ImGui::SameLine();
		if (ImGui::Button("Center##x"))
			config.mDevMode.mGameViewAlignment.x = 0.0f;

		ImGui::SliderFloat("Alignment Y", &config.mDevMode.mGameViewAlignment.y, -1.0f, 1.0f, "%.3f");
		ImGui::SameLine();
		if (ImGui::Button("Center##y"))
			config.mDevMode.mGameViewAlignment.y = 0.0f;
	}

	Color& accentColor = ImGuiIntegration::getAccentColor();
	if (ImGui::ColorEdit3("Dev Mode UI Accent Color", accentColor.data, ImGuiColorEditFlags_NoInputs))
	{
		ImGuiIntegration::refreshImGuiStyle();
	}

	ImGui::DragFloat("UI Scale", &ImGui::GetIO().FontGlobalScale, 0.003f, 0.5f, 4.0f, "%.1f");
}

#endif
