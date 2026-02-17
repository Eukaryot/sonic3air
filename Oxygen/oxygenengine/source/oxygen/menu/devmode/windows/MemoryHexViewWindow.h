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


class MemoryHexViewWindow : public DevModeWindowBase
{
public:
	MemoryHexViewWindow();

	virtual void buildContent() override;

private:
	uint32 mStartAddress = 0xffffb000;
	int mNumRows = 4;
	uint32 mClickedAddress = 0xffffffff;
};

#endif
