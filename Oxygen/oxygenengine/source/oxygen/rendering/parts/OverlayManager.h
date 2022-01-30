/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/SpacesManager.h"


class OverlayManager
{
public:
	using Space = SpacesManager::Space;

	enum class Context
	{
		INSIDE_FRAME = 0,	// Rendered during frame simulation
		OUTSIDE_FRAME = 1	// Rendered outside of frame simulation, e.g. for debug side panel update
	};
	static const size_t NUM_CONTEXTS = 2;

	struct DebugDrawRect
	{
		Recti mRect;
		Color mColor;
		Space mSpace = Space::WORLD;
		Context mContext = Context::OUTSIDE_FRAME;
	};

	struct Text
	{
		std::string mFontKeyString;
		uint64 mFontKeyHash = 0;
		Vec2i mPosition;
		std::string mTextString;
		uint64 mTextHash = 0;
		Color mColor;
		int mAlignment = 1;
		int mSpacing = 0;
		uint16 mRenderQueue = 0xffff;
		Space mSpace = Space::WORLD;
		Context mContext = Context::OUTSIDE_FRAME;
	};

public:
	void preFrameUpdate();
	void postFrameUpdate();

	inline const std::vector<DebugDrawRect>& getDebugDrawRects(Context context) const  { return mDebugDrawRects[(int)context]; }
	inline const std::vector<Text>& getTexts(Context context) const  { return mTexts[(int)context]; }

	void clear();
	void clearContext(Context context);

	void addDebugDrawRect(const Recti& rect, const Color& color = Color(1.0f, 0.0f, 1.0f, 0.75f));
	void addText(std::string_view fontKeyString, uint64 fontKeyHash, const Vec2i& position, std::string_view textString, uint64 textHash, const Color& color, int alignment, int spacing, uint16 renderQueue, Space space);

private:
	Context mCurrentContext = Context::OUTSIDE_FRAME;
	std::vector<DebugDrawRect> mDebugDrawRects[NUM_CONTEXTS];	// One list per context (see "Context" enum)
	std::vector<Text> mTexts[NUM_CONTEXTS];
};
