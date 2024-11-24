/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/DevModeMainWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiIntegration.h"
#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/devmode/windows/GameSimWindow.h"
#include "oxygen/devmode/windows/MemoryHexViewWindow.h"
#include "oxygen/devmode/windows/PaletteViewWindow.h"
#include "oxygen/devmode/windows/SpriteBrowserWindow.h"
#include "oxygen/devmode/windows/WatchesWindow.h"


DevModeMainWindow::DevModeMainWindow() :
	DevModeWindowBase("Dev Mode (F1)")
{
	mIsWindowOpen = true;

	createWindow(mGameSimWindow);
	createWindow(mMemoryHexViewWindow);
	createWindow(mWatchesWindow);
	createWindow(mPaletteViewWindow);
	createWindow(mSpriteBrowserWindow);
}

DevModeMainWindow::~DevModeMainWindow()
{
	for (DevModeWindowBase* window : mAllWindows)
		delete window;
}

bool DevModeMainWindow::buildWindow()
{
	const bool result = DevModeWindowBase::buildWindow();
	for (DevModeWindowBase* window : mAllWindows)
	{
		window->buildWindow();
	}

	// ImGui demo window for testing
	if (mShowImGuiDemo)
		ImGui::ShowDemoWindow();

	return result;
}

void DevModeMainWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(150.0f, 200.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowCollapsed(true, ImGuiCond_FirstUseEver);

	for (DevModeWindowBase* window : mAllWindows)
	{
		ImGui::Checkbox(window->mTitle.c_str(), &window->mIsWindowOpen);
	}

#ifdef DEBUG
	ImGui::Spacing();
	ImGui::Checkbox("ImGui Demo", &mShowImGuiDemo);

	// Just for debugging
	//ImGui::Text("ImGui Capture:   %s %s", ImGui::GetIO().WantCaptureMouse ? "[M]" : "      ", ImGui::GetIO().WantCaptureKeyboard ? "[K]" : "");
#endif

	if (ImGui::CollapsingHeader("Settings", 0))
	{
		ImGuiHelpers::ScopedIndent si;

		Color& accentColor = ImGuiIntegration::getAccentColor();
		if (ImGui::ColorEdit3("Dev Mode UI Accent Color", accentColor.data, ImGuiColorEditFlags_NoInputs))
		{
			ImGuiIntegration::refreshImGuiStyle();
		}

		// TODO: Move this somewhere else
		if (ImGui::CollapsingHeader("Game View Window", 0))
		{
			Configuration& config = Configuration::instance();
			ImGui::SliderFloat("Size", &config.mDevMode.mGameViewScale, 0.2f, 1.0f);

			ImGui::SliderFloat("Alignment X", &config.mDevMode.mGameViewAlignment.x, -1.0f, 1.0f);
			ImGui::SameLine();
			if (ImGui::Button("Center##x"))
				config.mDevMode.mGameViewAlignment.x = 0.0f;

			ImGui::SliderFloat("Alignment Y", &config.mDevMode.mGameViewAlignment.y, -1.0f, 1.0f);
			ImGui::SameLine();
			if (ImGui::Button("Center##y"))
				config.mDevMode.mGameViewAlignment.y = 0.0f;
		}
	}
}

void DevModeMainWindow::openWatchesWindow()
{
	mWatchesWindow->mIsWindowOpen = true;
}

#endif
