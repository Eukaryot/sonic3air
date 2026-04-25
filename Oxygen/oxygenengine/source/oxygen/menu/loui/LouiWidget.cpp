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
	}

	void Widget::addChildWidget(Widget& widget, bool autoDelete)
	{
		RMX_ASSERT(nullptr == widget.mParentWidget, "Double insertion of widget");
		mChildWidgets.push_back(&widget);
		widget.mParentWidget = this;
		widget.mAutoDelete = autoDelete;
		mLayoutDirty = true;
	}

	void Widget::moveToFront()
	{
		if (nullptr != mParentWidget)
		{
			if (mParentWidget->removeChildWidgetInternal(*this))
			{
				mParentWidget->mChildWidgets.insert(mParentWidget->mChildWidgets.begin(), this);
			}
		}
	}

	void Widget::moveToBack()
	{
		if (nullptr != mParentWidget)
		{
			if (mParentWidget->removeChildWidgetInternal(*this))
			{
				mParentWidget->mChildWidgets.push_back(this);
			}
		}
	}

	void Widget::refreshLayout()
	{
		applyLayouting();

		Vec2i basePosition = mFinalScreenRect.getPos();
		basePosition.x += mInnerPadding.mLeft;
		basePosition.y += mInnerPadding.mTop;
		basePosition += getInnerOffset();

		for (Widget* widget : getChildWidgets())
		{
			widget->setFinalScreenRect(widget->getRelativeRect() + basePosition);
			widget->applyLayouting();
		}

		mLayoutDirty = false;
	}

	void Widget::setRelativeRect(const Recti& rect)
	{
		mRelativeRect = rect;

		if (nullptr == mParentWidget)
		{
			mFinalScreenRect = mRelativeRect;
		}
		else
		{
			mFinalScreenRect = mRelativeRect + mParentWidget->mFinalScreenRect.getPos();
		}
	}

	void Widget::setFinalScreenRect(const Recti& rect)
	{
		mFinalScreenRect = rect;
	}

	void Widget::setVisible(bool visible)
	{
		mIsVisible = visible;
	}

	void Widget::setInteractable(bool interactable)
	{
		mIsInteractable = interactable;
	}

	void Widget::setSelected(bool selected)
	{
		mIsSelected = selected;
	}

	void Widget::update(UpdateInfo& updateInfo)
	{
		if (mLayoutDirty)
		{
			refreshLayout();
		}

		for (Widget* childWidget : mChildWidgets)
		{
			childWidget->update(updateInfo);
		}
	}

	void Widget::render(RenderInfo& renderInfo)
	{
	#if DEBUG
		if (FTX::keyState('^') && FTX::keyState(SDLK_LALT))
		{
			renderInfo.mDrawer.drawRect(mFinalScreenRect, Color(0.0f, 1.0f, 0.0f, 0.1f));
		}
	#endif

		for (Widget* childWidget : mChildWidgets)
		{
			RenderInfo childRenderInfo = renderInfo;
			childRenderInfo.mIsVisible = (renderInfo.mIsVisible && childWidget->mIsVisible);
			childRenderInfo.mIsInteractable = (renderInfo.mIsInteractable && childWidget->mIsInteractable);
			childRenderInfo.mOpacity = (renderInfo.mOpacity * childWidget->mOpacity);

			childWidget->render(childRenderInfo);
		}
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

			if (getChildWidgets()[index]->isInteractable())
				return index;
		}
		return -1;
	}

	bool Widget::removeChildWidgetInternal(Widget& childWidget)
	{
		for (auto it = mChildWidgets.begin(); it != mChildWidgets.end(); ++it)
		{
			if (*it == &childWidget)
			{
				mChildWidgets.erase(it);
				return true;
			}
		}
		return false;
	}
}
