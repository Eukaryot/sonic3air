/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/settings/submenus/DisplayMenu.h"
#include "oxygen/application/menu/settings/SimpleSelection.h"
#include "oxygen/application/menu/SharedFonts.h"


void DisplayMenu::init()
{
	const Vec2i buttonSize(200, 16);
	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;

	mRendererSelection
		.addOption("Fail-Safe / Pure Software",	(int)Configuration::RenderMethod::SOFTWARE)
		.addOption("OpenGL Software",			(int)Configuration::RenderMethod::OPENGL_SOFT)
		.addOption("OpenGL Hardware",			(int)Configuration::RenderMethod::OPENGL_FULL)
		.init("Renderer", font, buttonSize);
	addChildWidget(mRendererSelection);

	createChildWidget<loui::SimpleSelection>()
		.init("Frame Sync", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Upscaling", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Backdrop", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Screen Filter", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Scanlines", font, buttonSize);
}

void DisplayMenu::update(loui::UpdateInfo& updateInfo)
{
	loui::VerticalLayout::update(updateInfo);

	if (mRendererSelection.wasChanged())
	{
		const uint32 value = mRendererSelection.getCurrentOptionValue();
		EngineMain::instance().switchToRenderMethod((Configuration::RenderMethod)value);
	}
}
