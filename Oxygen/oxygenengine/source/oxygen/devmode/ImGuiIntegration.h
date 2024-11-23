/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/devmode/ImGuiDefinitions.h"
#include <rmxmedia.h>


class ImGuiIntegration
{
public:
	static void setEnabled(bool enable);
	static void startup();
	static void shutdown();

	static void processSdlEvent(const SDL_Event& ev);
	static void startFrame();
	static void showDebugWindow();
	static void endFrame();

	static void onWindowRecreated(bool useOpenGL);

	static bool isCapturingMouse();

	static inline Color& getAccentColor()  { return mAccentColor; }

	static void refreshImGuiStyle();

	static void toggleMainWindow();

private:
	static inline Color mAccentColor;
	static inline bool mEnabled = false;
	static inline bool mRunning = false;
};
