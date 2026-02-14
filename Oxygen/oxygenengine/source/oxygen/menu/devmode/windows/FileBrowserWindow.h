/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/devmode/DevModeWindowBase.h"
#include "oxygen/menu/imgui/implementations/ImGuiFileBrowser.h"


class FileBrowserWindow : public DevModeWindowBase
{
public:
	FileBrowserWindow();

	virtual void buildContent() override;

private:
	ImGuiFileBrowser mFileBrowser;
};

#endif
