/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/LouiVerticalLayout.h"


namespace loui
{
	void VerticalLayout::applyLayouting()
	{
		// Apply vertical layout
		Vec2i cursor;

		for (Widget* widget : getChildWidgets())
		{
			cursor.y += widget->getOuterMargin().mTop;

			const Recti rect(cursor, widget->getRelativeRect().getSize());
			widget->setRelativeRect(rect);

			cursor.y += rect.height;
			cursor.y += widget->getOuterMargin().mBottom;

			widget->refreshLayout();
		}

		mInnerSize = cursor.y;
	}

	void VerticalLayout::scrollToRect(const Recti& localRect)
	{
		const int oldScrollPos = mScrollPosition;
		mScrollPosition = std::max(mScrollPosition, localRect.y + localRect.height - mRelativeRect.height);
		mScrollPosition = std::min(mScrollPosition, localRect.y);
		mScrollPosition = clamp(mScrollPosition, 0, mInnerSize - mRelativeRect.height);

		if (mScrollPosition != oldScrollPos)
		{
			refreshLayout();
		}
	}

	void VerticalLayout::setSelected(bool selected)
	{
		if (selected != mIsSelected)
		{
			Widget::setSelected(selected);

			if (selected && mSelectedChildIndex < 0 && !getChildWidgets().empty())
			{
				changeSelectedChildindex(+1);
			}
		}
	}

	void VerticalLayout::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		// Change selection
		if (isSelected())
		{
			if (updateInfo.mButtonUp.justPressed() || updateInfo.mButtonDown.justPressed())
			{
				const int direction = updateInfo.mButtonUp.justPressed() ? -1 : 1;
				changeSelectedChildindex(direction);
			}
		}

		// Scroll with mouse wheel
		if (mUseScrolling)
		{
			if (!updateInfo.mMouseWheelConsumed && updateInfo.mMouseWheel != 0)
			{
				mScrollPosition -= updateInfo.mMouseWheel * mWheelScrollSpeed;
				mScrollPosition = clamp(mScrollPosition, 0, std::max(mInnerSize - mRelativeRect.height, 0));
				refreshLayout();
			}
		}
	}

	void VerticalLayout::render(RenderInfo& renderInfo)
	{
		if (mUseScrolling)
		{
			renderInfo.mDrawer.pushScissor(mFinalScreenRect);
		}

		Widget::render(renderInfo);

		if (mUseScrolling)
		{
			renderInfo.mDrawer.popScissor();
		}
	}

	void VerticalLayout::changeSelectedChildindex(int direction)
	{
		const int newIndex = getNextInteractableChildIndex(mSelectedChildIndex, direction);
		if (newIndex != mSelectedChildIndex)
		{
			if (mSelectedChildIndex >= 0)
			{
				getChildWidget(mSelectedChildIndex)->setSelected(false);
			}

			mSelectedChildIndex = newIndex;

			if (mSelectedChildIndex >= 0)
			{
				Widget& newSelectedChild = *getChildWidget(mSelectedChildIndex);
				newSelectedChild.setSelected(true);

				Recti rect = newSelectedChild.getRelativeRect();
				rect.y -= mSelectionScrollAddSpace;
				rect.height += mSelectionScrollAddSpace * 2;
				scrollToRect(rect);
			}
		}
	}
}
