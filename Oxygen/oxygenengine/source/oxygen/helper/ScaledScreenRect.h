/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct ScaledScreenRect
{
public:
	void setResolution(const Vec2i& resolution);
	inline const Vec2i& getResolution() const  { return mResolution; }

	inline bool isValidInnerPosition(const Vec2i& pos) const  { return inside(pos.x, 0, mResolution.x - 1) && inside(pos.y, 0, mResolution.y - 1); }

	inline const Recti& getRectOnScreen() const  { return mRectOnScreen; }
	void setRectOnScreen(const Recti& rect);

	void buildRectAspectFit(const Recti& outerRect, float stretchTowardsFullscreen = 0.0f);
	void buildRectIntegerAspectFit(const Recti& outerRect);
	void buildRectScaleToFill(const Recti& outerRect);
	void buildRectWithAlignment(const Recti& outerRect, float relativeScale, const Vec2f& relativeAlignment, bool withIntegerScaling);

	Vec2f getInnerPositionFromScreen(const Vec2f& screenPosition) const;
	Rectf getInnerRectFromScreen(const Rectf& screenRect) const;

	Vec2f getScreenPositionFromInner(const Vec2f& innerPosition) const;
	Rectf getScreenRectFromInner(const Rectf& innerRect) const;

private:
	Vec2i mResolution;
	Recti mRectOnScreen;
};
