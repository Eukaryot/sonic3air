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
	void VerticalLayout::scrollToRect(const Recti& localRect, bool animated)
	{
		const bool scrolling = mScrollingController.scrollToMakeVisible(localRect.y, localRect.height, animated);
		if (scrolling && !animated)
		{
			refreshChildPositions();
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
					refreshChildPositions();
				}
			}
			else
			{
				mDraggedByMouse = false;
			}
		}

		Widget::update(updateInfo);

		// Change focused child
		if (hasFocus())
		{
			if (updateInfo.mButtonUp.justPressedOrRepeat() || updateInfo.mButtonDown.justPressedOrRepeat())
			{
				const int direction = updateInfo.mButtonUp.justPressedOrRepeat() ? -1 : 1;
				changeFocusedChildindex(direction);
			}
		}

		if (mUseScrolling)
		{
			mScrollingController.setScrollAreaSize(mInnerSize, mRelativeRect.height);

			if (mScrollingController.update(updateInfo.mDeltaSeconds))
			{
				refreshChildPositions();
			}

			// Scroll with mouse wheel
			if (!updateInfo.mMouseWheelConsumed && updateInfo.mMouseWheel != 0)
			{
				if (mScrollingController.scrollRelative(-updateInfo.mMouseWheel * mWheelScrollSpeed))
				{
					refreshChildPositions();
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
			renderInfo.mDrawer.pushScissor(mFinalRect);
		}

		Widget::render(renderInfo);

		if (mUseScrolling)
		{
			renderInfo.mDrawer.popScissor();
		}
	}

	void VerticalLayout::applyLayouting()
	{
		// Apply vertical layout
		Vec2i cursor(getInnerPadding().mLeft, getInnerPadding().mTop);

		for (Widget* widget : getChildWidgets())
		{
			cursor.y += widget->getOuterMargin().mTop;

			const Recti rect(cursor, widget->getRelativeRect().getSize());
			widget->setRelativeRect(rect);

			cursor.y += rect.height;
			cursor.y += widget->getOuterMargin().mBottom;

			widget->refreshLayout();
		}

		cursor.y += getInnerPadding().mBottom;
		mInnerSize = cursor.y;
	}

	void VerticalLayout::refreshChildBaseOffset()
	{
		mChildBaseOffset = mFinalRect.getPos();
		mChildBaseOffset.y -= mScrollingController.getScrollPosition();
	}

	void VerticalLayout::onFocusGained()
	{
		if (mFocusedChildIndex < 0 && !getChildWidgets().empty())
		{
			changeFocusedChildindex(+1);
		}
	}

	void VerticalLayout::onChangedFocusedChildIndex()
	{
		Widget* newSelectedChild = getChildWidget(mFocusedChildIndex);
		if (nullptr != newSelectedChild)
		{
			Recti rect = newSelectedChild->getRelativeRect();
			rect.y -= mSelectionScrollAddSpace;
			rect.height += mSelectionScrollAddSpace * 2;
			scrollToRect(rect);
		}
	}

	void VerticalLayout::changeFocusedChildindex(int direction)
	{
		const int newIndex = getNextInteractableChildIndex(mFocusedChildIndex, direction);
		if (newIndex != mFocusedChildIndex)
		{
			Widget* newSelectedChild = getChildWidget(newIndex);
			if (nullptr != newSelectedChild)
			{
				newSelectedChild->grantFocus();
			}
		}
	}
}
