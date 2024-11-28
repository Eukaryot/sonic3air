/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/devmode/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

class DevModeMainWindow;


class DevModeWindowBase
{
friend class DevModeMainWindow;

public:
	DevModeWindowBase(std::string_view title, ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize);
	virtual ~DevModeWindowBase() {}

	virtual bool buildWindow();
	virtual void buildContent() = 0;

public:
	std::string mTitle;
	bool mIsWindowOpen = false;
	ImGuiWindowFlags mImGuiWindowFlags = 0;

protected:
	DevModeMainWindow* mDevModeMainWindow = nullptr;
};

#endif
