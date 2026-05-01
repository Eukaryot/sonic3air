/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/sidebar/OxygenSideBar.h"
#include "oxygen/application/menu/OxygenMenu.h"
#include "oxygen/application/menu/SharedFonts.h"
#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/menu/loui/LouiLabel.h"


void OxygenSideBar::init()
{
	addChildWidget(mButtonLayout);

	mButtonLayout.setScrolling(true);
	mButtonLayout.setInnerPadding(10, 10, 0, 0);

	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;
	const Vec2i buttonSize(120, 16);

	mButtonLayout.createChildWidget<loui::Label>()
		.init("System Menu", font, buttonSize)
		.setOuterMargin(1, 5, 0, 0);

	mContinueButton
		.init("Continue", font, buttonSize)
		.setOuterMargin(1, 2, 0, 0);
	mButtonLayout.addChildWidget(mContinueButton);

	mSettingsButton
		.init("Settings", font, buttonSize)
		.setOuterMargin(1, 2, 0, 0);
	mButtonLayout.addChildWidget(mSettingsButton);

	mButtonLayout.makeFocusedChild();
}

void OxygenSideBar::setOpen(bool open)
{
	mShouldBeOpen = open;
}

void OxygenSideBar::update(loui::UpdateInfo& updateInfo)
{
	// Update open/close animation
	if (mShouldBeOpen)
	{
		if (mVisibility < 1.0f)
		{
			mVisibility = saturate(mVisibility + updateInfo.mDeltaSeconds / 0.15f);
		}
	}
	else
	{
		if (mVisibility == 0.0f)
		{
			setVisible(false);
			return;
		}

		mVisibility = saturate(mVisibility - updateInfo.mDeltaSeconds / 0.15f);
	}
	setVisible(true);

	const float animPos = 1.0f - (1.0f - mVisibility) * (1.0f - mVisibility);
	const Vec2i sideBarSize = getRelativeRect().getSize();
	const Recti rect(roundToInt(sideBarSize.x * (animPos - 1.0f)), 0, 160, getParentWidget()->getRelativeRect().height);
	setRelativeRect(rect);
	setInteractable(mVisibility == 1.0f);

	mButtonLayout.setRelativeRect(Recti(20, 0, rect.width - 40, rect.height));
	refreshLayout();

	Widget::update(updateInfo);

	// Check buttons
	if (mContinueButton.wasPressed())
	{
		OxygenMenu::instance().closeSideBar();
	}
	if (mSettingsButton.wasPressed())
	{
		OxygenMenu::instance().openSettingsMenu();
	}

	if (hasFocus() && updateInfo.mButtonB.justPressed())
	{
		OxygenMenu::instance().closeSideBar();
		updateInfo.mButtonB.consume();
	}
}

void OxygenSideBar::render(loui::RenderInfo& renderInfo)
{
	if (mBackground.getSize() != getRelativeRect().getSize())
	{
		Bitmap& bmp = mBackground.accessBitmap();
		bmp.create(getRelativeRect().getSize());
		const Vec3f hsl1 = Color::fromARGB32(0x0c65a1).getHSL();
		const Vec3f hsl2 = Color::fromARGB32(0x240048).getHSL();
		for (int y = 0; y < bmp.getHeight(); ++y)
		{
			for (int x = 0; x < bmp.getWidth(); ++x)
			{
				Vec3f hsl = Vec3f::interpolate(hsl1, hsl2, (float)(bmp.getWidth() - x + y) / (float)(bmp.getWidth() + bmp.getHeight()));
				Color color = Color::fromHSL(hsl);
				color.a = 1.0f;
				bmp.setPixel(x, y, color.getABGR32());
			}
		}
		mBackground.bitmapUpdated();
	}

	renderInfo.mDrawer.drawRect(getFinalRect(), mBackground);

	Widget::render(renderInfo);
}
