/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiWidget.h"
#include "oxygen/menu/loui/LouiScrollingController.h"


namespace loui
{
	class VerticalLayout : public Widget
	{
	public:
		void setScrolling(bool useScrolling)  { mUseScrolling = useScrolling; }

		void scrollToRect(const Recti& localRect, bool animated = true);

		virtual void update(UpdateInfo& updateInfo) override;
		virtual void render(RenderInfo& renderInfo) override;

	protected:
		virtual void applyLayouting() override;
		virtual void refreshChildBaseOffset() override;

		virtual void onFocusGained() override;
		virtual void onChangedFocusedChildIndex() override;

		void changeFocusedChildindex(int direction);

	protected:
		// Scrolling
		bool mUseScrolling = false;
		int mWheelScrollSpeed = 20;
		int mSelectionScrollAddSpace = 15;	// Number of pixels around the selected child that get shown as well

		int mInnerSize = 0;
		ScrollingController mScrollingController;

		bool mDraggedByMouse = false;
		int mLastMouseY = 0;
	};
}
