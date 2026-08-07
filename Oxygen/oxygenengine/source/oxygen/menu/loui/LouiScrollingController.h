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
	class ScrollingController
	{
	public:
		inline int getScrollPosition() const  { return mScrollPosition; }

		void setScrollAreaSize(int totalSize, int visibleSize);

		bool scrollToTargetPosition(int position, bool animated = true);
		bool scrollToMakeVisible(int position, int size, bool animated = true);
		bool scrollRelative(int change);

		bool update(float deltaSeconds);

	protected:
		int mTotalAreaSize = 0;
		int mVisibleAreaSize = 0;

		int mScrollPosition = 0;
		int mScrollTargetPosition = 0;
		float mScrollPosFractional = 0.0f;
		float mCurrentScrollSpeed = 0.0f;	// 0.0f while not scrolling, otherwise > 0.0f
	};
}
