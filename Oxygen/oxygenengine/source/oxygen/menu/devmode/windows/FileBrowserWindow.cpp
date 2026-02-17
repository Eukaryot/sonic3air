/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/FileBrowserWindow.h"

#if defined(SUPPORT_IMGUI)


FileBrowserWindow::FileBrowserWindow() :
	DevModeWindowBase("File Browser", Category::MISC, 0)
{
	mFileBrowser.mStartInModsFolder = false;
}

void FileBrowserWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(100.0f, 200.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(700.0f, 500.0f), ImGuiCond_FirstUseEver);

	mFileBrowser.buildWindowContent();
}

#endif
