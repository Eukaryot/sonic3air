/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct SoftwareBlur
{
public:
	static void blurBitmap(Bitmap& bitmap, int blurValue)
	{
		switch (blurValue)
		{
			case 1:  blurBitmap<16>(bitmap);  break;
			case 2:  blurBitmap<32>(bitmap);  break;
			case 3:  blurBitmap<48>(bitmap);  break;
			case 4:  blurBitmap<64>(bitmap);  break;
			default: break;
		}
	}

private:
	// Template parameters are weights for each of the neighbor pixels, as a fraction of 256;
	//  the higher these values are, the stronger the blur gets
	template<int weightX, int weightY = weightX>
	static void blurBitmap(Bitmap& bitmap)
	{
		// Can't blur bitmaps that are too small
		if (bitmap.getWidth() < 2 || bitmap.getHeight() < 2)
			return;

		// Get weights for the central pixel
		constexpr int invWeightX = 256 - weightX * 2;
		constexpr int invWeightY = 256 - weightY * 2;

		mTemp.create(bitmap.getSize());

		// Pass 1: Blur in x-direction
		for (int y = 0; y < bitmap.getHeight(); ++y)
		{
			uint32* dst = mTemp.getPixelPointer(0, y);
			const uint32* src = bitmap.getPixelPointer(0, y);

			// Preparations & leftmost pixel
			uint32 src0 = src[0];
			uint32 src1 = src[0];
			uint32 src2 = src[1];
			dst[0] = mixColors<weightX, invWeightX>(src0, src1, src2);

			// Pixels in between
			const uint32* end = src + ((std::ptrdiff_t)bitmap.getWidth() - 2);
			while (src != end)
			{
				++src;
				++dst;
				src0 = src1;
				src1 = src2;
				src2 = src[1];
				dst[0] = mixColors<weightX, invWeightX>(src0, src1, src2);
			}

			// Rightmost pixel
			dst[1] = mixColors<weightX, invWeightX>(src1, src2, src2);
		}

		// Copy everything over
		bitmap = mTemp;

		// Pass 2: Blur in y-direction
		{
			// Topmost line
			mixLines<weightY, invWeightY>(bitmap, mTemp, 0, 0, 1);

			// Lines in between
			int y = 1;
			for (; y < bitmap.getHeight() - 1; ++y)
			{
				mixLines<weightY, invWeightY>(bitmap, mTemp, y-1, y, y+1);
			}

			// Bottommost line
			mixLines<weightY, invWeightY>(bitmap, mTemp, y-1, y, y);
		}
	}

private:
	template<int factor0, int factor1>
	static inline uint32 mixColors(uint32 src0, uint32 src1, uint32 src2)
	{
		return ((((src0 & 0xff00ff) * factor0 + (src1 & 0xff00ff) * factor1 + (src2 & 0xff00ff) * factor0) >> 8) & 0xff00ff)
			 + ((((src0 & 0x00ff00) * factor0 + (src1 & 0x00ff00) * factor1 + (src2 & 0x00ff00) * factor0) >> 8) & 0x00ff00);
	};

	template<int factor0, int factor1>
	static inline void mixLines(Bitmap& output, const Bitmap& input, int y0, int y1, int y2)
	{
		const uint32* src0 = input.getPixelPointer(0, y0);
		uint32* src1 = output.getPixelPointer(0, y1);
		const uint32* src2 = input.getPixelPointer(0, y2);
		
		uint32* end = src1 + input.getWidth();
		for (; src1 != end; ++src0, ++src1, ++src2)
		{
			*src1 = mixColors<factor0, factor1>(*src0, *src1, *src2);
		}
	}

private:
	static inline Bitmap mTemp;
};
