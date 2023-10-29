/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/RenderItem.h"


class OverlayManager
{
public:
	using Space = SpacesManager::Space;

	struct Rectangle : public RenderItem
	{
		Recti mRect;
		Color mColor;

		inline Rectangle() : RenderItem(Type::RECTANGLE) {}
	};

	struct Text : public RenderItem
	{
		std::string mFontKeyString;
		uint64 mFontKeyHash = 0;
		Vec2i mPosition;
		std::string mTextString;
		uint64 mTextHash = 0;
		Color mColor;
		int mAlignment = 1;
		int mSpacing = 0;

		inline Text() : RenderItem(Type::TEXT) {}
	};

public:
	inline OverlayManager()  { clear(); }

	void preFrameUpdate();
	void postFrameUpdate(bool resetItems);

	inline const std::vector<RenderItem*>& getRenderItems(RenderItem::LifetimeContext context) const  { return mContexts[(int)context].mItems; }

	void clear();
	void clearLifetimeContext(RenderItem::LifetimeContext lifetimeContext);

	void addRectangle(const Recti& rect, const Color& color, uint16 renderQueue, Space space, bool useGlobalComponentTint);
	void addText(std::string_view fontKeyString, uint64 fontKeyHash, const Vec2i& position, std::string_view textString, uint64 textHash, const Color& color, int alignment, int spacing, uint16 renderQueue, Space space, bool useGlobalComponentTint);

private:
	struct ItemSet
	{
		std::vector<RenderItem*> mItems;
	};

private:
	ItemSet& getItemsByContext(RenderItem::LifetimeContext lifetimeContext);
	void destroyRenderItem(RenderItem& renderItem);

private:
	RenderItem::LifetimeContext mCurrentContext = RenderItem::LifetimeContext::OUTSIDE_FRAME;
	ItemSet mContexts[RenderItem::NUM_CONTEXTS];
	ItemSet mAddedItems;

	ObjectPool<Rectangle> mPoolOfRectangles;
	ObjectPool<Text> mPoolOfTexts;
};
