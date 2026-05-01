/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/menu/settings/submenus/SettingsSubMenu.h"


class AudioMenu : public SettingsSubMenu
{
public:
	virtual void init() override;

	virtual void update(loui::UpdateInfo& updateInfo) override;
};
