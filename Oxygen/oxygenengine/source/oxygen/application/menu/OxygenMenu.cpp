/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/OxygenMenu.h"
#include "oxygen/application/menu/SharedFonts.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/menu/loui/basics/SimpleButton.h"
#include "oxygen/menu/loui/basics/SimpleLabel.h"
#include "oxygen/menu/loui/basics/SimpleSelection.h"


OxygenMenu::OxygenMenu()
{
}

OxygenMenu::~OxygenMenu()
{
}

void OxygenMenu::initialize()
{
	mRootWidget.setRelativeRect(Recti(50, 10, 160, 155));
	mRootWidget.setScrolling(true);

	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;
	const Vec2i buttonSize(80, 16);
	
	mRootWidget.createChildWidget<loui::SimpleLabel>()
		.init("Title", font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);
	
	mRootWidget.createChildWidget<loui::SimpleButton>()
		.init("Button 1", font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);

	mRootWidget.createChildWidget<loui::SimpleButton>()
		.init("Button 2", font, buttonSize)
		.setOuterMargin(1, 5, 0, 0);

	for (int k = 0; k < 10; ++k)
	{
		mRootWidget.createChildWidget<loui::SimpleSelection>()
			.init(String(0, "Value %c", 'A' + k), font, buttonSize)
			.setOuterMargin(1, 1, 0, 0);
	}

	mRootWidget.setSelected(true);
}

void OxygenMenu::deinitialize()
{
}

void OxygenMenu::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);

	bool hasMousePos = false;
	Vec2i mousePos;
	Vec2f mousePosf;
	if (Application::instance().getGameView().translatePositionIntoGameViewport(mousePosf, Vec2f(FTX::mousePos())))
	{
		mousePos = Vec2i(mousePosf);
		hasMousePos = true;
	}
	const InputManager::ControllerScheme& controller = InputManager::instance().getController(0);

	mUpdateInfo.mDeltaSeconds = timeElapsed;
	mUpdateInfo.mMousePosition = mousePos;
	mUpdateInfo.mMouseWheel = FTX::mouseWheel();
	mUpdateInfo.mMousePosConsumed = !hasMousePos;
	mUpdateInfo.mMouseWheelConsumed = !hasMousePos;
	mUpdateInfo.mLeftMouseButton.updateState(FTX::mouseState(rmx::MouseButton::Left));
	mUpdateInfo.mButtonUp.updateState(controller.Up.isPressed());
	mUpdateInfo.mButtonDown.updateState(controller.Down.isPressed());
	mUpdateInfo.mButtonLeft.updateState(controller.Left.isPressed());
	mUpdateInfo.mButtonRight.updateState(controller.Right.isPressed());
	mUpdateInfo.mButtonA.updateState(controller.A.isPressed());
	mUpdateInfo.mButtonB.updateState(controller.B.isPressed());
	mUpdateInfo.mButtonX.updateState(controller.X.isPressed());
	mUpdateInfo.mButtonY.updateState(controller.Y.isPressed());

	// Use a copy of update info
	loui::UpdateInfo updateInfo = mUpdateInfo;
	mRootWidget.update(updateInfo);
}

void OxygenMenu::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();
	drawer.drawRect(getRect(), Color(0.1f, 0.2f, 0.4f, 0.9f));

	loui::RenderInfo renderInfo { drawer };
	mRootWidget.render(renderInfo);

	drawer.performRendering();
}
