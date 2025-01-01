/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/helper/GameMenuScrolling.h"


void GameMenuScrolling::setCurrentSelection(int selectionY1, int selectionY2)
{
	mCurrentSelectionY1 = (float)selectionY1;
	mCurrentSelectionY2 = (float)selectionY2;
}

void GameMenuScrolling::setCurrentSelection(float selectionY1, float selectionY2)
{
	mCurrentSelectionY1 = selectionY1;
	mCurrentSelectionY2 = selectionY2;
}

int GameMenuScrolling::getScrollOffsetYInt() const
{
	return roundToInt(mScrollOffsetY);
}

void GameMenuScrolling::update(float timeElapsed)
{
	const float maxScrollOffset = std::max(mCurrentSelectionY1, 0.0f);
	const float minScrollOffset = mCurrentSelectionY2 - mVisibleAreaHeight;
	const float targetY = clamp(mScrollOffsetY, minScrollOffset, maxScrollOffset);

	// Still scrolling at all?
	if (targetY == mScrollOffsetY)
	{
		mScrollingFast = false;
		return;
	}

	// Switch to fast scrolling if it's a long way to go; or even skip part of it
	if (std::fabs(targetY - mScrollOffsetY) > 500.0f)
	{
		mScrollOffsetY = clamp(mScrollOffsetY, targetY - 500.0f, targetY + 500.0f);
	}
	if (std::fabs(targetY - mScrollOffsetY) > 100.0f)
	{
		mScrollingFast = true;
	}

	const float maxStep = timeElapsed * (mScrollingFast ? 1200.0f : 450.0f);
	if (mScrollOffsetY < targetY)
	{
		mScrollOffsetY = std::min(mScrollOffsetY + maxStep, targetY);
	}
	else
	{
		mScrollOffsetY = std::max(mScrollOffsetY - maxStep, targetY);
	}
}
