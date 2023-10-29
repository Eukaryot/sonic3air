/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/OverlayManager.h"


void OverlayManager::preFrameUpdate()
{
	mCurrentContext = RenderItem::LifetimeContext::DEFAULT;
}

void OverlayManager::postFrameUpdate(bool resetItems)
{
	if (resetItems)
	{
		for (int index = 0; index < RenderItem::NUM_CONTEXTS; ++index)
		{
			clearLifetimeContext((RenderItem::LifetimeContext)index);
		}
	}

	// Add render items from "next" to "current"
	for (RenderItem* renderItem : mAddedItems.mItems)
	{
		getItemsByContext(renderItem->mLifetimeContext).mItems.push_back(renderItem);
	}
	mAddedItems.mItems.clear();		// Intentionally not using anything like "clearLifetimeContext" here, as it would invalidate the copied instances

	clearLifetimeContext(RenderItem::LifetimeContext::OUTSIDE_FRAME);
	mCurrentContext = RenderItem::LifetimeContext::OUTSIDE_FRAME;
}

void OverlayManager::clear()
{
	for (int index = 0; index < RenderItem::NUM_CONTEXTS; ++index)
	{
		clearLifetimeContext((RenderItem::LifetimeContext)index);
	}
	for (RenderItem* renderItem : mAddedItems.mItems)
		destroyRenderItem(*renderItem);
	mAddedItems.mItems.clear();
}

void OverlayManager::clearLifetimeContext(RenderItem::LifetimeContext lifetimeContext)
{
	ItemSet& context = getItemsByContext(lifetimeContext);
	for (RenderItem* renderItem : context.mItems)
		destroyRenderItem(*renderItem);
	context.mItems.clear();
}

void OverlayManager::addRectangle(const Recti& rect, const Color& color, uint16 renderQueue, Space space, bool useGlobalComponentTint)
{
	Rectangle& newRect = mPoolOfRectangles.createObject();
	newRect.mRect = rect;
	newRect.mColor = color;
	newRect.mRenderQueue = renderQueue;
	newRect.mCoordinatesSpace = space;
	newRect.mLifetimeContext = mCurrentContext;
	newRect.mUseGlobalComponentTint = useGlobalComponentTint;
	mAddedItems.mItems.push_back(&newRect);
}

void OverlayManager::addText(std::string_view fontKeyString, uint64 fontKeyHash, const Vec2i& position, std::string_view textString, uint64 textHash, const Color& color, int alignment, int spacing, uint16 renderQueue, Space space, bool useGlobalComponentTint)
{
	Text& newText = mPoolOfTexts.createObject();
	newText.mFontKeyString = fontKeyString;
	newText.mFontKeyHash = fontKeyHash;
	newText.mPosition = position;
	newText.mTextHash = textHash;
	newText.mTextString = textString;
	newText.mColor = color;
	newText.mAlignment = alignment;
	newText.mSpacing = spacing;
	newText.mRenderQueue = renderQueue;
	newText.mCoordinatesSpace = space;
	newText.mLifetimeContext = mCurrentContext;
	newText.mUseGlobalComponentTint = useGlobalComponentTint;
	mAddedItems.mItems.push_back(&newText);
}

OverlayManager::ItemSet& OverlayManager::getItemsByContext(RenderItem::LifetimeContext lifetimeContext)
{
	RMX_ASSERT((int)lifetimeContext < RenderItem::NUM_CONTEXTS, "Invalid lifetime context " << (int)lifetimeContext);
	return mContexts[(int)lifetimeContext];
}

void OverlayManager::destroyRenderItem(RenderItem& renderItem)
{
	switch (renderItem.getType())
	{
		case RenderItem::Type::RECTANGLE: mPoolOfRectangles.destroyObject(static_cast<Rectangle&>(renderItem));  break;
		case RenderItem::Type::TEXT:	  mPoolOfTexts.destroyObject(static_cast<Text&>(renderItem));  break;
		default:
			RMX_ASSERT(false, "Trying to destroy unsupported render item type");
			break;
	}
}
