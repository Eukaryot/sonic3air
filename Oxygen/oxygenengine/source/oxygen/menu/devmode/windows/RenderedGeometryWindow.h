/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/devmode/DevModeWindowBase.h"


class RenderedGeometryWindow : public DevModeWindowBase
{
public:
	RenderedGeometryWindow();

	virtual void buildContent() override;
};

#endif
