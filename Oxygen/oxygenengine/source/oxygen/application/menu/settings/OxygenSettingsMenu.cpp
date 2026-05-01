/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/settings/OxygenSettingsMenu.h"
#include "oxygen/application/menu/settings/SimpleSelection.h"
#include "oxygen/application/menu/settings/submenus/AudioMenu.h"
#include "oxygen/application/menu/settings/submenus/DisplayMenu.h"
#include "oxygen/application/menu/OxygenMenu.h"
#include "oxygen/application/menu/SharedFonts.h"
#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/menu/loui/LouiLabel.h"


void OxygenSettingsMenu::init()
{
	addChildWidget(mOverviewLayout);

	//setupSubMenu(SubMenu::Type::GENERAL,  "General");
	setupSubMenu(SubMenu::Type::DISPLAY,  "Display");
	setupSubMenu(SubMenu::Type::AUDIO,	  "Audio");
	//setupSubMenu(SubMenu::Type::CONTROLS, "Controls");

	{
		mOverviewLayout.makeFocusedChild();

		const Vec2i buttonSize(120, 17);
		loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;

		for (SubMenu& subMenu : mSubMenus)
		{
			subMenu.mOverviewButton
				.init(subMenu.mDisplayName, font, buttonSize)
				.setOuterMargin(1, 2, 0, 0);

			mOverviewLayout.addChildWidget(subMenu.mOverviewButton);
		}
	}
}

void OxygenSettingsMenu::update(loui::UpdateInfo& updateInfo)
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
	Recti rect = getParentWidget()->getRelativeRect();
	rect.x -= roundToInt(rect.width * (1.0f - animPos));
	setRelativeRect(rect);
	setInteractable(mVisibility == 1.0f);

	Widget::update(updateInfo);

	// Check overview buttons
	for (SubMenu& subMenu : mSubMenus)
	{
		if (subMenu.mOverviewButton.wasPressed())
		{
			// Open this sub-menu, and close all others
			for (SubMenu& other : mSubMenus)
			{
				other.mSubMenu->setVisible(&other == &subMenu);
			}
			subMenu.mSubMenu->makeFocusedChild();
		}
	}

	if (hasFocus() && updateInfo.mButtonB.justPressed())
	{
		if (mOverviewLayout.isFocusedChild())
		{
			OxygenMenu::instance().closeSettingsMenu();
		}
		else
		{
			mOverviewLayout.makeFocusedChild();
		}

		updateInfo.mButtonB.consume();
	}
}

void OxygenSettingsMenu::render(loui::RenderInfo& renderInfo)
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

void OxygenSettingsMenu::applyLayouting()
{
	// Calculate a frame with a maximum width
	Recti frameRect = getRelativeRect();
	if (frameRect.width > 600)
	{
		frameRect.x += (frameRect.width - 600) / 2;
		frameRect.width = 600;
	}

	mOverviewLayout.setRelativeRect(Recti(frameRect.x + 20, frameRect.y + 50, 120, frameRect.height - 100));

	for (SubMenu& subMenu : mSubMenus)
	{
		subMenu.mSubMenu->setRelativeRect(Recti(frameRect.x + frameRect.width / 2 - 80, frameRect.y, 200, frameRect.height));
	}
}

void OxygenSettingsMenu::setupSubMenu(SubMenu::Type type, std::string_view displayName)
{
	SubMenu& subMenu = mSubMenus[(int)type];
	subMenu.mType = type;
	subMenu.mDisplayName = displayName;

	switch (type)
	{
		case SubMenu::Type::DISPLAY:
			subMenu.mSubMenu = new DisplayMenu();
			break;

		case SubMenu::Type::AUDIO:
			subMenu.mSubMenu = new AudioMenu();
			break;

		default:
			subMenu.mSubMenu = new SettingsSubMenu();
			break;
	}

	addChildWidget(*subMenu.mSubMenu, true);

	subMenu.mSubMenu->setScrolling(true);
	subMenu.mSubMenu->setInnerPadding(10, 10, 0, 0);

	const Vec2i buttonSize(200, 16);
	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;

	subMenu.mSubMenu->createChildWidget<loui::Label>()
		.init(subMenu.mDisplayName, font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);

	subMenu.mSubMenu->init();

	subMenu.mSubMenu->setVisible(false);
}
