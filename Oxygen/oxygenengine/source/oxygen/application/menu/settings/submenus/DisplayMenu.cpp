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
#include "oxygen/application/Application.h"


void DisplayMenu::init()
{
	const Vec2i buttonSize(300, 16);
	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;

	mRendererSelection
		.addOption("Fail-Safe / Pure Software",	Configuration::RenderMethod::SOFTWARE)
		.addOption("OpenGL Software",			Configuration::RenderMethod::OPENGL_SOFT)
		.addOption("OpenGL Hardware",			Configuration::RenderMethod::OPENGL_FULL)
		.init("Renderer", font, buttonSize);
	addChildWidget(mRendererSelection);

	mFrameSyncSelection
		.addOption("V-Sync Off",		Configuration::FrameSyncType::VSYNC_OFF)
		.addOption("V-Sync On",			Configuration::FrameSyncType::VSYNC_ON)
		.addOption("V-Sync + FPS Cap",	Configuration::FrameSyncType::VSYNC_FRAMECAP)
		.init("Frame Sync", font, buttonSize);
	addChildWidget(mFrameSyncSelection);

/*
	createChildWidget<loui::SimpleSelection>()
		.init("Upscaling", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Backdrop", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Screen Filter", font, buttonSize);

	createChildWidget<loui::SimpleSelection>()
		.init("Scanlines", font, buttonSize);
*/

	const Configuration& config = Configuration::instance();

	mRendererSelection.setCurrentOptionByValue(config.mRenderMethod);
	mFrameSyncSelection.setCurrentOptionByValue(config.mFrameSync);
}

void DisplayMenu::update(loui::UpdateInfo& updateInfo)
{
	loui::VerticalLayout::update(updateInfo);

	Configuration& config = Configuration::instance();

	if (mRendererSelection.wasChanged())
	{
		Application::instance().setPendingRenderMethod(mRendererSelection.getCurrentOptionValueTyped<Configuration::RenderMethod>());
	}

	if (mFrameSyncSelection.wasChanged())
	{
		config.mFrameSync = (Configuration::FrameSyncType)mFrameSyncSelection.getCurrentOptionValue();
	}
}
