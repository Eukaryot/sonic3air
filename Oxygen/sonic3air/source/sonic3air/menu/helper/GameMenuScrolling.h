/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class GameMenuScrolling
{
public:
	inline void setVisibleAreaHeight(float height)  { mVisibleAreaHeight = height; }
	void setCurrentSelection(int selectionY1, int selectionY2);
	void setCurrentSelection(float selectionY1, float selectionY2);

	inline float getScrollOffsetY() const  { return mScrollOffsetY; }
	int getScrollOffsetYInt() const;
	void update(float timeElapsed);

public:
	float mVisibleAreaHeight = 224.0f;
	float mCurrentSelectionY1 = 0.0f;
	float mCurrentSelectionY2 = 0.0f;
	float mScrollOffsetY = 0.0f;
	bool mScrollingFast = false;
};
