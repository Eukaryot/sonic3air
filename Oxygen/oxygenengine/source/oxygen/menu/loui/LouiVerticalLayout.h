/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiWidget.h"


namespace loui
{
	class VerticalLayout : public Widget
	{
	public:
		void setScrolling(bool useScrolling)  { mUseScrolling = useScrolling; }

		void scrollToRect(const Recti& localRect);

		virtual void setSelected(bool selected) override;

		virtual void update(UpdateInfo& updateInfo) override;
		virtual void render(RenderInfo& renderInfo) override;

	protected:
		virtual Vec2i getInnerOffset() const  { return Vec2i(0, -mScrollPosition); }
		virtual void applyLayouting();

		void changeSelectedChildindex(int direction);

	protected:
		// Selection
		int mSelectedChildIndex = -1;

		// Scrolling
		bool mUseScrolling = false;
		int mWheelScrollSpeed = 20;
		int mScrollPosition = 0;
		int mInnerSize = 0;
		int mSelectionScrollAddSpace = 15;	// Number of pixels around the selected child that get shown as well
	};
}
