/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiManager.h"
#include <rmxmedia.h>


class ImGuiIntegration : public SingleInstance<ImGuiIntegration>
{
public:
	void setEnabled(bool enable);
	void startup();
	void shutdown();

	void processSdlEvent(const SDL_Event& ev);
	void startFrame();
	void endFrame();
	void onWindowRecreated(bool useOpenGL);

	void buildContents();

	bool isCapturingMouse();
	bool isCapturingKeyboard();

	void refreshImGuiStyle();

private:
	void saveIniSettings();

private:
	bool mEnabled = false;
	bool mRunning = false;
	bool mInsideFrame = false;
	bool mUsingOpenGL = false;

#if defined(SUPPORT_IMGUI)
	ImGuiManager mImGuiManager;

	ImFont* mDefaultFont = nullptr;
	std::wstring mIniFilePath;
#endif
};
