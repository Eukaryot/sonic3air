/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/OxygenMenu.h"
#include "oxygen/application/menu/sidebar/OxygenSideBar.h"
#include "oxygen/application/menu/settings/OxygenSettingsMenu.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/menu/SharedFonts.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"


void OxygenMenu::initialize()
{
	if (nullptr == mSideBar)
	{
		mSideBar = &mRootWidget.createChildWidget<OxygenSideBar>();
		mSideBar->init();
	}

	if (nullptr == mSettingsMenu)
	{
		mSettingsMenu = &mRootWidget.createChildWidget<OxygenSettingsMenu>();
		mSettingsMenu->init();
	}

	// All widgets are supposed to be invisible at first
	for (loui::Widget* widget : mRootWidget.getChildWidgets())
	{
		widget->setVisible(false);
	}
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
		// Toggle side bar
		if (isSideBarOpen())
		{
			closeSideBar();
		}
		else
		{
			openSideBar();
		}
	}
#endif
}

void OxygenMenu::update(float deltaSeconds)
{
	// Update resolution
	refreshMenuResolution();
	mRootWidget.setRelativeRect(Recti(Vec2i(), mMenuResolution));

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

		switch (mTriggeredAction)
		{
			case TriggeredAction::OPEN_SIDE_BAR:
			{
				mSideBar->setOpen(true);
				mSettingsMenu->setOpen(false);

				mSideBar->grantFocus();
				break;
			}

			case TriggeredAction::CLOSE_SIDE_BAR:
			{
				mSideBar->setOpen(false);
				mSettingsMenu->setOpen(false);
				break;
			}

			case TriggeredAction::OPEN_SETTINGS:
			{
				mSideBar->setOpen(false);
				mSettingsMenu->setOpen(true);

				mSettingsMenu->grantFocus();
				break;
			}

			case TriggeredAction::CLOSE_SETTINGS:
			{
				mSideBar->setOpen(true);
				mSettingsMenu->setOpen(false);

				mSideBar->grantFocus();
				break;
			}
		}
		mTriggeredAction = TriggeredAction::NONE;
	}
}

void OxygenMenu::render()
{
	GuiBase::render();

	mCoveredScreenRect = Recti(0, 0, 0, mRootWidget.getRelativeRect().height);
	for (loui::Widget* widget : mRootWidget.getChildWidgets())
	{
		if (widget->isVisible())
		{
			mCoveredScreenRect.width = std::max(mCoveredScreenRect.width, widget->getFinalRect().getEndPos().x);
		}
	}
	mCoveredScreenRect = Recti::getIntersection(mCoveredScreenRect, Recti(Vec2i(), mRootWidget.getRelativeRect().getSize()));

	if (mCoveredScreenRect.width <= 0)
		return;

	Drawer& drawer = EngineMain::instance().getDrawer();

	// Render the (pixelated) menu
	{
		Vec2i resolution = mMenuResolution;
		Vec2i upscaledResolution = mUpscaledResolution;
		if (mCoveredScreenRect.width < resolution.x)
		{
			upscaledResolution.x = upscaledResolution.x * mCoveredScreenRect.width / resolution.x;
			resolution.x = mCoveredScreenRect.width;
		}
		const Recti menuScreenRect(Vec2i(), resolution);

		mOxygenMenuTexture.setupAsRenderTarget(resolution);
		mOxygenMenuViewport.setResolution(resolution);
		mOxygenMenuViewport.setRectOnScreen(Recti(Vec2i(), upscaledResolution));

		drawer.setRenderTarget(mOxygenMenuTexture, menuScreenRect);

		setRect(menuScreenRect);

		loui::RenderInfo renderInfo { drawer };
		renderInfo.mShowFocus = !mUpdateInfo.mLastInputWasMouse;
		mRootWidget.render(renderInfo);

	#if DEBUG
		//drawer.printText(SharedFonts::oxyFontSmall, Vec2i(8, resolution.y - 6), String(0, "%dx%d", resolution.x, resolution.y), 7);
	#endif

		drawer.performRendering();
	}

	// Draw to screen
	drawer.setWindowRenderTarget(FTX::screenRect());
	drawer.setBlendMode(BlendMode::OPAQUE);
	drawer.drawUpscaledRect(mOxygenMenuViewport.getRectOnScreen(), mOxygenMenuTexture);
	drawer.performRendering();
}

bool OxygenMenu::isSideBarOpen() const
{
	return mSideBar->shouldBeOpen();
}

void OxygenMenu::openSideBar()
{
	mTriggeredAction = TriggeredAction::OPEN_SIDE_BAR;
}

void OxygenMenu::closeSideBar()
{
	mTriggeredAction = TriggeredAction::CLOSE_SIDE_BAR;
}

void OxygenMenu::openSettingsMenu()
{
	mTriggeredAction = TriggeredAction::OPEN_SETTINGS;
}

void OxygenMenu::closeSettingsMenu()
{
	mTriggeredAction = TriggeredAction::CLOSE_SETTINGS;
}

void OxygenMenu::refreshMenuResolution()
{
	const Vec2i minimumSize(500, 300);
	const Vec2i desiredSize(560, 360);

	const Vec2i fullSize = FTX::screenSize();
	const Vec2f scales = Vec2f(fullSize) / Vec2f(desiredSize);
	int scale = std::max(1, roundToInt(std::min(scales.x, scales.y)));

	while (true)
	{
		mMenuResolution = (fullSize + Vec2i(scale - 1)) / scale;	// Round up to ensure the screen is fully covered
		if (mMenuResolution.x >= minimumSize.x && mMenuResolution.y >= minimumSize.y)
		{
			mUpscaledResolution = mMenuResolution * scale;
			return;
		}

		--scale;
		if (scale == 0)
		{
			mMenuResolution.x = std::max(mMenuResolution.x, minimumSize.x);
			mMenuResolution.y = std::max(mMenuResolution.y, minimumSize.y);
			mUpscaledResolution = fullSize;
			return;
		}
	}
}
