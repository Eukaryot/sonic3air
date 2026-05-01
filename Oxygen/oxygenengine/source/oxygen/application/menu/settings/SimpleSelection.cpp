/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/settings/SimpleSelection.h"
#include "oxygen/menu/loui/LouiFontWrapper.h"


namespace loui
{
	SimpleSelection& SimpleSelection::init(const std::string_view text, FontWrapper& font, Vec2i size)
	{
		mRelativeRect.setSize(size);
		setOuterMargin(1, 1, 0, 0);

		mTitleLabel.init(text, font, Vec2i(), 4);
		addChildWidget(mTitleLabel);

		mValueLabel.init("", font, Vec2i());
		addChildWidget(mValueLabel);

		mButtonLeft.init("<", font, Vec2i(), true);
		addChildWidget(mButtonLeft);

		mButtonRight.init(">", font, Vec2i(), true);
		addChildWidget(mButtonRight);

		setValue(1);

		return *this;
	}

	SimpleSelection& SimpleSelection::addOption(std::string_view displayText, int32 value)
	{
		Option& option = vectorAdd(mOptions);
		option.mDisplayText = displayText;
		option.mValue = value;

		return *this;
	}

	void SimpleSelection::setValue(int newValue)
	{
		if (mOptionIndex == newValue)
			return;

		mOptionIndex = newValue;
		if (mOptionIndex >= 0 && mOptionIndex < (int)mOptions.size())
		{
			mValueLabel.setText(mOptions[mOptionIndex].mDisplayText);
		}
		else
		{
			mValueLabel.setText(String(0, "%d", mOptionIndex));
		}
	}

	int32 SimpleSelection::getCurrentOptionValue() const
	{
		if (mOptionIndex >= 0 && mOptionIndex < (int)mOptions.size())
		{
			return mOptions[mOptionIndex].mValue;
		}
		else
		{
			return 0;
		}
	}

	void SimpleSelection::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		mIsHovered = (isInteractable() && !updateInfo.mMousePosConsumed && mFinalRect.contains(updateInfo.mMousePosition));
		mWasChanged = false;

		if (mButtonLeft.wasPressed() || (hasFocus() && updateInfo.mButtonLeft.justPressedOrRepeat()))
		{
			setValue(mOptionIndex - 1);
			mWasChanged = true;
		}
		if (mButtonRight.wasPressed() || (hasFocus() && updateInfo.mButtonRight.justPressedOrRepeat()))
		{
			setValue(mOptionIndex + 1);
			mWasChanged = true;
		}
	}

	void SimpleSelection::render(RenderInfo& renderInfo)
	{
		if (hasFocus() && renderInfo.mShowFocus)
		{
			renderInfo.mDrawer.drawRect(mFinalRect, Color(0.8f, 0.8f, 0.5f, 0.5f));
		}

		if (mIsHovered)
		{
			renderInfo.mDrawer.drawRect(mFinalRect, Color(0.6f, 0.7f, 0.8f, 0.1f));
		}

		Widget::render(renderInfo);
	}

	void SimpleSelection::applyLayouting()
	{
		const int x0 = 5;
		const int x1 = 100;
		const int x2 = 125;
		const int x3 = 175;
		const int x4 = 200;

		mTitleLabel.setRelativeRect (Recti(x0, 0, x1 - x0, mRelativeRect.height));
		mValueLabel.setRelativeRect (Recti(x2, 0, x3 - x2, mRelativeRect.height));
		mButtonLeft.setRelativeRect (Recti(x1, 0, x2 - x1, mRelativeRect.height).withBorder(-3));
		mButtonRight.setRelativeRect(Recti(x3, 0, x4 - x3, mRelativeRect.height).withBorder(-3));
	}
}
