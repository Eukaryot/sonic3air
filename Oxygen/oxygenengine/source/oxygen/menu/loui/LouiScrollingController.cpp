/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/LouiScrollingController.h"


namespace loui
{
	void ScrollingController::setScrollAreaSize(int totalSize, int visibleSize)
	{
		mTotalAreaSize = totalSize;
		mVisibleAreaSize = visibleSize;
	}

	bool ScrollingController::scrollToTargetPosition(int position, bool animated)
	{
		mScrollTargetPosition = clamp(position, 0, mTotalAreaSize - mVisibleAreaSize);
		mCurrentScrollSpeed = 0.0f;
		mScrollPosFractional = 0.0f;

		if (mScrollPosition == mScrollTargetPosition)
			return false;

		if (animated)
		{
			// TODO: Scroll faster if it's too far away
			mCurrentScrollSpeed = 450.0f;
		}
		else
		{
			mScrollPosition = mScrollTargetPosition;
		}
		return true;
	}

	bool ScrollingController::scrollToMakeVisible(int position, int size, bool animated)
	{
		int targetPos = mScrollPosition;
		targetPos = std::max(targetPos, position + size - mVisibleAreaSize);
		targetPos = std::min(targetPos, position);

		return scrollToTargetPosition(targetPos, animated);
	}

	bool ScrollingController::scrollRelative(int change)
	{
		return scrollToTargetPosition(mScrollPosition + change, false);
	}

	bool ScrollingController::update(float deltaSeconds)
	{
		if (mCurrentScrollSpeed <= 0.0f)
			return false;

		const float distance = (float)std::abs(mScrollPosition - mScrollTargetPosition);
		const float step = std::max(mCurrentScrollSpeed, distance * 20.0f) * deltaSeconds;
		if (step > distance)
		{
			mScrollPosition = mScrollTargetPosition;
			mScrollPosFractional = 0.0f;
			mCurrentScrollSpeed = 0.0f;
		}
		else
		{
			mScrollPosFractional += (mScrollTargetPosition < mScrollPosition) ? -step : step;
			const int rounded = roundToInt(mScrollPosFractional);
			mScrollPosition += rounded;
			mScrollPosFractional -= (float)rounded;
		}
		return true;
	}
}
