/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/OverlayManager.h"


void OverlayManager::preFrameUpdate()
{
	clearDebugDrawRects();	// Clear both contexts
	mCurrentContext = Context::INSIDE_FRAME;
}

void OverlayManager::postFrameUpdate()
{
	clearDebugDrawRects(Context::OUTSIDE_FRAME);
	mCurrentContext = Context::OUTSIDE_FRAME;
}

void OverlayManager::clearDebugDrawRects()
{
	for (int i = 0; i < NUM_CONTEXTS; ++i)
		mDebugDrawRects[i].clear();
}

void OverlayManager::clearDebugDrawRects(Context context)
{
	mDebugDrawRects[(int)context].clear();
}

void OverlayManager::addDebugDrawRect(const Recti& rect, const Color& color)
{
	DebugDrawRect& ddr = vectorAdd(mDebugDrawRects[(int)mCurrentContext]);
	ddr.mRect = rect;
	ddr.mColor = color;
	ddr.mContext = mCurrentContext;
}
