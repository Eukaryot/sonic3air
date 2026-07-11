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

		setCurrentOptionByIndex(0);

		return *this;
	}

	SimpleSelection& SimpleSelection::addOption(std::string_view displayText, int32 value)
	{
		Option& option = vectorAdd(mOptions);
		option.mDisplayText = displayText;
		option.mValue = value;

		return *this;
	}

	void SimpleSelection::setCurrentOptionByIndex(int index)
	{
		if (mOptionIndex == index)
			return;

		if (index >= 0 && index < (int)mOptions.size())
		{
			mOptionIndex = index;
			mValueLabel.setText(mOptions[mOptionIndex].mDisplayText);
		}
		else
		{
			mValueLabel.setText("?");
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

	void SimpleSelection::setCurrentOptionByValue(int value)
	{
		if (mOptions.empty())
			return;

		if (mOptionIndex < 0 || mOptionIndex >= (int)mOptions.size())
			mOptionIndex = 0;

		// Any change?
		if (mOptions[mOptionIndex].mValue == value)
			return;

		// Check for closest match
		int index = mOptionIndex;
		int difference = 0x7fffffff;
		for (size_t i = 0; i < mOptions.size(); ++i)
		{
			const int diff = std::abs(mOptions[i].mValue - value);
			if (diff < difference)
			{
				index = (int)i;
				if (diff == 0)
					break;		// This is an exact match already, no need to check any other option
				difference = diff;
			}
		}

		setCurrentOptionByIndex(index);
	}

	bool SimpleSelection::canGoLeft() const
	{
		return (mOptionIndex > 0);
	}

	bool SimpleSelection::canGoRight() const
	{
		return (mOptionIndex < (int)mOptions.size() - 1);
	}

	void SimpleSelection::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		mIsHovered = (isInteractable() && !updateInfo.mMousePosConsumed && mFinalRect.contains(updateInfo.mMousePosition));
		mWasChanged = false;

		if (canGoLeft() && (mButtonLeft.wasPressed() || (hasFocus() && updateInfo.mButtonLeft.justPressedOrRepeat())))
		{
			setCurrentOptionByIndex(mOptionIndex - 1);
			mWasChanged = true;
		}
		if (canGoRight() && (mButtonRight.wasPressed() || (hasFocus() && updateInfo.mButtonRight.justPressedOrRepeat())))
		{
			setCurrentOptionByIndex(mOptionIndex + 1);
			mWasChanged = true;
		}

		mButtonLeft.setVisible(canGoLeft());
		mButtonRight.setVisible(canGoRight());
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
		const int x1 = mRelativeRect.width * 4/8;
		const int x2 = mRelativeRect.width * 5/8;
		const int x3 = mRelativeRect.width * 7/8;
		const int x4 = mRelativeRect.width;

		mTitleLabel.setRelativeRect (Recti(x0, 0, x1 - x0, mRelativeRect.height));
		mValueLabel.setRelativeRect (Recti(x2, 0, x3 - x2, mRelativeRect.height));
		mButtonLeft.setRelativeRect (Recti(x1, 0, x2 - x1, mRelativeRect.height).withBorder(-3));
		mButtonRight.setRelativeRect(Recti(x3, 0, x4 - x3, mRelativeRect.height).withBorder(-3));
	}
}
