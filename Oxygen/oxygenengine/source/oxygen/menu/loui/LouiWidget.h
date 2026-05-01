/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiDefinitions.h"


namespace loui
{
	class Widget
	{
	friend class Container;

	public:
		Widget();
		virtual ~Widget();

		void clearChildren();
		void addChildWidget(Widget& widget, bool autoDelete = false);

		template<typename T>
		T& createChildWidget()
		{
			T* widget = new T();
			addChildWidget(*widget, true);
			return *widget;
		}

		void removeChildWidget(Widget& widget);

		inline Widget* getParentWidget() const  { return mParentWidget; }
		inline const std::vector<Widget*> getChildWidgets() const  { return mChildWidgets; }
		inline Widget* getChildWidget(int index) const  { return (index >= 0 && index < (int)mChildWidgets.size()) ? mChildWidgets[index] : nullptr; }

		void moveToFront();
		void moveToBack();

		void refreshLayout();

		inline const Recti& getRelativeRect() const  { return mRelativeRect; }
		virtual void setRelativeRect(const Recti& rect);
		void refreshChildPositions();

		inline const Recti& getFinalRect() const  { return mFinalRect; }

		inline const Borders& getInnerPadding() const							{ return mInnerPadding; }
		inline void setInnerPadding(const Borders& padding)						{ mInnerPadding = padding; }
		inline void setInnerPadding(int top, int bottom, int left, int right)	{ setInnerPadding(Borders { top, bottom, left, right }); }

		inline const Borders& getOuterMargin() const							{ return mOuterMargin; }
		inline void setOuterMargin(const Borders& margin)						{ mOuterMargin = margin; }
		inline void setOuterMargin(int top, int bottom, int left, int right)	{ setOuterMargin(Borders { top, bottom, left, right }); }

		inline bool isFocusedChild() const	{ return mFocusState >= FocusState::FOCUSED_CHILD; }
		inline bool hasFocus() const		{ return mFocusState == FocusState::FULL_FOCUS; }
		void makeFocusedChild(bool focus = true);
		void grantFocus(bool focus = true);

		inline bool isVisible() const  { return mIsVisible; }
		virtual void setVisible(bool visible);

		inline bool isInteractable() const  { return mIsInteractable && mIsVisible; }
		virtual void setInteractable(bool interactable);

		inline float getOpacity() const  { return mOpacity; }
		virtual void setOpacity(float opacity);

		virtual void update(UpdateInfo& updateInfo);
		virtual void render(RenderInfo& renderInfo);

	protected:
		enum class FocusState
		{
			NONE,			// Widget is not focused at all
			FOCUSED_CHILD,	// Widget would be focused, if its parent was fully focused
			FULL_FOCUS,		// Widget has full focus (and so does its parent)
		};

	protected:
		virtual void applyLayouting() {}
		virtual void refreshChildBaseOffset();

		void setFocusStateInternal(FocusState newState);
		virtual void onFocusGained() {}
		virtual void onFocusLost() {}
		virtual void onChangedFocusedChildIndex() {}

		int getIndexOfChild(Widget& childWidget) const;
		int getNextInteractableChildIndex(int index, int direction) const;

	private:
		bool removeChildWidgetInternal(Widget& childWidget);

		void beginIteratingChildren();
		void endIteratingChildren();

	protected:
		// Layout
		Borders mInnerPadding;
		Borders mOuterMargin;

		Recti mRelativeRect;		// Positioning relative to parent's inner rect
		Recti mFinalRect;			// Final rect on screen (or whatever coordinate system the root is using)
		Vec2i mChildBaseOffset;		// Difference between relative rect and final rect for child widgets
		bool mLayoutDirty = false;

		FocusState mFocusState = FocusState::NONE;
		int mFocusedChildIndex = -1;

		// Flags
		bool mIsVisible = true;				// Evaluated visiblity value, considering visibility of the parent
		bool mIsVisibleSelf = true;			// Own local visibility value
		bool mIsInteractable = true;
		bool mIsInteractableSelf = true;
		float mOpacity = 1.0f;
		float mOpacitySelf = 1.0f;

	private:
		// Widget hierarchy
		Widget* mParentWidget = nullptr;
		std::vector<Widget*> mChildWidgets;
		std::vector<Widget*> mChildrenToRemove;
		bool mIteratingChildren = false;

		bool mAutoDelete = true;
	};
}
