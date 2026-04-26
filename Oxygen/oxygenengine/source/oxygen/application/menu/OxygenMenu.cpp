/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/OxygenMenu.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"


void OxygenMenu::setVisible(bool visible)
{
	mIsVisible = visible;
}

void OxygenMenu::initialize()
{
	mSideBar.init();
	mRootWidget.addChildWidget(mSideBar, false);
}

void OxygenMenu::deinitialize()
{
}

void OxygenMenu::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);

	// Only in debug builds during development
#if DEBUG
	if (ev.key == '<' && ev.state)
	{
		// Toggle visibility
		setVisible(!mIsVisible);
	}
#endif
}

void OxygenMenu::update(float deltaSeconds)
{
	if (mIsVisible)
	{
		if (mVisibility < 1.0f)
		{
			mVisibility = saturate(mVisibility + deltaSeconds / 0.15f);
		}
	}
	else
	{
		if (mVisibility == 0.0f)
			return;

		mVisibility = saturate(mVisibility - deltaSeconds / 0.15f);
	}

	const float animPos = 1.0f - (1.0f - mVisibility) * (1.0f - mVisibility);
	const Vec2i sideBarSize = mSideBar.getRelativeRect().getSize();
	mSideBar.setRelativeRect(Recti(Vec2i(roundToInt(sideBarSize.x * (animPos - 1.0f)), 0), sideBarSize));
	mSideBar.setInteractable(mVisibility == 1.0f);

	const Vec2i resolution = (FTX::screenSize() + Vec2i(mMenuScale - 1)) / mMenuScale;	// Round up, to ensure the screen is fully covered
	mCoveredScreenRect = Recti::getIntersection(mSideBar.getRelativeRect(), Recti(Vec2i(), resolution));

	if (!FTX::System->wasEventConsumed())
	{
		GuiBase::update(deltaSeconds);

		// Update input
		{
			InputManager& inputManager = InputManager::instance();
			const Vec2i mousePos = Vec2i(mOxygenMenuViewport.getInnerPositionFromScreen(Vec2f(FTX::mousePos())));
			const bool hasMousePos = mOxygenMenuViewport.isValidInnerPosition(mousePos);
			const InputManager::ControllerScheme& controller = inputManager.getController(0);

			mUpdateInfo.mDeltaSeconds = deltaSeconds;
			mUpdateInfo.mLastInputWasMouse = (inputManager.getLastInputType() == InputManager::InputType::TOUCH);

			mUpdateInfo.mMousePosition = mousePos;
			mUpdateInfo.mMouseWheel = FTX::mouseWheel();
			mUpdateInfo.mMousePosConsumed = !hasMousePos;
			mUpdateInfo.mMouseWheelConsumed = !hasMousePos;
			mUpdateInfo.mLeftMouseButton.updateState(FTX::mouseState(rmx::MouseButton::Left), deltaSeconds);

			mUpdateInfo.mButtonUp.updateState(controller.Up.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonDown.updateState(controller.Down.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonLeft.updateState(controller.Left.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonRight.updateState(controller.Right.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonA.updateState(controller.A.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonB.updateState(controller.B.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonX.updateState(controller.X.isPressed(), deltaSeconds);
			mUpdateInfo.mButtonY.updateState(controller.Y.isPressed(), deltaSeconds);
		}

		// Use a copy of update info
		loui::UpdateInfo updateInfo = mUpdateInfo;
		mRootWidget.update(updateInfo);
	}
}

void OxygenMenu::render()
{
	if (mVisibility <= 0.0f)
		return;

	GuiBase::render();

	if (mCoveredScreenRect.width <= 0)
		return;

	Drawer& drawer = EngineMain::instance().getDrawer();

	// Render the (pixelated) menu
	{
		mMenuScale = getMenuScale();
		Vec2i resolution = (FTX::screenSize() + Vec2i(mMenuScale - 1)) / mMenuScale;	// Round up, to ensure the screen is fully covered

		resolution.x = mCoveredScreenRect.width;
		const Recti menuScreenRect(Vec2i(), resolution);

		mOxygenMenuTexture.setupAsRenderTarget(resolution);
		mOxygenMenuViewport.setResolution(resolution);
		mOxygenMenuViewport.setRectOnScreen(Recti(Vec2i(), resolution * mMenuScale));

		drawer.setRenderTarget(mOxygenMenuTexture, menuScreenRect);

		setRect(menuScreenRect);

		loui::RenderInfo renderInfo { drawer };
		mRootWidget.render(renderInfo);

		drawer.performRendering();
	}

	// Draw to screen
	drawer.setWindowRenderTarget(FTX::screenRect());
	drawer.setBlendMode(BlendMode::OPAQUE);
	drawer.drawUpscaledRect(mOxygenMenuViewport.getRectOnScreen(), mOxygenMenuTexture);
	drawer.performRendering();
}

int OxygenMenu::getMenuScale() const
{
	const Vec2f desiredSize(640, 360);
	const Vec2f scales = Vec2f(FTX::screenSize()) / desiredSize;
	return std::max(1, roundToInt(std::min(scales.x, scales.y)));
}
