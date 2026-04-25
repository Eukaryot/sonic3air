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

	void VerticalLayout::scrollToRect(const Recti& localRect, bool animated)
	{
		const bool scrolling = mScrollingController.scrollToMakeVisible(localRect.y, localRect.height, animated);
		if (scrolling && !animated)
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
		if (mDraggedByMouse)
		{
			if (mUseScrolling && updateInfo.mLeftMouseButton.isPressed())
			{
				updateInfo.mMousePosConsumed = true;
				updateInfo.mLeftMouseButton.consume();

				if (mScrollingController.scrollRelative(mLastMouseY - updateInfo.mMousePosition.y))
				{
					refreshLayout();
				}
			}
			else
			{
				mDraggedByMouse = false;
			}
		}

		Widget::update(updateInfo);

		// Change selection
		if (isSelected())
		{
			if (updateInfo.mButtonUp.justPressedOrRepeat() || updateInfo.mButtonDown.justPressedOrRepeat())
			{
				const int direction = updateInfo.mButtonUp.justPressedOrRepeat() ? -1 : 1;
				changeSelectedChildindex(direction);
			}
		}

		if (mUseScrolling)
		{
			mScrollingController.setScrollAreaSize(mInnerSize, mRelativeRect.height);

			if (mScrollingController.update(updateInfo.mDeltaSeconds))
			{
				refreshLayout();
			}

			// Scroll with mouse wheel
			if (!updateInfo.mMouseWheelConsumed && updateInfo.mMouseWheel != 0)
			{
				if (mScrollingController.scrollRelative(-updateInfo.mMouseWheel * mWheelScrollSpeed))
				{
					refreshLayout();
				}
			}

			if (updateInfo.mLeftMouseButton.justPressed())
			{
				mDraggedByMouse = true;
			}
		}

		mLastMouseY = updateInfo.mMousePosition.y;
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
