/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/sidebar/OxygenSideBar.h"
#include "oxygen/application/menu/SharedFonts.h"
#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/menu/loui/LouiLabel.h"
#include "oxygen/menu/loui/basics/SimpleSelection.h"


void OxygenSideBar::init()
{
	addChildWidget(mButtonLayout, false);

	mButtonLayout.setScrolling(true);

	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;
	const Vec2i buttonSize(120, 16);

	mButtonLayout.createChildWidget<loui::Label>()
		.init("Title", font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);

	mButtonLayout.createChildWidget<loui::Button>()
		.init("Button 1", font, buttonSize)
		.setOuterMargin(1, 1, 0, 0);

	mButtonLayout.createChildWidget<loui::Button>()
		.init("Button 2", font, buttonSize)
		.setOuterMargin(1, 5, 0, 0);

	for (int k = 0; k < 10; ++k)
	{
		mButtonLayout.createChildWidget<loui::SimpleSelection>()
			.init(String(0, "Value %c", 'A' + k), font, buttonSize)
			.setOuterMargin(1, 1, 0, 0);
	}

	mButtonLayout.setSelected(true);
}

void OxygenSideBar::update(loui::UpdateInfo& updateInfo)
{
	const Recti rect(getRelativeRect().x, 0, 160, FTX::screenHeight());
	setRelativeRect(rect);

	mButtonLayout.setRelativeRect(Recti(20, 0, rect.width - 40, rect.height));
	refreshLayout();

	Widget::update(updateInfo);
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

	renderInfo.mDrawer.drawRect(getFinalScreenRect(), mBackground);

	Widget::render(renderInfo);
}
