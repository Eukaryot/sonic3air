/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiVerticalLayout.h"
#include "oxygen/drawing/DrawerTexture.h"


class OxygenSideBar : public loui::Widget
{
public:
	void init();

	virtual void update(loui::UpdateInfo& updateInfo) override;
	virtual void render(loui::RenderInfo& renderInfo) override;

private:
	loui::VerticalLayout mButtonLayout;
	DrawerTexture mBackground;
};
