/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/software/Blitter.h"


namespace blitterinternal
{
	const constexpr bool IS_LITTLE_ENDIAN = true;	// TODO: Use correct endianness

	inline uint32 swapRedBlue(uint32 rgba)
	{
		return ((rgba & 0x00ff0000) >> 16) | (rgba & 0xff00ff00) | ((rgba & 0x000000ff) << 16);
	}

	template<bool SWAP_RED_BLUE>
	inline uint32 convertPixel(uint32 rgba)
	{
		return rgba;
	}

	template<>
	inline uint32 convertPixel<true>(uint32 rgba)
	{
		return swapRedBlue(rgba);
	}

	inline uint32 multiplyColors(uint32 color1, uint32 color2)
	{
		uint32 result = 0;
		const uint8* src1 = (uint8*)&color1;
		const uint8* src2 = (uint8*)&color2;
		uint8* dst = (uint8*)&result;
		dst[0] = (uint8)((int)src1[0] * (int)src2[0] / 255);
		dst[1] = (uint8)((int)src1[1] * (int)src2[1] / 255);
		dst[2] = (uint8)((int)src1[2] * (int)src2[2] / 255);
		dst[3] = (uint8)((int)src1[3] * (int)src2[3] / 255);
		return result;
	}

	template<bool SWAP_RED_BLUE>
	inline void blendColors(uint8* dst, const uint8* src)
	{
		const uint16 alpha = src[3];
		dst[0] = (src[0] * alpha + dst[0] * (255 - alpha)) / 255;
		dst[1] = (src[1] * alpha + dst[1] * (255 - alpha)) / 255;
		dst[2] = (src[2] * alpha + dst[2] * (255 - alpha)) / 255;
		dst[3] |= src[3];	// Assume at least one of both is 0xff, or both are 0
	}

	template<>
	inline void blendColors<true>(uint8* dst, const uint8* src)
	{
		const uint16 alpha = src[3];
		dst[0] = (src[2] * alpha + dst[0] * (255 - alpha)) / 255;
		dst[1] = (src[1] * alpha + dst[1] * (255 - alpha)) / 255;
		dst[2] = (src[0] * alpha + dst[2] * (255 - alpha)) / 255;
		dst[3] |= src[3];	// Assume at least one of both is 0xff, or both are 0
	}

	template<bool ENDIANNESS>
	inline uint16& getFixedPoint1616_Int(uint32& value)
	{
		return *((uint16*)&value);		// For big endian
	}

	template<>
	inline uint16& getFixedPoint1616_Int<true>(uint32& value)
	{
		return *((uint16*)&value + 1);	// For little endian
	}


	template<bool SWAP_RED_BLUE>
	void blitColorWithBlending(BitmapWrapper& destBitmap, Recti destRect, uint16 multiplicator, const uint16* additions)
	{
		for (int line = 0; line < destRect.height; ++line)
		{
			uint8* dst = (uint8*)destBitmap.getPixelPointer(destRect.x, destRect.y + line);
			for (int i = 0; i < destRect.width; ++i)
			{
				dst[0] = ((dst[0] * multiplicator) >> 8) + additions[0];
				dst[1] = ((dst[1] * multiplicator) >> 8) + additions[1];
				dst[2] = ((dst[2] * multiplicator) >> 8) + additions[2];
				dst[3] = 0xff;
				dst += 4;
			}
		}
	}

	template<bool SWAP_RED_BLUE, bool USE_TINT_COLOR>
	void blitBitmap(BitmapWrapper& destBitmap, Vec2i destPosition, const BitmapWrapper& sourceBitmap, Recti sourceRect, uint32 tintColor)
	{
		uint32* srcBase = sourceBitmap.getPixelPointer(sourceRect.x, sourceRect.y);
		uint32* dstBase = destBitmap.getPixelPointer(destPosition.x, destPosition.y);

		for (int y = 0; y < sourceRect.height; ++y)
		{
			uint8* src = (uint8*)(&srcBase[y * sourceBitmap.mSize.x]);
			uint8* dst = (uint8*)(&dstBase[y * destBitmap.mSize.x]);

			if (SWAP_RED_BLUE)
			{
				if (USE_TINT_COLOR)
				{
					for (int x = 0; x < sourceRect.width; ++x)
					{
						*dst = convertPixel<SWAP_RED_BLUE>(multiplyColors(*(uint32*)src, tintColor));
						src += 4;
						dst += 4;
					}
				}
				else
				{
					for (int x = 0; x < sourceRect.width; ++x)
					{
						*dst = convertPixel<SWAP_RED_BLUE>(*src);
						src += 4;
						dst += 4;
					}
				}
			}
			else
			{
				if (USE_TINT_COLOR)
				{
					for (int x = 0; x < sourceRect.width; ++x)
					{
						*dst = multiplyColors(*(uint32*)src, tintColor);
						src += 4;
						dst += 4;
					}
				}
				else
				{
					memcpy(dst, src, sourceRect.width * 4);
				}
			}
		}
	}

	template<bool SWAP_RED_BLUE, bool USE_TINT_COLOR>
	void blitBitmapWithBlending(BitmapWrapper& destBitmap, Vec2i destPosition, const BitmapWrapper& sourceBitmap, Recti sourceRect, uint32 tintColor)
	{
		uint32* srcBase = sourceBitmap.getPixelPointer(sourceRect.x, sourceRect.y);
		uint32* dstBase = destBitmap.getPixelPointer(destPosition.x, destPosition.y);

		for (int y = 0; y < sourceRect.height; ++y)
		{
			uint8* src = (uint8*)(&srcBase[y * sourceBitmap.mSize.x]);
			uint8* dst = (uint8*)(&dstBase[y * destBitmap.mSize.x]);

			if (USE_TINT_COLOR)
			{
				for (int x = 0; x < sourceRect.width; ++x)
				{
					uint32 tinted = multiplyColors(*(uint32*)src, tintColor);
					blendColors<SWAP_RED_BLUE>(dst, (uint8*)&tinted);
					src += 4;
					dst += 4;
				}
			}
			else
			{
				for (int x = 0; x < sourceRect.width; ++x)
				{
					blendColors<SWAP_RED_BLUE>(dst, src);
					src += 4;
					dst += 4;
				}
			}
		}
	}

	template<bool SWAP_RED_BLUE, bool ALPHA_BLENDING, bool USE_TINT_COLOR>
	void blitBitmapWithScaling(BitmapWrapper& destBitmap, Recti destRect, const BitmapWrapper& sourceBitmap, Recti sourceRect, uint32 tintColor)
	{
		if (destBitmap.empty())
			return;

		int lastSourceY = -1;
		uint32* lastDestData = nullptr;

		uint32 positionExact = 0;	// This is used as a 16.16 fixed point number
		uint16& position = getFixedPoint1616_Int<IS_LITTLE_ENDIAN>(positionExact);
		const uint32 advance = (sourceRect.width << 16) / destRect.width;

		for (int lineIndex = 0; lineIndex < destRect.height; ++lineIndex)
		{
			const int destY = destRect.y + lineIndex;
			const int sourceY = sourceRect.y + lineIndex * sourceRect.height / destRect.height;
			uint32* destData = destBitmap.getPixelPointer(destRect.x, destY);

			if (sourceY == lastSourceY)
			{
				// Just copy the content from the last line, as it's the contents again
				memcpy(destData, lastDestData, destRect.width * 4);
			}
			else
			{
				const uint32* sourceData = sourceBitmap.getPixelPointer(sourceRect.x, sourceY);
				positionExact = 0;

				if (SWAP_RED_BLUE || USE_TINT_COLOR)
				{
					const constexpr int BUFFER_SIZE = 2048;
					RMX_ASSERT(sourceRect.width <= BUFFER_SIZE, "Buffer supports only widths of " << BUFFER_SIZE << " pixels at maximum");
					sourceRect.width = std::min(sourceRect.width, BUFFER_SIZE);
					uint32 buffer[BUFFER_SIZE];
					if (SWAP_RED_BLUE)
					{
						if (USE_TINT_COLOR)
						{
							for (int x = 0; x < sourceRect.width; ++x)
							{
								buffer[x] = convertPixel<SWAP_RED_BLUE>(multiplyColors(sourceData[x], tintColor));
							}
						}
						else
						{
							for (int x = 0; x < sourceRect.width; ++x)
							{
								buffer[x] = convertPixel<SWAP_RED_BLUE>(sourceData[x]);
							}
						}
					}
					else
					{
						if (USE_TINT_COLOR)
						{
							for (int x = 0; x < sourceRect.width; ++x)
							{
								buffer[x] = multiplyColors(sourceData[x], tintColor);
							}
						}
						else
						{
							memcpy(buffer, sourceData, sourceRect.width * 4);
						}
					}

					if (ALPHA_BLENDING)
					{
						for (int destX = 0; destX < destRect.width; ++destX)
						{
							blendColors<false>((uint8*)&destData[destX], (uint8*)&buffer[position]);
							positionExact += advance;
						}
					}
					else
					{
						for (int destX = 0; destX < destRect.width; ++destX)
						{
							destData[destX] = buffer[position];
							positionExact += advance;
						}
					}
				}
				else
				{
					if (ALPHA_BLENDING)
					{
						for (int destX = 0; destX < destRect.width; ++destX)
						{
							blendColors<false>((uint8*)&destData[destX], (uint8*)&sourceData[position]);
							positionExact += advance;
						}
					}
					else
					{
						for (int destX = 0; destX < destRect.width; ++destX)
						{
							destData[destX] = sourceData[position];
							positionExact += advance;
						}
					}
				}

				lastSourceY = sourceY;
				lastDestData = destData;
			}
		}
	}

	bool cropBlitRect(Vec2i& destPosition, Recti& sourceRect, const Vec2i& destSize, const Vec2i& sourceSize)
	{
		int sx = sourceRect.width;
		int sy = sourceRect.height;

		if (sourceRect.x < 0)
		{
			sx += sourceRect.x;
			destPosition.x -= sourceRect.x;
			sourceRect.x = 0;
		}
		if (sourceRect.y < 0)
		{
			sy += sourceRect.y;
			destPosition.y -= sourceRect.y;
			sourceRect.y = 0;
		}

		if (destPosition.x < 0)
		{
			sx += destPosition.x;
			sourceRect.x -= destPosition.x;
			destPosition.x = 0;
		}
		if (destPosition.y < 0)
		{
			sy += destPosition.y;
			sourceRect.y -= destPosition.y;
			destPosition.y = 0;
		}

		sx = std::min(sx, sourceSize.x - sourceRect.x);
		sy = std::min(sy, sourceSize.y - sourceRect.y);
		sx = std::min(sx, destSize.x - destPosition.x);
		sy = std::min(sy, destSize.y - destPosition.y);

		if (sx <= 0 || sy <= 0)
			return false;

		sourceRect.width = sx;
		sourceRect.height = sy;
		return true;
	}
}



void Blitter::blitColor(BitmapWrapper& destBitmap, Recti destRect, const Color& color, const Options& options)
{
	destRect.intersect(Recti(0, 0, destBitmap.mSize.x, destBitmap.mSize.y));
	if (destRect.empty())
		return;

	if (!options.mUseAlphaBlending || color.a >= 1.0f)
	{
		// No blending
		uint32 rgba = color.getABGR32();
		if (options.mSwapRedBlue)
			rgba = blitterinternal::swapRedBlue(rgba);

		uint32* firstLine = destBitmap.getPixelPointer(destRect.x, destRect.y);
		for (int i = 0; i < destRect.width; ++i)
		{
			firstLine[i] = rgba;
		}
		for (int line = 1; line < destRect.height; ++line)
		{
			uint32* dst = destBitmap.getPixelPointer(destRect.x, destRect.y + line);
			memcpy(dst, firstLine, (size_t)destRect.width * 4);
		}
	}
	else if (color.a > 0.0f)
	{
		// Alpha blending
		uint16 additions[3] =
		{
			(uint16)roundToInt(color.r * color.a * 255.0f),
			(uint16)roundToInt(color.g * color.a * 255.0f),
			(uint16)roundToInt(color.b * color.a * 255.0f)
		};
		if (options.mSwapRedBlue)
		{
			std::swap(additions[0], additions[2]);
		}
		const uint16 multiplicator = (uint16)(256 - roundToInt(color.a * 256.0f));

		for (int line = 0; line < destRect.height; ++line)
		{
			uint8* dst = (uint8*)destBitmap.getPixelPointer(destRect.x, destRect.y + line);
			for (int i = 0; i < destRect.width; ++i)
			{
				dst[0] = ((dst[0] * multiplicator) >> 8) + additions[0];
				dst[1] = ((dst[1] * multiplicator) >> 8) + additions[1];
				dst[2] = ((dst[2] * multiplicator) >> 8) + additions[2];
				dst[3] = 0xff;
				dst += 4;
			}
		}
	}
}

void Blitter::blitBitmap(BitmapWrapper& destBitmap, Vec2i destPosition, const BitmapWrapper& sourceBitmap, Recti sourceRect, const Options& options)
{
	if (!blitterinternal::cropBlitRect(destPosition, sourceRect, destBitmap.mSize, sourceBitmap.mSize))
		return;

	if (options.mTintColor == Color::WHITE)
	{
		if (!options.mUseAlphaBlending)
		{
			// No blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmap<true, false>(destBitmap, destPosition, sourceBitmap, sourceRect, 0xffffffff);
			}
			else
			{
				blitterinternal::blitBitmap<false, false>(destBitmap, destPosition, sourceBitmap, sourceRect, 0xffffffff);
			}
		}
		else
		{
			// Alpha blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmapWithBlending<true, false>(destBitmap, destPosition, sourceBitmap, sourceRect, 0xffffffff);
			}
			else
			{
				blitterinternal::blitBitmapWithBlending<false, false>(destBitmap, destPosition, sourceBitmap, sourceRect, 0xffffffff);
			}
		}
	}
	else
	{
		if (!options.mUseAlphaBlending)
		{
			// No blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmap<true, true>(destBitmap, destPosition, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
			else
			{
				blitterinternal::blitBitmap<false, true>(destBitmap, destPosition, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
		}
		else
		{
			// Alpha blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmapWithBlending<true, true>(destBitmap, destPosition, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
			else
			{
				blitterinternal::blitBitmapWithBlending<false, true>(destBitmap, destPosition, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
		}
	}
}

void Blitter::blitBitmapWithScaling(BitmapWrapper& destBitmap, Recti destRect, const BitmapWrapper& sourceBitmap, Recti sourceRect, const Options& options)
{
	if (destBitmap.empty())
		return;

	if (options.mTintColor == Color::WHITE)
	{
		if (!options.mUseAlphaBlending)
		{
			// No blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmapWithScaling<true, false, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
			}
			else
			{
				blitterinternal::blitBitmapWithScaling<false, false, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
			}
		}
		else
		{
			// Alpha blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmapWithScaling<true, true, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
			}
			else
			{
				blitterinternal::blitBitmapWithScaling<false, true, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
			}
		}
	}
	else
	{
		if (!options.mUseAlphaBlending)
		{
			// No blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmapWithScaling<true, false, true>(destBitmap, destRect, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
			else
			{
				blitterinternal::blitBitmapWithScaling<false, false, true>(destBitmap, destRect, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
		}
		else
		{
			// Alpha blending
			if (options.mSwapRedBlue)
			{
				blitterinternal::blitBitmapWithScaling<true, true, true>(destBitmap, destRect, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
			else
			{
				blitterinternal::blitBitmapWithScaling<false, true, true>(destBitmap, destRect, sourceBitmap, sourceRect, options.mTintColor.getABGR32());
			}
		}
	}
}
