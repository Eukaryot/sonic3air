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


class CallFramesWindow : public DevModeWindowBase
{
public:
	CallFramesWindow();

	virtual void buildContent() override;

private:
	bool mShowAllHitFunctions  = false;
	bool mVisualizationSorting = false;
	bool mShowProfilingSamples = false;

	std::unordered_map<uint64, bool> mOpenState;
	std::vector<uint32> mSortedUnknownAddressed;
};

#endif
