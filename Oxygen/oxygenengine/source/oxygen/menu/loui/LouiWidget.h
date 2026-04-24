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
		void addChildWidget(Widget& widget, bool autoDelete);

		template<typename T>
		T& createChildWidget()
		{
			T* widget = new T();
			addChildWidget(*widget, true);
			return *widget;
		}

		inline Widget* getParentWidget() const  { return mParentWidget; }
		inline const std::vector<Widget*> getChildWidgets() const  { return mChildWidgets; }
		inline Widget* getChildWidget(int index) const  { return (index >= 0 && index < (int)mChildWidgets.size()) ? mChildWidgets[index] : nullptr; }

		void moveToFront();
		void moveToBack();

		void refreshLayout();

		inline const Recti& getRelativeRect() const  { return mRelativeRect; }
		void setRelativeRect(const Recti& rect);

		inline const Recti& getFinalScreenRect() const  { return mFinalScreenRect; }
		void setFinalScreenRect(const Recti& rect);

		inline const Borders& getInnerPadding() const							{ return mInnerPadding; }
		inline void setInnerPadding(const Borders& padding)						{ mInnerPadding = padding; }
		inline void setInnerPadding(int top, int bottom, int left, int right)	{ setInnerPadding(Borders { top, bottom, left, right }); }

		inline const Borders& getOuterMargin() const							{ return mOuterMargin; }
		inline void setOuterMargin(const Borders& margin)						{ mOuterMargin = margin; }
		inline void setOuterMargin(int top, int bottom, int left, int right)	{ setOuterMargin(Borders { top, bottom, left, right }); }

		virtual Vec2i getInnerOffset() const  { return Vec2i::ZERO; }

		inline bool isVisible() const  { return mIsVisible; }
		virtual void setVisible(bool visible);

		inline bool isInteractable() const  { return mIsInteractable; }
		virtual void setInteractable(bool interactable);

		inline bool isSelected() const  { return mIsSelected; }
		virtual void setSelected(bool selected);

		virtual void update(UpdateInfo& updateInfo);
		virtual void render(RenderInfo& renderInfo);

	protected:
		virtual void applyLayouting() {}

		int getNextInteractableChildIndex(int index, int direction) const;

	private:
		bool removeChildWidgetInternal(Widget& childWidget);

	protected:
		// Layout
		Borders mInnerPadding;
		Borders mOuterMargin;

		bool mLayoutDirty = false;
		Recti mRelativeRect;		// Positioning relative to parent's inner rect
		Recti mFinalScreenRect;		// Final screen rect

		// Flags
		bool mIsVisible = true;
		bool mIsInteractable = true;
		bool mIsSelected = false;
		float mOpacity = 1.0f;

	private:
		// Widget hierarchy
		Widget* mParentWidget = nullptr;
		std::vector<Widget*> mChildWidgets;
		bool mAutoDelete = true;
	};
}
