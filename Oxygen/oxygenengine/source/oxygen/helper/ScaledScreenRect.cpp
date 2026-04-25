/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/ScaledScreenRect.h"
#include "oxygen/rendering/utils/RenderUtils.h"


void ScaledScreenRect::setResolution(const Vec2i& resolution)
{
	mResolution = resolution;
}

void ScaledScreenRect::setRectOnScreen(const Recti& rect)
{
	mRectOnScreen = rect;
}

void ScaledScreenRect::buildRectAspectFit(const Recti& outerRect, float stretchTowardsFullscreen)
{
	mRectOnScreen = RenderUtils::getLetterBoxRect(outerRect, (float)mResolution.x / (float)mResolution.y);

	if (stretchTowardsFullscreen > 0.0f)
	{
		Rectf rect1(mRectOnScreen);
		Rectf rect2(outerRect);
		stretchTowardsFullscreen = saturate(stretchTowardsFullscreen);

		mRectOnScreen.setPos(Vec2i(Vec2f::interpolate(rect1.getPos(), rect2.getPos(), stretchTowardsFullscreen)));
		mRectOnScreen.setSize(Vec2i(Vec2f::interpolate(rect1.getSize(), rect2.getSize(), stretchTowardsFullscreen)));
	}
}

void ScaledScreenRect::buildRectIntegerAspectFit(const Recti& outerRect)
{
	buildRectAspectFit(outerRect);

	const int scale = mRectOnScreen.height / mResolution.y;
	if (scale >= 1)
	{
		mRectOnScreen.width = roundToInt((float)mResolution.x * scale);
		mRectOnScreen.height = roundToInt((float)mResolution.y * scale);
		mRectOnScreen.x = outerRect.x + (outerRect.width - mRectOnScreen.width) / 2;
		mRectOnScreen.y = outerRect.y + (outerRect.height - mRectOnScreen.height) / 2;
	}
}

void ScaledScreenRect::buildRectScaleToFill(const Recti& outerRect)
{
	mRectOnScreen = RenderUtils::getScaleToFillRect(outerRect, (float)mResolution.x / (float)mResolution.y);
}

void ScaledScreenRect::buildRectWithAlignment(const Recti& outerRect, float relativeScale, const Vec2f& relativeAlignment, bool withIntegerScaling)
{
	// Consider integer scaling
	Vec2f scaledSize = Vec2f(mRectOnScreen.getSize()) * relativeScale;
	if (withIntegerScaling)
	{
		const int scale = std::max((int)((float)mRectOnScreen.height * relativeScale) / mResolution.y, 1);
		scaledSize = Vec2f(mResolution * scale);
	}

	const Vec2f maxPos = Vec2f(outerRect.getEndPos()) - scaledSize;
	mRectOnScreen.x = roundToInt(maxPos.x * (1.0f + relativeAlignment.x) / 2.0f);
	mRectOnScreen.y = roundToInt(maxPos.y * (1.0f + relativeAlignment.y) / 2.0f);
	mRectOnScreen.setSize(Vec2i(scaledSize));
}

Vec2f ScaledScreenRect::getInnerPositionFromScreen(const Vec2f& screenPosition) const
{
	const Vec2f normalizedPos = (screenPosition - Vec2f(mRectOnScreen.getPos())) / Vec2f(mRectOnScreen.getSize());
	return normalizedPos * Vec2f(mResolution);
}

Rectf ScaledScreenRect::getInnerRectFromScreen(const Rectf& screenRect) const
{
	const Vec2f normalizedPos = (screenRect.getPos() - Vec2f(mRectOnScreen.getPos())) / Vec2f(mRectOnScreen.getSize());
	const Vec2f normalizedSize = screenRect.getSize() / Vec2f(mRectOnScreen.getSize());
	return Rectf(normalizedPos * Vec2f(mResolution), normalizedSize * Vec2f(mResolution));
}

Vec2f ScaledScreenRect::getScreenPositionFromInner(const Vec2f& innerPosition) const
{
	const Vec2f normalizedPos = innerPosition / Vec2f(mResolution);
	return Vec2f(mRectOnScreen.getPos()) + normalizedPos * Vec2f(mRectOnScreen.getSize());
}

Rectf ScaledScreenRect::getScreenRectFromInner(const Rectf& innerRect) const
{
	const Vec2f normalizedPos = innerRect.getPos() / Vec2f(mResolution);
	const Vec2f normalizedSize = innerRect.getSize() / Vec2f(mResolution);
	return Rectf(Vec2f(mRectOnScreen.getPos()) + normalizedPos * Vec2f(mRectOnScreen.getSize()), Vec2f(normalizedSize * Vec2f(mRectOnScreen.getSize())));
}
