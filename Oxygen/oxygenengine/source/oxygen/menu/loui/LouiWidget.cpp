/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/LouiWidget.h"


namespace loui
{
	Widget::Widget() :
		mRelativeRect(0, 0, 100, 20)	// Just some default size
	{
	}

	Widget::~Widget()
	{
		clearChildren();
	}

	void Widget::clearChildren()
	{
		for (Widget* childWidget : mChildWidgets)
		{
			if (childWidget->mAutoDelete)
			{
				delete childWidget;
			}
			else
			{
				childWidget->mParentWidget = nullptr;
			}
		}
		mChildWidgets.clear();
		mLayoutDirty = false;
		mFocusedChildIndex = -1;
	}

	void Widget::addChildWidget(Widget& widget, bool autoDelete)
	{
		RMX_ASSERT(nullptr == widget.mParentWidget, "Double insertion of widget");
		mChildWidgets.push_back(&widget);
		widget.mParentWidget = this;
		widget.mAutoDelete = autoDelete;
		mLayoutDirty = true;
	}

	void Widget::removeChildWidget(Widget& widget)
	{
		if (widget.mParentWidget != this)
			return;

		widget.mParentWidget = nullptr;

		if (mIteratingChildren)
		{
			mChildrenToRemove.push_back(&widget);
		}
		else
		{
			removeChildWidgetInternal(widget);
		}
	}

	void Widget::moveToFront()
	{
		if (nullptr != mParentWidget)
		{
			const int index = mParentWidget->getIndexOfChild(*this);
			if (index > 0)
			{
				// Move to the front of parent's list of children
				mChildWidgets.erase(mChildWidgets.begin() + index);
				mParentWidget->mChildWidgets.insert(mParentWidget->mChildWidgets.begin(), this);

				// Update focused child index of the parent if needed
				if (mParentWidget->mFocusedChildIndex == index)
				{
					mParentWidget->mFocusedChildIndex = 0;
					mParentWidget->onChangedFocusedChildIndex();
				}
				else if (mParentWidget->mFocusedChildIndex >= 0 && mParentWidget->mFocusedChildIndex < index)
				{
					++mParentWidget->mFocusedChildIndex;
					mParentWidget->onChangedFocusedChildIndex();
				}
			}
		}
	}

	void Widget::moveToBack()
	{
		if (nullptr != mParentWidget)
		{
			const int index = mParentWidget->getIndexOfChild(*this);
			const int lastIndex = (int)mParentWidget->mChildWidgets.size() - 1;
			if (index >= 0 && index < lastIndex)
			{
				// Move to the back of parent's list of children
				mChildWidgets.erase(mChildWidgets.begin() + index);
				mParentWidget->mChildWidgets.push_back(this);

				// Update focused child index of the parent if needed
				if (mParentWidget->mFocusedChildIndex == index)
				{
					mParentWidget->mFocusedChildIndex = lastIndex;
					mParentWidget->onChangedFocusedChildIndex();
				}
				else if (mParentWidget->mFocusedChildIndex > index)
				{
					--mParentWidget->mFocusedChildIndex;
					mParentWidget->onChangedFocusedChildIndex();
				}
			}
		}
	}

	void Widget::refreshLayout()
	{
		applyLayouting();

		for (Widget* widget : getChildWidgets())
		{
			widget->applyLayouting();
		}

		mLayoutDirty = false;
	}

	void Widget::setRelativeRect(const Recti& rect)
	{
		if (mRelativeRect != rect)
		{
			mRelativeRect = rect;
			mLayoutDirty = true;
		}

		// Update final rect
		Recti newFinalRect = mRelativeRect;
		if (nullptr != mParentWidget)
		{
			newFinalRect += mParentWidget->mChildBaseOffset;
		}
		if (mFinalRect != newFinalRect)
		{
			mFinalRect = newFinalRect;
			refreshChildPositions();
		}
	}

	void Widget::refreshChildPositions()
	{
		refreshChildBaseOffset();

		for (Widget* childWidget : mChildWidgets)
		{
			childWidget->setRelativeRect(childWidget->mRelativeRect);
		}
	}

	void Widget::makeFocusedChild(bool focus)
	{
		if (focus)
		{
			if (mFocusState == FocusState::FULL_FOCUS || (nullptr == mParentWidget || mParentWidget->mFocusState == FocusState::FULL_FOCUS))
			{
				setFocusStateInternal(FocusState::FULL_FOCUS);
			}
			else
			{
				setFocusStateInternal(FocusState::FOCUSED_CHILD);
			}
		}
		else
		{
			setFocusStateInternal(FocusState::NONE);
		}
	}

	void Widget::grantFocus(bool focus)
	{
		if (focus)
		{
			setFocusStateInternal(FocusState::FULL_FOCUS);
		}
		else
		{
			setFocusStateInternal(FocusState::NONE);
		}
	}

	void Widget::setVisible(bool visible)
	{
		mIsVisibleSelf = visible;

		const bool newValue = mIsVisibleSelf && (nullptr != mParentWidget ? mParentWidget->mIsVisible : true);
		if (mIsVisible != newValue)
		{
			mIsVisible = newValue;
			for (Widget* childWidget : mChildWidgets)
			{
				childWidget->setVisible(childWidget->mIsVisibleSelf);
			}
		}
	}

	void Widget::setInteractable(bool interactable)
	{
		mIsInteractableSelf = interactable;

		const bool newValue = mIsInteractableSelf && (nullptr != mParentWidget ? mParentWidget->mIsInteractable : true);
		if (mIsInteractable != newValue)
		{
			mIsInteractable = newValue;
			for (Widget* childWidget : mChildWidgets)
			{
				childWidget->setInteractable(childWidget->mIsInteractableSelf);
			}
		}
	}

	void Widget::setOpacity(float opacity)
	{
		mOpacitySelf = opacity;

		const float newValue = mOpacitySelf * (nullptr != mParentWidget ? mParentWidget->mOpacity : 1.0f);
		if (mOpacity != newValue)
		{
			mOpacity = newValue;
			for (Widget* childWidget : mChildWidgets)
			{
				childWidget->setOpacity(childWidget->mOpacitySelf);
			}
		}
	}

	void Widget::update(UpdateInfo& updateInfo)
	{
		if (mLayoutDirty)
		{
			refreshLayout();
		}

		beginIteratingChildren();
		for (Widget* childWidget : mChildWidgets)
		{
			childWidget->update(updateInfo);
		}
		endIteratingChildren();
	}

	void Widget::render(RenderInfo& renderInfo)
	{
	#if DEBUG
		if (FTX::keyState('^') && FTX::keyState(SDLK_LALT))
		{
			renderInfo.mDrawer.drawRect(mFinalRect, Color(0.0f, 1.0f, 0.0f, 0.1f));
		}
	#endif

		// Render in reverse order, so the widget at the front gets rendered above all others
		beginIteratingChildren();
		for (int k = (int)mChildWidgets.size() - 1; k >= 0; --k)
		{
			Widget* widget = mChildWidgets[k];
			if (widget->isVisible())
			{
				RenderInfo childRenderInfo = renderInfo;
				widget->render(childRenderInfo);
			}
		}
		endIteratingChildren();
	}

	void Widget::refreshChildBaseOffset()
	{
		mChildBaseOffset = mFinalRect.getPos();
	}

	void Widget::setFocusStateInternal(FocusState newState)
	{
		// Any change?
		if (newState == mFocusState)
			return;

		const FocusState oldState = mFocusState;
		if (nullptr != mParentWidget)
		{
			// Remove or set as focus child in its parent
			if (newState == FocusState::NONE)
			{
				const int index = mParentWidget->getIndexOfChild(*this);
				if (mParentWidget->mFocusedChildIndex == index)
				{
					mParentWidget->mFocusedChildIndex = -1;
				}
			}
			else if (oldState == FocusState::NONE)
			{
				const int index = mParentWidget->getIndexOfChild(*this);
				RMX_ASSERT(index != mParentWidget->mFocusedChildIndex, "Inconsistency in focus state");

				// Update the old focused sibling
				Widget* oldFocusedSibling = mParentWidget->getChildWidget(mParentWidget->mFocusedChildIndex);
				if (nullptr != oldFocusedSibling)
				{
					oldFocusedSibling->setFocusStateInternal(FocusState::NONE);
				}

				mParentWidget->mFocusedChildIndex = index;
			}

			// Update own focus state
			mFocusState = newState;

			if (newState == FocusState::FULL_FOCUS)
			{
				// Ensure that the parent widget is fully focused as well
				mParentWidget->setFocusStateInternal(FocusState::FULL_FOCUS);
			}
		}
		else
		{
			// Update own focus state
			mFocusState = newState;
		}

		if (newState == FocusState::FULL_FOCUS)
		{
			// Update focused child widget
			Widget* focusedChild = getChildWidget(mFocusedChildIndex);
			if (nullptr != focusedChild)
			{
				focusedChild->setFocusStateInternal(FocusState::FULL_FOCUS);
			}

			onFocusGained();
		}
		else if (oldState == FocusState::FULL_FOCUS)
		{
			// Update focused child widget
			Widget* focusedChild = getChildWidget(mFocusedChildIndex);
			if (nullptr != focusedChild)
			{
				focusedChild->setFocusStateInternal(FocusState::FOCUSED_CHILD);
			}

			onFocusLost();
		}
	}

	int Widget::getIndexOfChild(Widget& childWidget) const
	{
		for (int k = 0; k < (int)mChildWidgets.size(); ++k)
		{
			if (mChildWidgets[k] == &childWidget)
				return k;
		}
		return -1;
	}

	int Widget::getNextInteractableChildIndex(int index, int direction) const
	{
		const int numChildren = (int)getChildWidgets().size();
		if (numChildren == 0)
			return -1;

		if (index < 0)
		{
			index = (direction < 0) ? numChildren : -1;
		}

		const int change = (direction < 0) ? (numChildren - 1) : 1;

		for (int k = 0; k < numChildren; ++k)
		{
			index = (index + change) % numChildren;

			if (getChildWidgets()[index]->mIsInteractableSelf)
				return index;
		}
		return -1;
	}

	bool Widget::removeChildWidgetInternal(Widget& childWidget)
	{
		const int index = getIndexOfChild(childWidget);
		if (index < 0)
			return false;

		mChildWidgets.erase(mChildWidgets.begin() + index);

		if (index == mFocusedChildIndex)
		{
			const int oldFocusedChildIndex = mFocusedChildIndex;
			mFocusedChildIndex = -1;
		}
		return true;
	}

	void Widget::beginIteratingChildren()
	{
		mIteratingChildren = true;
	}

	void Widget::endIteratingChildren()
	{
		mIteratingChildren = false;

		for (Widget* child : mChildrenToRemove)
		{
			removeChildWidgetInternal(*child);
		}
		mChildrenToRemove.clear();
	}
}
