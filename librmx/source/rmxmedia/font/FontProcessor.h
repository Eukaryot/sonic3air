/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Font
*		Rendering of text.
*/

#pragma once

#include "Font.h"


struct FontProcessingData
{
	Bitmap mBitmap;
	int mBorderLeft = 0;
	int mBorderRight = 0;
	int mBorderTop = 0;
	int mBorderBottom = 0;
};


class FontProcessor
{
public:
	virtual void process(FontProcessingData& data) = 0;
};


class ShadowFontProcessor : public FontProcessor
{
public:
	inline explicit ShadowFontProcessor(Vec2i shadowOffset = Vec2i(1, 1), float shadowBlur = 1.0f, Color shadowColor = Color::BLACK) :
		mShadowOffset(shadowOffset),
		mShadowBlur(shadowBlur),
		mShadowColor(shadowColor)
	{}

	inline explicit ShadowFontProcessor(Vec2i shadowOffset, float shadowBlur, float shadowAlpha) :
		mShadowOffset(shadowOffset),
		mShadowBlur(shadowBlur),
		mShadowColor(0.0f, 0.0f, 0.0f, shadowAlpha)
	{}

	virtual void process(FontProcessingData& data) override;

private:
	Vec2i mShadowOffset;
	float mShadowBlur = 1.0f;
	Color mShadowColor = Color::BLACK;
};


class OutlineFontProcessor : public FontProcessor
{
public:
	inline explicit OutlineFontProcessor(Color outlineColor = Color::BLACK, int range = 1, bool rectangularOutline = false) :
		mOutlineColor(outlineColor),
		mRange(range),
		mRectangularOutline(rectangularOutline)
	{}

	virtual void process(FontProcessingData& data) override;

private:
	Color mOutlineColor = Color::BLACK;
	int mRange = 1;
	bool mRectangularOutline = false;
};


class GradientFontProcessor : public FontProcessor
{
public:
	inline explicit GradientFontProcessor()
	{}

	virtual void process(FontProcessingData& data) override;
};
