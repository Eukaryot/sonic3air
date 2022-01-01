/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/OverlayManager.h"


void OverlayManager::preFrameUpdate()
{
	clear();	// Clear both contexts
	mCurrentContext = Context::INSIDE_FRAME;
}

void OverlayManager::postFrameUpdate()
{
	clearContext(Context::OUTSIDE_FRAME);
	mCurrentContext = Context::OUTSIDE_FRAME;
}

void OverlayManager::clear()
{
	clearContext(Context::INSIDE_FRAME);
	clearContext(Context::OUTSIDE_FRAME);
}

void OverlayManager::clearContext(Context context)
{
	mDebugDrawRects[(int)context].clear();
	mTexts[(int)context].clear();
}

void OverlayManager::addDebugDrawRect(const Recti& rect, const Color& color)
{
	DebugDrawRect& ddr = vectorAdd(mDebugDrawRects[(int)mCurrentContext]);
	ddr.mRect = rect;
	ddr.mColor = color;
	ddr.mContext = mCurrentContext;
}

void OverlayManager::addText(const std::string& fontKeyString, uint64 fontKeyHash, const Vec2i& position, const std::string& textString, uint64 textHash, const Color& color, int alignment, int spacing, uint16 renderQueue, Space space)
{
	Text& newText = vectorAdd(mTexts[(int)mCurrentContext]);
	newText.mFontKeyString = fontKeyString;
	newText.mFontKeyHash = fontKeyHash;
	newText.mPosition = position;
	newText.mTextHash = textHash;
	newText.mTextString = textString;
	newText.mColor = color;
	newText.mAlignment = alignment;
	newText.mSpacing = spacing;
	newText.mRenderQueue = renderQueue;
	newText.mSpace = space;
	newText.mContext = mCurrentContext;
}
