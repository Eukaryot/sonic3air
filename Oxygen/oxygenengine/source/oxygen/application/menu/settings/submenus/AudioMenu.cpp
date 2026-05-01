/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/settings/submenus/AudioMenu.h"
#include "oxygen/application/menu/settings/SimpleSelection.h"
#include "oxygen/application/menu/SharedFonts.h"


void AudioMenu::init()
{
	const Vec2i buttonSize(200, 16);
	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;

	createChildWidget<loui::SimpleSelection>()
		.init("Main Volume", font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);

	createChildWidget<loui::SimpleSelection>()
		.init("Music", font, buttonSize)
		.setOuterMargin(5, 1, 0, 0);

	createChildWidget<loui::SimpleSelection>()
		.init("Sound Effects", font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);
}

void AudioMenu::update(loui::UpdateInfo& updateInfo)
{
	loui::VerticalLayout::update(updateInfo);
}
