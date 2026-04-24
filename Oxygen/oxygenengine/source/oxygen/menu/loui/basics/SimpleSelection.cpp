/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/basics/SimpleSelection.h"
#include "oxygen/menu/loui/LouiFontWrapper.h"


namespace loui
{
	SimpleSelection& SimpleSelection::init(const std::string_view text, FontWrapper& font, Vec2i size)
	{
		const int x0 = 0;
		const int x1 = size.x * 40/100;
		const int x2 = size.x * 60/100;
		const int x3 = size.x * 80/100;
		const int x4 = size.x;

		mTitleLabel.init(text, font, Vec2i());
		mTitleLabel.setRelativeRect(Recti(x0, 0, x1 - x0, size.y));
		addChildWidget(mTitleLabel, false);

		mValueLabel.init("0", font, Vec2i());
		mValueLabel.setRelativeRect(Recti(x2, 0, x3 - x2, size.y));
		addChildWidget(mValueLabel, false);

		mButtonLeft.init("<", font, Vec2i());
		mButtonLeft.setRelativeRect(Recti(x1, 0, x2 - x1, size.y));
		addChildWidget(mButtonLeft, false);

		mButtonRight.init(">", font, Vec2i());
		mButtonRight.setRelativeRect(Recti(x3, 0, x4 - x3, size.y));
		addChildWidget(mButtonRight, false);

		return *this;
	}

	void SimpleSelection::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		mIsHovered = (!updateInfo.mMousePosConsumed && mFinalScreenRect.contains(updateInfo.mMousePosition));
	}

	void SimpleSelection::render(RenderInfo& renderInfo)
	{
		if (mIsHovered)
		{
			renderInfo.mDrawer.drawRect(mFinalScreenRect, Color(0.6f, 0.7f, 0.8f, 0.5f));
		}
		else if (mIsSelected)
		{
			renderInfo.mDrawer.drawRect(mFinalScreenRect, Color(0.8f, 0.8f, 0.5f, 0.5f));
		}

		Widget::render(renderInfo);
	}

	void SimpleSelection::applyLayouting()
	{
		const int x0 = 0;
		const int x1 = mRelativeRect.width * 40/100;
		const int x2 = mRelativeRect.width * 60/100;
		const int x3 = mRelativeRect.width * 80/100;
		const int x4 = mRelativeRect.width;

		mTitleLabel.setRelativeRect (Recti(x0, 0, x1 - x0, mRelativeRect.height));
		mValueLabel.setRelativeRect (Recti(x2, 0, x3 - x2, mRelativeRect.height));
		mButtonLeft.setRelativeRect (Recti(x1, 0, x2 - x1, mRelativeRect.height));
		mButtonRight.setRelativeRect(Recti(x3, 0, x4 - x3, mRelativeRect.height));

		Vec2i basePosition = mFinalScreenRect.getPos();
		basePosition.x += mInnerPadding.mLeft;
		basePosition.y += mInnerPadding.mTop;

		for (Widget* widget : getChildWidgets())
		{
			widget->setFinalScreenRect(widget->getRelativeRect() + basePosition);
		}
	}
}
