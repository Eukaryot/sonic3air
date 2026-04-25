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
		mRelativeRect.setSize(size);

		mTitleLabel.init(text, font, Vec2i());
		addChildWidget(mTitleLabel, false);

		mValueLabel.init("1", font, Vec2i());
		addChildWidget(mValueLabel, false);

		mButtonLeft.init("<", font, Vec2i(), true);
		addChildWidget(mButtonLeft, false);

		mButtonRight.init(">", font, Vec2i(), true);
		addChildWidget(mButtonRight, false);

		return *this;
	}

	void SimpleSelection::setValue(int newValue)
	{
		if (mValue == newValue)
			return;

		mValue = newValue;
		mValueLabel.setText(String(0, "%d", mValue));
	}

	void SimpleSelection::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		mIsHovered = (!updateInfo.mMousePosConsumed && mFinalScreenRect.contains(updateInfo.mMousePosition));

		if (mButtonLeft.wasPressed() || (isSelected() && updateInfo.mButtonLeft.justPressedOrRepeat()))
		{
			setValue(mValue - 1);
		}
		if (mButtonRight.wasPressed() || (isSelected() && updateInfo.mButtonRight.justPressedOrRepeat()))
		{
			setValue(mValue + 1);
		}
	}

	void SimpleSelection::render(RenderInfo& renderInfo)
	{
		if (mIsHovered)
		{
			renderInfo.mDrawer.drawRect(mFinalScreenRect, Color(0.6f, 0.7f, 0.8f, 0.1f));
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
		mButtonLeft.setRelativeRect (Recti(x1, 0, x2 - x1, mRelativeRect.height).withBorder(-3));
		mButtonRight.setRelativeRect(Recti(x3, 0, x4 - x3, mRelativeRect.height).withBorder(-3));

		Vec2i basePosition = mFinalScreenRect.getPos();
		basePosition.x += mInnerPadding.mLeft;
		basePosition.y += mInnerPadding.mTop;

		for (Widget* widget : getChildWidgets())
		{
			widget->setFinalScreenRect(widget->getRelativeRect() + basePosition);
		}
	}
}
