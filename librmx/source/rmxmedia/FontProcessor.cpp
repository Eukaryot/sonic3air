/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


void ShadowFontProcessor::process(FontProcessingData& data)
{
	if (mShadowAlpha < 0.001f)
		return;

	// Add shadow effect
	const int oldBorderLeft   = data.mBorderLeft;
	const int oldBorderRight  = data.mBorderRight;
	const int oldBorderTop    = data.mBorderTop;
	const int oldBorderBottom = data.mBorderBottom;

	const int border = clamp(roundToInt(mShadowBlur * 2.0f), 0, 16);
	const int offsX = clamp(roundToInt(mShadowOffset.x), 0, 16);
	const int offsY = clamp(roundToInt(mShadowOffset.y), 0, 16);

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
	if (mShadowAlpha < 1.0f)
	{
		uint32* data = bmp.getData();
		for (int i = 0; i < bmp.getPixelCount(); ++i)
		{
			data[i] = roundToInt((float)(data[i] >> 24) * mShadowAlpha) << 24;
		}
	}
	else
	{
		bmp.clearRGB(0);
	}
	bmp.insertBlend(insetX, insetY, data.mBitmap);
	data.mBitmap = bmp;
}


void OutlineFontProcessor::process(FontProcessingData& data)
{
	const int outlineWidth = 1;

	const int oldBorderLeft   = data.mBorderLeft;
	const int oldBorderRight  = data.mBorderRight;
	const int oldBorderTop    = data.mBorderTop;
	const int oldBorderBottom = data.mBorderBottom;

	data.mBorderLeft   = oldBorderLeft   + outlineWidth;
	data.mBorderRight  = oldBorderRight  + outlineWidth;
	data.mBorderTop    = oldBorderTop    + outlineWidth;
	data.mBorderBottom = oldBorderBottom + outlineWidth;

	const int newWidth  = data.mBitmap.getWidth()  + (data.mBorderLeft + data.mBorderRight) - (oldBorderLeft + oldBorderRight);
	const int newHeight = data.mBitmap.getHeight() + (data.mBorderTop + data.mBorderBottom) - (oldBorderTop + oldBorderBottom);
	const int insetX = data.mBorderLeft - oldBorderLeft;
	const int insetY = data.mBorderTop - oldBorderTop;

	Bitmap bitmap;
	bitmap.create(newWidth, newHeight, 0);

#if 0
	// TODO: This does not work correctly yet
	const uint32 outlineColorABGR = mOutlineColor.getABGR32();
	for (int y = 0; y < data.mBitmap.getHeight(); ++y)
	{
		for (int x = 0; x < data.mBitmap.getWidth(); ++x)
		{
			const int alpha = data.mBitmap.getPixel(x, y) >> 24;
			if (alpha > 0)
			{
				const uint32 color = (outlineColorABGR & 0x00ffffff) + (((outlineColorABGR >> 24) * alpha / 255) << 24);
				bitmap.setPixel(insetX + x - 1, insetY + y, std::max(color, bitmap.getPixel(insetX + x - 1, insetY + y)));
				bitmap.setPixel(insetX + x + 1, insetY + y, std::max(color, bitmap.getPixel(insetX + x + 1, insetY + y)));
				bitmap.setPixel(insetX + x, insetY + y - 1, std::max(color, bitmap.getPixel(insetX + x, insetY + y - 1)));
				bitmap.setPixel(insetX + x, insetY + y + 1, std::max(color, bitmap.getPixel(insetX + x, insetY + y + 1)));

				if (mDiagonalNeighbors)
				{
					bitmap.setPixel(insetX + x - 1, insetY + y - 1, std::max(color, bitmap.getPixel(insetX + x - 1, insetY + y - 1)));
					bitmap.setPixel(insetX + x - 1, insetY + y + 1, std::max(color, bitmap.getPixel(insetX + x - 1, insetY + y + 1)));
					bitmap.setPixel(insetX + x + 1, insetY + y - 1, std::max(color, bitmap.getPixel(insetX + x + 1, insetY + y - 1)));
					bitmap.setPixel(insetX + x + 1, insetY + y + 1, std::max(color, bitmap.getPixel(insetX + x + 1, insetY + y + 1)));
				}
			}
		}
	}
#else
	const uint32 outlineColorABGR = mOutlineColor.getABGR32();
	for (int y = 0; y < data.mBitmap.getHeight(); ++y)
	{
		for (int x = 0; x < data.mBitmap.getWidth(); ++x)
		{
			if (*data.mBitmap.getPixelPointer(x, y) & 0xff000000)
			{
				bitmap.setPixel(insetX + x - 1, insetY + y, outlineColorABGR);
				bitmap.setPixel(insetX + x + 1, insetY + y, outlineColorABGR);
				bitmap.setPixel(insetX + x, insetY + y - 1, outlineColorABGR);
				bitmap.setPixel(insetX + x, insetY + y + 1, outlineColorABGR);

				if (mDiagonalNeighbors)
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
