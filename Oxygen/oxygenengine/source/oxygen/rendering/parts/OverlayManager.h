/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class OverlayManager
{
public:
	enum class Context
	{
		INSIDE_FRAME,	// Rendered during frame simulation
		OUTSIDE_FRAME	// Rendered outside of frame simulation, e.g. for debug side panel update
	};

	struct DebugDrawRect
	{
		Recti mRect;
		Color mColor;
		Context mContext = Context::OUTSIDE_FRAME;
	};

public:
	void preFrameUpdate();
	void postFrameUpdate();

	inline const std::vector<DebugDrawRect>& getDebugDrawRects() const  { return mDebugDrawRects; }
	void clearDebugDrawRects();
	void clearDebugDrawRects(Context context);
	void addDebugDrawRect(const Recti& rect, const Color& color = Color(1.0f, 0.0f, 1.0f, 0.75f));

private:
	Context mCurrentContext = Context::OUTSIDE_FRAME;
	std::vector<DebugDrawRect> mDebugDrawRects;
};
