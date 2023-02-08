/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/drawing/software/Blitter.h"


struct BlitterHelper
{
	static inline void fillRect(const BitmapViewMutable<uint32>& view, Color color)
	{
		if (view.isEmpty())
			return;

		// Fill the first line, then copy it into the rest
		const uint32 rgba = color.getABGR32();
		uint32* firstLine = view.getLinePointer(0);
		for (int i = 0; i < view.getSize().x; ++i)
		{
			firstLine[i] = rgba;
		}
		for (int line = 1; line < view.getSize().y; ++line)
		{
			uint32* dst = view.getLinePointer(line);
			memcpy(dst, firstLine, (size_t)view.getSize().x * sizeof(uint32));
		}
	}

	static inline void blendRectAlpha(const BitmapViewMutable<uint32>& view, Color color)
	{
		if (view.isEmpty())
			return;

		uint16 additions[3] =
		{
			(uint16)roundToInt(color.r * color.a * 255.0f),
			(uint16)roundToInt(color.g * color.a * 255.0f),
			(uint16)roundToInt(color.b * color.a * 255.0f)
		};
		const uint16 multiplicator = (uint16)(256 - roundToInt(color.a * 256.0f));
		for (int line = 0; line < view.getSize().y; ++line)
		{
			uint8* dst = (uint8*)view.getLinePointer(line);
			for (int i = 0; i < view.getSize().x; ++i)
			{
				dst[0] = (uint8)(((dst[0] * multiplicator) >> 8) + additions[0]);
				dst[1] = (uint8)(((dst[1] * multiplicator) >> 8) + additions[1]);
				dst[2] = (uint8)(((dst[2] * multiplicator) >> 8) + additions[2]);
				dst += 4;
			}
		}
	}

	static inline void blendLineOpaque(uint32* dst, const uint32* src, size_t numPixels)
	{
		memcpy(dst, src, numPixels * sizeof(uint32));
	}

	static inline void blendLineOpaqueWithDepth(uint32* dst, const uint32* src, size_t numPixels, uint8* depthBuffer, uint8 depthValue)
	{
		for (size_t x = 0; x < numPixels; ++x)
		{
			if (depthValue >= *depthBuffer)
			{
				*dst = *src;
				*depthBuffer = depthValue;
			}
			++dst;
			++src;
			++depthBuffer;
		}
	}

	static inline void blendLineAlpha(uint32* dst_, const uint32* src_, size_t numPixels)
	{
		uint8* dst = (uint8*)dst_;
		const uint8* src = (const uint8*)src_;
		for (size_t x = 0; x < numPixels; ++x)
		{
			const int alpha = src[3];
			if (alpha > 0)
			{
				const int oneMinusAlpha = 0x100 - alpha;
				dst[0] = (uint8)(((int)src[0] * alpha + (int)dst[0] * oneMinusAlpha) >> 8);
				dst[1] = (uint8)(((int)src[1] * alpha + (int)dst[1] * oneMinusAlpha) >> 8);
				dst[2] = (uint8)(((int)src[2] * alpha + (int)dst[2] * oneMinusAlpha) >> 8);
			}
			dst += 4;
			src += 4;
		}
	}

	static inline void blendLineAlphaWithDepth(uint32* dst_, const uint32* src_, size_t numPixels, uint8* depthBuffer, uint8 depthValue)
	{
		uint8* dst = (uint8*)dst_;
		const uint8* src = (const uint8*)src_;
		for (size_t x = 0; x < numPixels; ++x)
		{
			if (depthValue >= *depthBuffer)
			{
				const int alpha = src[3];
				if (alpha > 0)
				{
					const int oneMinusAlpha = 0x100 - alpha;
					dst[0] = (uint8)(((int)src[0] * alpha + (int)dst[0] * oneMinusAlpha) >> 8);
					dst[1] = (uint8)(((int)src[1] * alpha + (int)dst[1] * oneMinusAlpha) >> 8);
					dst[2] = (uint8)(((int)src[2] * alpha + (int)dst[2] * oneMinusAlpha) >> 8);
					*depthBuffer = depthValue;
				}
			}
			dst += 4;
			src += 4;
			++depthBuffer;
		}
	}

	static inline void blendLineOneBit(uint32* dst, const uint32* src, size_t numPixels)
	{
		for (size_t x = 0; x < numPixels; ++x)
		{
			*dst = (*src & 0x80000000) ? (*src | 0xff000000) : *dst;
			++dst;
			++src;
		}
	}

	static inline void blendLineOneBitWithDepth(uint32* dst, const uint32* src, size_t numPixels, uint8* depthBuffer, uint8 depthValue)
	{
		for (size_t x = 0; x < numPixels; ++x)
		{
			if (depthValue >= *depthBuffer)
			{
				*dst = (*src & 0x80000000) ? (*src | 0xff000000) : *dst;
				*depthBuffer = depthValue;
			}
			++dst;
			++src;
			++depthBuffer;
		}
	}

	static inline void blendLineAdditive(uint32* dst_, const uint32* src_, size_t numPixels)
	{
		uint8* dst = (uint8*)dst_;
		const uint8* src = (const uint8*)src_;
		for (size_t x = 0; x < numPixels; ++x)
		{
			dst[0] = std::min(src[0] + dst[0], 0xff);
			dst[1] = std::min(src[1] + dst[1], 0xff);
			dst[2] = std::min(src[2] + dst[2], 0xff);
			dst += 4;
			src += 4;
		}
	}

	static inline void blendLineAdditiveWithDepth(uint32* dst_, const uint32* src_, size_t numPixels, uint8* depthBuffer, uint8 depthValue)
	{
		uint8* dst = (uint8*)dst_;
		const uint8* src = (const uint8*)src_;
		for (size_t x = 0; x < numPixels; ++x)
		{
			if (depthValue >= *depthBuffer)
			{
				dst[0] = std::min(src[0] + dst[0], 0xff);
				dst[1] = std::min(src[1] + dst[1], 0xff);
				dst[2] = std::min(src[2] + dst[2], 0xff);
				*depthBuffer = depthValue;
			}
			dst += 4;
			src += 4;
			++depthBuffer;
		}
	}

	static inline void blendLineMultiplicative(uint32* dst_, const uint32* src_, size_t numPixels)
	{
		uint8* dst = (uint8*)dst_;
		const uint8* src = (const uint8*)src_;
		for (size_t x = 0; x < numPixels; ++x)
		{
			dst[0] = (src[0] * dst[0]) / 255;
			dst[1] = (src[1] * dst[1]) / 255;
			dst[2] = (src[2] * dst[2]) / 255;
			dst += 4;
			src += 4;
		}
	}

	static inline void blendLineMultiplicativeWithDepth(uint32* dst_, const uint32* src_, size_t numPixels, uint8* depthBuffer, uint8 depthValue)
	{
		uint8* dst = (uint8*)dst_;
		const uint8* src = (const uint8*)src_;
		for (size_t x = 0; x < numPixels; ++x)
		{
			if (depthValue >= *depthBuffer)
			{
				dst[0] = (src[0] * dst[0]) / 255;
				dst[1] = (src[1] * dst[1]) / 255;
				dst[2] = (src[2] * dst[2]) / 255;
				*depthBuffer = depthValue;
			}
			dst += 4;
			src += 4;
			++depthBuffer;
		}
	}

	static inline uint32 pointSampling(const BitmapView<uint32>& bitmap, int px, int py)
	{
		if (px >= 0 && px < bitmap.getSize().x && py >= 0 && py < bitmap.getSize().y)
		{
			return bitmap.getPixel(px, py);
		}
		else
		{
			return 0;
		}
	}

	static inline uint32 pointSampling(const BitmapView<uint8>& bitmap, const Blitter::PaletteWrapper& palette, int px, int py)
	{
		if (px >= 0 && px < bitmap.getSize().x && py >= 0 && py < bitmap.getSize().y)
		{
			const uint8 index = bitmap.getPixel(px, py);
			return (index < palette.mNumEntries) ? palette.mPalette[index] : 0;
		}
		else
		{
			return 0;
		}
	}

	static inline int bilinearEval(int pixel00, int pixel01, int pixel10, int pixel11, float fx, float fy)
	{
		const float blended = (((float)(pixel00 & 0xff) * (1.0f - fx) + (float)(pixel10 & 0xff) * fx) * (1.0f - fy)
							+ ((float)(pixel01 & 0xff) * (1.0f - fx) + (float)(pixel11 & 0xff) * fx) * fy);
		return roundToInt(blended);
	}

	static inline uint32 bilinearSampling(const BitmapView<uint32>& bitmap, float px, float py)
	{
		const int ix = (int)std::floor(px);
		const int iy = (int)std::floor(py);
		const float fx = px - (float)ix;
		const float fy = py - (float)iy;
		const uint32 pixel00 = pointSampling(bitmap, ix, iy);
		const uint32 pixel01 = pointSampling(bitmap, ix, iy+1);
		const uint32 pixel10 = pointSampling(bitmap, ix+1, iy);
		const uint32 pixel11 = pointSampling(bitmap, ix+1, iy+1);
		const int r = bilinearEval(pixel00,       pixel01,       pixel10,       pixel11,       fx, fy);
		const int g = bilinearEval(pixel00 >> 8,  pixel01 >> 8,  pixel10 >> 8,  pixel11 >> 8,  fx, fy);
		const int b = bilinearEval(pixel00 >> 16, pixel01 >> 16, pixel10 >> 16, pixel11 >> 16, fx, fy);
		const int a = bilinearEval(pixel00 >> 24, pixel01 >> 24, pixel10 >> 24, pixel11 >> 24, fx, fy);
		return r + (g << 8) + (b << 16) + (a << 24);
	}

	static inline uint32 bilinearSampling(const BitmapView<uint8>& bitmap, const Blitter::PaletteWrapper& palette, float px, float py)
	{
		const int ix = (int)std::floor(px);
		const int iy = (int)std::floor(py);
		const float fx = px - (float)ix;
		const float fy = py - (float)iy;
		const uint32 pixel00 = pointSampling(bitmap, palette, ix, iy);
		const uint32 pixel01 = pointSampling(bitmap, palette, ix, iy+1);
		const uint32 pixel10 = pointSampling(bitmap, palette, ix+1, iy);
		const uint32 pixel11 = pointSampling(bitmap, palette, ix+1, iy+1);
		const int r = bilinearEval(pixel00,       pixel01,       pixel10,       pixel11,       fx, fy);
		const int g = bilinearEval(pixel00 >> 8,  pixel01 >> 8,  pixel10 >> 8,  pixel11 >> 8,  fx, fy);
		const int b = bilinearEval(pixel00 >> 16, pixel01 >> 16, pixel10 >> 16, pixel11 >> 16, fx, fy);
		const int a = bilinearEval(pixel00 >> 24, pixel01 >> 24, pixel10 >> 24, pixel11 >> 24, fx, fy);
		return r + (g << 8) + (b << 16) + (a << 24);
	}

	static void mergeIntoOutputDirect(const BitmapViewMutable<uint32>& output, const BitmapView<uint32>& input, const Blitter::Options& options)
	{
		switch (options.mBlendMode)
		{
			case BlendMode::ALPHA:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineAlpha(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x);
				break;
			}

			case BlendMode::ONE_BIT:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineOneBit(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x);
				break;
			}

			case BlendMode::ADDITIVE:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineAdditive(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x);
				break;
			}

			case BlendMode::MULTIPLICATIVE:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineMultiplicative(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x);
				break;
			}

			default:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineOpaque(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x);
				break;
			}
		}
	}

	static void mergeIntoOutputWithDepth(const BitmapViewMutable<uint32>& output, const BitmapView<uint32>& input, BitmapViewMutable<uint8>& depthBuffer, const Blitter::Options& options)
	{
		switch (options.mBlendMode)
		{
			case BlendMode::ALPHA:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineAlphaWithDepth(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x, depthBuffer.getLinePointer(y), options.mDepthValue);
				break;
			}

			case BlendMode::ONE_BIT:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineOneBitWithDepth(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x, depthBuffer.getLinePointer(y), options.mDepthValue);
				break;
			}

			case BlendMode::ADDITIVE:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineAdditiveWithDepth(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x, depthBuffer.getLinePointer(y), options.mDepthValue);
				break;
			}

			case BlendMode::MULTIPLICATIVE:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineMultiplicativeWithDepth(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x, depthBuffer.getLinePointer(y), options.mDepthValue);
				break;
			}

			default:
			{
				for (int y = 0; y < input.getSize().y; ++y)
					blendLineOpaqueWithDepth(output.getLinePointer(y), input.getLinePointer(y), input.getSize().x, depthBuffer.getLinePointer(y), options.mDepthValue);
				break;
			}
		}
	}

	static void mergeIntoOutput(const Blitter::OutputWrapper& output, const Recti& outputBoundingBox, const BitmapView<uint32>& intermediate, const Blitter::Options& options)
	{
		BitmapViewMutable<uint32> outputView(output.mBitmapView, Recti(outputBoundingBox.getPos(), intermediate.getSize()));
		if (nullptr == options.mDepthBuffer)
		{
			mergeIntoOutputDirect(outputView, intermediate, options);
		}
		else
		{
			//RMX_ASSERT(options.mDepthBuffer->getSize() == output.mBitmapView.getSize(), "Depth buffer size differs from output bitmap size");
			BitmapViewMutable<uint8> depthBuffer(*options.mDepthBuffer, Recti(outputBoundingBox.getPos(), intermediate.getSize()));
			mergeIntoOutputWithDepth(outputView, intermediate, depthBuffer, options);
		}
	}
};
