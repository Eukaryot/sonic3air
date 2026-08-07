/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


void ShadowFontProcessor::process(FontProcessingData& data)
{
	if (mShadowColor.a < 0.001f)
		return;

	// Add shadow effect
	const int oldBorderLeft   = data.mBorderLeft;
	const int oldBorderRight  = data.mBorderRight;
	const int oldBorderTop    = data.mBorderTop;
	const int oldBorderBottom = data.mBorderBottom;

	const int border = clamp(roundToInt(mShadowBlur * 2.0f), 0, 16);
	const int offsX = clamp(mShadowOffset.x, 0, 16);
	const int offsY = clamp(mShadowOffset.y, 0, 16);

	data.mBorderLeft   = oldBorderLeft   + std::max(0, border - offsX);
	data.mBorderRight  = oldBorderRight  + border + offsX;
	data.mBorderTop    = oldBorderTop    + std::max(0, border - offsY);
	data.mBorderBottom = oldBorderBottom + border + offsY;

	const int newWidth  = data.mBitmap.getWidth()  + (data.mBorderLeft + data.mBorderRight) - (oldBorderLeft + oldBorderRight);
	const int newHeight = data.mBitmap.getHeight() + (data.mBorderTop + data.mBorderBottom) - (oldBorderTop + oldBorderBottom);
	const int insetX = data.mBorderLeft - oldBorderLeft;
	const int insetY = data.mBorderTop - oldBorderTop;

	Bitmap bmp;
	bmp.create(newWidth, newHeight, 0);
	bmp.insertBlend(insetX + offsX, insetY + offsY, data.mBitmap);
	if (mShadowBlur > 0.0f)
	{
		bmp.gaussianBlur(bmp, mShadowBlur);
	}

	const uint32 shadowRGB = mShadowColor.getABGR32() & 0xffffff;
	if (mShadowColor.a < 1.0f)
	{
		uint32* data = bmp.getData();
		for (int i = 0; i < bmp.getPixelCount(); ++i)
		{
			data[i] = shadowRGB + (roundToInt((float)(data[i] >> 24) * mShadowColor.a) << 24);
		}
	}
	else
	{
		bmp.clearRGB(shadowRGB);
	}
	bmp.insertBlend(insetX, insetY, data.mBitmap);
	data.mBitmap = bmp;
}


void OutlineFontProcessor::process(FontProcessingData& data)
{
	if (mRange <= 0)
		return;
	if (mRange > 10)
		mRange = 10;

	const int oldBorderLeft   = data.mBorderLeft;
	const int oldBorderRight  = data.mBorderRight;
	const int oldBorderTop    = data.mBorderTop;
	const int oldBorderBottom = data.mBorderBottom;

	data.mBorderLeft   = oldBorderLeft   + mRange;
	data.mBorderRight  = oldBorderRight  + mRange;
	data.mBorderTop    = oldBorderTop    + mRange;
	data.mBorderBottom = oldBorderBottom + mRange;

	const int newWidth  = data.mBitmap.getWidth()  + (data.mBorderLeft + data.mBorderRight) - (oldBorderLeft + oldBorderRight);
	const int newHeight = data.mBitmap.getHeight() + (data.mBorderTop + data.mBorderBottom) - (oldBorderTop + oldBorderBottom);
	const int insetX = data.mBorderLeft - oldBorderLeft;
	const int insetY = data.mBorderTop - oldBorderTop;

	Bitmap bitmap;
	bitmap.create(newWidth, newHeight, 0);

#if 1
	const uint32 outlineColorBGR = mOutlineColor.getABGR32() & 0x00ffffff;
	const uint32 outlineColorAlpha = mOutlineColor.getABGR32() >> 24;

	for (int y = 0; y < bitmap.getHeight(); ++y)
	{
		for (int x = 0; x < bitmap.getWidth(); ++x)
		{
			uint8 highestAlpha = 0;
			for (int dy = -mRange; dy <= mRange; ++dy)
			{
				int rangeX = mRange;
				if (!mRectangularOutline)
				{
					rangeX -= abs(dy);
				}

				const int originalY = y + dy - insetY;
				if (originalY >= 0 && originalY < data.mBitmap.getHeight())
				{
					for (int dx = -rangeX; dx <= rangeX; ++dx)
					{
						const int originalX = x + dx - insetX;
						if (originalX >= 0 && originalX < data.mBitmap.getWidth())	// TODO: This can be optimized by calculating the correct range for dx
						{
							const uint32 color = data.mBitmap.getPixel(originalX, originalY);
							const uint8 alpha = (color >> 24);
							highestAlpha = std::max(alpha, highestAlpha);
						}
					}
				}
			}

			highestAlpha = highestAlpha * outlineColorAlpha / 255;
			const uint32 outColor = outlineColorBGR + (highestAlpha << 24);
			bitmap.setPixel(x, y, outColor);
		}
	}
#else
	// Old simpler implementation, which mostly ignores alpha values
	const uint32 outlineColorABGR = mOutlineColor.getABGR32();
	for (int y = 0; y < data.mBitmap.getHeight(); ++y)
	{
		for (int x = 0; x < data.mBitmap.getWidth(); ++x)
		{
			if (data.mBitmap.getPixel(x, y) & 0xff000000)
			{
				bitmap.setPixel(insetX + x - 1, insetY + y, outlineColorABGR);
				bitmap.setPixel(insetX + x + 1, insetY + y, outlineColorABGR);
				bitmap.setPixel(insetX + x, insetY + y - 1, outlineColorABGR);
				bitmap.setPixel(insetX + x, insetY + y + 1, outlineColorABGR);

				if (mRectangularOutline)
				{
					bitmap.setPixel(insetX + x - 1, insetY + y - 1, outlineColorABGR);
					bitmap.setPixel(insetX + x - 1, insetY + y + 1, outlineColorABGR);
					bitmap.setPixel(insetX + x + 1, insetY + y - 1, outlineColorABGR);
					bitmap.setPixel(insetX + x + 1, insetY + y + 1, outlineColorABGR);
				}
			}
		}
	}
#endif

	bitmap.insertBlend(insetX, insetY, data.mBitmap);
	data.mBitmap = bitmap;
}


void GradientFontProcessor::process(FontProcessingData& data)
{
	for (int y = 0; y < data.mBitmap.getHeight(); ++y)
	{
		uint32* pixelPtr = data.mBitmap.getPixelPointer(0, y);
		const float shadingFactor = interpolate(0.65f, 1.0f, saturate((float)y / (float)data.mBitmap.getHeight() * 2.0f));
		for (int x = 0; x < data.mBitmap.getWidth(); ++x)
		{
			float colorR = (float)((*pixelPtr) & 0xff);
			float colorG = (float)((*pixelPtr >> 8) & 0xff);
			float colorB = (float)((*pixelPtr >> 16) & 0xff);
			colorR *= shadingFactor;
			colorG *= shadingFactor;
			colorB *= shadingFactor;
			*pixelPtr = (uint32)(colorR + 0.5f) | ((uint32)(colorG + 0.5f) << 8) | ((uint32)(colorB + 0.5f) << 16) | (*pixelPtr & 0xff000000);
			++pixelPtr;
		}
	}
}
