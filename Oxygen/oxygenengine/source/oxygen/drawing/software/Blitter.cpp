/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/software/Blitter.h"
#include "oxygen/drawing/software/BlitterHelper.h"


namespace
{
	FORCE_INLINE int roundToIntFast(float x)
	{
		// Avoiding the std::floor in rmxbase's roundToInt, this is a bit cheaper
		return (int)(x + ((x >= 0.0f) ? 0.5f : -0.5f));
	}

	void getTransformedLineRange(int& minX, int& maxX, int iy, int width, Vec2i offset, Recti spriteRect, const float* transform)
	{
		// Output pixels to render in the given line
		//  -> Output minX is included, maxX is excluded
		const float dy = (float)(iy - offset.y) + 0.5f;
		{
			int x1 = width;
			int x2 = 0;
			if (std::abs(transform[0]) > 0.001f)
			{
				const float A = -dy * transform[1] + spriteRect.x;
				x1 = offset.x + roundToIntFast((A) / transform[0]);
				x2 = offset.x + roundToIntFast((A + spriteRect.width) / transform[0]);
			}
			minX = std::min(x1, x2);
			maxX = std::max(x1, x2);
		}
		{
			int x1 = width;
			int x2 = 0;
			if (std::abs(transform[2]) > 0.001f)
			{
				const float A = -dy * transform[3] + spriteRect.y;
				x1 = offset.x + roundToIntFast((A) / transform[2]);
				x2 = offset.x + roundToIntFast((A + spriteRect.height) / transform[2]);
			}
			const int minX2 = std::min(x1, x2);
			const int maxX2 = std::max(x1, x2);
			minX = std::max(minX, minX2);
			maxX = std::min(maxX, maxX2);
		}
		minX = clamp(minX, 0, width);
		maxX = clamp(maxX, 0, width);
	}
}


void Blitter::blitColor(const OutputWrapper& output, const Color& color, BlendMode blendMode)
{
	BitmapViewMutable<uint32> outputView(output.mBitmapView, output.mViewportRect);
	if (outputView.isEmpty())
		return;

	// TODO: Support other blend modes
	if (blendMode == BlendMode::OPAQUE || color.a >= 1.0f)
	{
		// No blending, just filling
		BlitterHelper::fillRect(outputView, color);
	}
	else if (color.a > 0.0f)
	{
		// Alpha blending
		BlitterHelper::blendRectAlpha(outputView, color);
	}
}

void Blitter::blitSprite(const OutputWrapper& output, const SpriteWrapper& sprite, Vec2i position, Options& options)
{
	const Recti outputBoundingBox = applyCropping(mPixelSegments, output.mViewportRect, Recti(-sprite.mPivot, sprite.mBitmapView.getSize()), position, options);
	if (mPixelSegments.empty())
		return;

	// TODO: Use "mPixelSegments" in more of the calculations below

	if (nullptr == options.mTransform)
	{
		const Vec2i innerIndent = outputBoundingBox.getPos() - position + sprite.mPivot;

		// As an optimization, differentiate between whether intermediate processing is needed (and we thus need to make an actual copy of the sprite data) or not
		if (needsIntermediateProcessing(options))
		{
			// Copy only the actually used part from the input sprite into an intermediate bitmap
			BitmapViewMutable<uint32> intermediate = makeTempBitmapAsCopy(sprite.mBitmapView, outputBoundingBox.getSize(), innerIndent);

			// Intermediate processing (like tint color / added color)
			processIntermediateBitmap(intermediate, options);

			// Merge into output (incl. blending and depth test)
			BlitterHelper::mergeIntoOutput(output, outputBoundingBox, intermediate, mPixelSegments, options);
		}
		else
		{
			// We're not going to make changes to the intermediate bitmap, so just use the sprite data directly
			BitmapView<uint32> intermediate(sprite.mBitmapView, Recti(innerIndent, outputBoundingBox.getSize()));

			// Merge into output (incl. blending and depth test)
			BlitterHelper::mergeIntoOutput(output, outputBoundingBox, intermediate, mPixelSegments, options);
		}
	}
	else
	{
		// TODO: Add optimizations for "simple" transformations, especially flips
		BitmapViewMutable<uint32> intermediate = makeTempBitmapAsTransformedCopy(outputBoundingBox, sprite, position, options);

		// Intermediate processing (like tint color / added color)
		processIntermediateBitmap(intermediate, options);

		// Merge into output (incl. blending and depth test)
		BlitterHelper::mergeIntoOutput(output, outputBoundingBox, intermediate, mPixelSegments, options);
	}
}

void Blitter::blitIndexed(const OutputWrapper& output, const IndexedSpriteWrapper& sprite, const PaletteWrapper& palette, Vec2i position, Options& options)
{
	const Recti outputBoundingBox = applyCropping(mPixelSegments, output.mViewportRect, Recti(-sprite.mPivot, sprite.mBitmapView.getSize()), position, options);
	if (mPixelSegments.empty())
		return;

	// TODO: Use "mPixelSegments" in more of the calculations below

	if (nullptr == options.mTransform)
	{
		const Vec2i innerIndent = outputBoundingBox.getPos() - position + sprite.mPivot;

		// Copy only the actually used part from the input sprite into an intermediate bitmap
		BitmapViewMutable<uint32> intermediate = makeTempBitmapAsCopy(sprite.mBitmapView, palette, outputBoundingBox.getSize(), innerIndent);

		// Intermediate processing (like tint color / added color)
		processIntermediateBitmap(intermediate, options);

		// Merge into output (incl. blending and depth test)
		BlitterHelper::mergeIntoOutput(output, outputBoundingBox, intermediate, mPixelSegments, options);
	}
	else
	{
		// TODO: Add optimizations for "simple" transformations, especially flips
		BitmapViewMutable<uint32> intermediate = makeTempBitmapAsTransformedCopy(outputBoundingBox, sprite, palette, position, options);

		// Intermediate processing (like tint color / added color)
		processIntermediateBitmap(intermediate, options);

		// Merge into output (incl. blending and depth test)
		BlitterHelper::mergeIntoOutput(output, outputBoundingBox, intermediate, mPixelSegments, options);
	}
}

void Blitter::blitRectWithScaling(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options)
{
	if (destBitmap.isEmpty() || sourceRect.isEmpty())
		return;

	if (nullptr == options.mTintColor)
	{
		if (options.mBlendMode != BlendMode::ALPHA)
		{
			// No blending
			BlitterHelper::blitBitmapWithScaling<false, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
		}
		else
		{
			// Alpha blending
			BlitterHelper::blitBitmapWithScaling<true, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
		}
	}
	else
	{
		if (options.mBlendMode != BlendMode::ALPHA)
		{
			// No blending
			BlitterHelper::blitBitmapWithScaling<false, true>(destBitmap, destRect, sourceBitmap, sourceRect, Color(*options.mTintColor).getABGR32());
		}
		else
		{
			// Alpha blending
			BlitterHelper::blitBitmapWithScaling<true, true>(destBitmap, destRect, sourceBitmap, sourceRect, Color(*options.mTintColor).getABGR32());
		}
	}
}

void Blitter::blitRectWithUVs(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options)
{
	if (destBitmap.isEmpty() || sourceRect.isEmpty())
		return;

	if (nullptr == options.mTintColor)
	{
		if (options.mBlendMode != BlendMode::ALPHA)
		{
			// No blending
			BlitterHelper::blitBitmapWithUVs<false, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
		}
		else
		{
			// Alpha blending
			BlitterHelper::blitBitmapWithUVs<true, false>(destBitmap, destRect, sourceBitmap, sourceRect, 0xffffffff);
		}
	}
	else
	{
		if (options.mBlendMode != BlendMode::ALPHA)
		{
			// No blending
			BlitterHelper::blitBitmapWithUVs<false, true>(destBitmap, destRect, sourceBitmap, sourceRect, Color(*options.mTintColor).getABGR32());
		}
		else
		{
			// Alpha blending
			BlitterHelper::blitBitmapWithUVs<true, true>(destBitmap, destRect, sourceBitmap, sourceRect, Color(*options.mTintColor).getABGR32());
		}
	}
}


BitmapViewMutable<uint32> Blitter::makeTempBitmap(Vec2i size)
{
	mTempBitmapData.resize(size.x * size.y);
	return BitmapViewMutable<uint32>(&mTempBitmapData[0], size);
}

BitmapViewMutable<uint32> Blitter::makeTempBitmapAsCopy(const BitmapView<uint32>& input, Vec2i size, Vec2i innerIndent)
{
	BitmapViewMutable<uint32> result = makeTempBitmap(size);
	for (int y = 0; y < size.y; ++y)
	{
		memcpy(result.getLinePointer(y), input.getPixelPointer(innerIndent.x, innerIndent.y + y), size.x * sizeof(uint32));
	}
	return result;
}

BitmapViewMutable<uint32> Blitter::makeTempBitmapAsCopy(const BitmapView<uint8>& input, const PaletteWrapper& palette, Vec2i size, Vec2i innerIndent)
{
	BitmapViewMutable<uint32> result = makeTempBitmap(size);
	for (int y = 0; y < size.y; ++y)
	{
		uint32* dst = result.getLinePointer(y);
		const uint8* src = input.getPixelPointer(innerIndent.x, innerIndent.y + y);
		for (int x = 0; x < size.x; ++x)
		{
			const uint8 index = *src;
			*dst = (index < palette.mNumEntries) ? palette.mPalette[index] : 0;
			++dst;
			++src;
		}
	}
	return result;
}

BitmapViewMutable<uint32> Blitter::makeTempBitmapAsTransformedCopy(Recti outputBoundingBox, const SpriteWrapper& sprite, Vec2i position, const Options& options)
{
	BitmapViewMutable<uint32> result = makeTempBitmap(outputBoundingBox.getSize());
	const Vec2f floatPivot(sprite.mPivot);
	switch (options.mSamplingMode)
	{
		case SamplingMode::POINT:
		{
			for (int iy = 0; iy < outputBoundingBox.height; ++iy)
			{
				// Transform into sprite-local coordinates
				const float dx = (float)(outputBoundingBox.x - position.x) + 0.5f;
				const float dy = (float)(outputBoundingBox.y - position.y + iy) + 0.5f;
				float localX = dx * options.mInvTransform[0] + dy * options.mInvTransform[1] + floatPivot.x;
				float localY = dx * options.mInvTransform[2] + dy * options.mInvTransform[3] + floatPivot.y;
				const float advanceX = options.mInvTransform[0];
				const float advanceY = options.mInvTransform[2];

				uint32* dst = result.getPixelPointer(0, iy);
				for (int ix = 0; ix < outputBoundingBox.width; ++ix)
				{
					*dst = BlitterHelper::pointSampling(sprite.mBitmapView, (int)localX, (int)localY);
					++dst;
					localX += advanceX;
					localY += advanceY;
				}
			}
			break;
		}

		case SamplingMode::BILINEAR:
		{
			for (int iy = 0; iy < outputBoundingBox.height; ++iy)
			{
				// Transform into sprite-local coordinates
				const float dx = (float)(outputBoundingBox.x - position.x) + 0.5f;
				const float dy = (float)(outputBoundingBox.y - position.y + iy) + 0.5f;
				float localX = dx * options.mInvTransform[0] + dy * options.mInvTransform[1] + floatPivot.x - 0.5f;
				float localY = dx * options.mInvTransform[2] + dy * options.mInvTransform[3] + floatPivot.y - 0.5f;
				const float advanceX = options.mInvTransform[0];
				const float advanceY = options.mInvTransform[2];

				uint32* dst = result.getPixelPointer(0, iy);
				for (int ix = 0; ix < outputBoundingBox.width; ++ix)
				{
					*dst = BlitterHelper::bilinearSampling(sprite.mBitmapView, localX, localY);
					++dst;
					localX += advanceX;
					localY += advanceY;
				}
			}
			break;
		}
	}
	return result;
}

BitmapViewMutable<uint32> Blitter::makeTempBitmapAsTransformedCopy(Recti outputBoundingBox, const IndexedSpriteWrapper& sprite, const PaletteWrapper& palette, Vec2i position, const Options& options)
{
	BitmapViewMutable<uint32> result = makeTempBitmap(outputBoundingBox.getSize());
	const Vec2f floatPivot(sprite.mPivot);
	switch (options.mSamplingMode)
	{
		case SamplingMode::POINT:
		{
			for (int iy = 0; iy < outputBoundingBox.height; ++iy)
			{
				// Transform into sprite-local coordinates
				const float dx = (float)(outputBoundingBox.x - position.x) + 0.5f;
				const float dy = (float)(outputBoundingBox.y - position.y + iy) + 0.5f;
				float localX = dx * options.mInvTransform[0] + dy * options.mInvTransform[1] + floatPivot.x;
				float localY = dx * options.mInvTransform[2] + dy * options.mInvTransform[3] + floatPivot.y;
				const float advanceX = options.mInvTransform[0];
				const float advanceY = options.mInvTransform[2];

				uint32* dst = result.getPixelPointer(0, iy);
				for (int ix = 0; ix < outputBoundingBox.width; ++ix)
				{
					*dst = BlitterHelper::pointSampling(sprite.mBitmapView, palette, (int)localX, (int)localY);
					++dst;
					localX += advanceX;
					localY += advanceY;
				}
			}
			break;
		}

		case SamplingMode::BILINEAR:
		{
			for (int iy = 0; iy < outputBoundingBox.height; ++iy)
			{
				// Transform into sprite-local coordinates
				const float dx = (float)(outputBoundingBox.x - position.x) + 0.5f;
				const float dy = (float)(outputBoundingBox.y - position.y + iy) + 0.5f;
				float localX = dx * options.mInvTransform[0] + dy * options.mInvTransform[1] + floatPivot.x - 0.5f;
				float localY = dx * options.mInvTransform[2] + dy * options.mInvTransform[3] + floatPivot.y - 0.5f;
				const float advanceX = options.mInvTransform[0];
				const float advanceY = options.mInvTransform[2];

				uint32* dst = result.getPixelPointer(0, iy);
				for (int ix = 0; ix < outputBoundingBox.width; ++ix)
				{
					*dst = BlitterHelper::bilinearSampling(sprite.mBitmapView, palette, localX, localY);
					++dst;
					localX += advanceX;
					localY += advanceY;
				}
			}
			break;
		}
	}
	return result;
}

Recti Blitter::applyCropping(std::vector<PixelSegment>& outPixelSegments, const Recti& viewportRect, const Recti& spriteRect, const Vec2i& position, const Options& options)
{
	outPixelSegments.clear();
	if (spriteRect.isEmpty())
		return Recti();

	// First calculate the sprite's bounding box, taking into account the transformation and all inner rectangles
	Recti uncroppedBoundingBox;
	if (nullptr == options.mTransform)
	{
		uncroppedBoundingBox.setPos(position + spriteRect.getPos());
		uncroppedBoundingBox.setSize(spriteRect.getSize());
	}
	else
	{
		Vec2f min(1e10f, 1e10f);
		Vec2f max(-1e10f, -1e10f);
		const Vec2i size = spriteRect.getSize();
		const Vec2i corners[4] = { Vec2i(0, 0), Vec2i(size.x, 0), Vec2i(0, size.y), size };
		for (int i = 0; i < 4; ++i)
		{
			const Vec2f localCorner = Vec2f(corners[i] + spriteRect.getPos());
			const float screenCornerX = position.x + localCorner.x * options.mTransform[0] + localCorner.y * options.mTransform[1];
			const float screenCornerY = position.y + localCorner.x * options.mTransform[2] + localCorner.y * options.mTransform[3];
			min.x = std::min(screenCornerX, min.x);
			min.y = std::min(screenCornerY, min.y);
			max.x = std::max(screenCornerX, max.x);
			max.y = std::max(screenCornerY, max.y);
		}

		uncroppedBoundingBox.x = (int)min.x;
		uncroppedBoundingBox.y = (int)min.y;
		uncroppedBoundingBox.width  = (int)max.x + 1 - uncroppedBoundingBox.x;
		uncroppedBoundingBox.height = (int)max.y + 1 - uncroppedBoundingBox.y;
	}

	// Get the (cropped) bounding box in the output viewport
	const Recti boundingBox = Recti::getIntersection(uncroppedBoundingBox, viewportRect);
	if (!boundingBox.isEmpty())
	{
		// Build pixel segments list
		outPixelSegments.reserve(boundingBox.height);
		if (nullptr == options.mTransform)
		{
			// Simple output the bounding rect
			for (int iy = 0; iy < boundingBox.height; ++iy)
			{
				PixelSegment& pixelSegment = vectorAdd(outPixelSegments);
				pixelSegment.mPosition.set(0, iy);
				pixelSegment.mNumPixels = boundingBox.width;
			}
		}
		else
		{
			// Output transformed bounding rect
			for (int iy = 0; iy < boundingBox.height; ++iy)
			{
				int minX = 0;
				int maxX = 0;
				getTransformedLineRange(minX, maxX, iy, boundingBox.width, position - boundingBox.getPos(), spriteRect, options.mInvTransform);
				if (minX < maxX)
				{
					PixelSegment& pixelSegment = vectorAdd(outPixelSegments);
					pixelSegment.mPosition.set(minX, iy);
					pixelSegment.mNumPixels = maxX - minX;
				}
			}
		}
	}

	return boundingBox;
}

bool Blitter::needsIntermediateProcessing(const Options& options)
{
	return (nullptr != options.mTintColor || nullptr != options.mAddedColor || options.mSwapRedBlueChannels);
}

void Blitter::processIntermediateBitmap(BitmapViewMutable<uint32>& bitmap, Options& options)
{
	const Options* useOptions = &options;
	if (nullptr != options.mTintColor || nullptr != options.mAddedColor)
	{
		int mult[4];
		if (nullptr != options.mTintColor)
		{
			mult[0] = clamp((int)(options.mTintColor->r * 0x100 + 0.5f), -0x10000, 0x10000);
			mult[1] = clamp((int)(options.mTintColor->g * 0x100 + 0.5f), -0x10000, 0x10000);
			mult[2] = clamp((int)(options.mTintColor->b * 0x100 + 0.5f), -0x10000, 0x10000);
			mult[3] = clamp((int)(options.mTintColor->a * 0x100 + 0.5f), -0x10000, 0x10000);
		}
		else
		{
			mult[0] = 0x100;
			mult[1] = 0x100;
			mult[2] = 0x100;
			mult[3] = 0x100;
		}

		if (nullptr == options.mAddedColor)
		{
			// Only apply tint color
			for (int y = 0; y < bitmap.getSize().y; ++y)
			{
				uint8* dst = (uint8*)bitmap.getLinePointer(y);
				for (int x = 0; x < bitmap.getSize().x; ++x)
				{
					dst[0] = (uint8)clamp((dst[0] * mult[0]) >> 8, 0, 0xff);
					dst[1] = (uint8)clamp((dst[1] * mult[1]) >> 8, 0, 0xff);
					dst[2] = (uint8)clamp((dst[2] * mult[2]) >> 8, 0, 0xff);
					dst[3] = (uint8)clamp((dst[3] * mult[3]) >> 8, 0, 0xff);
					dst += 4;
				}
			}
		}
		else
		{
			// Apply tint & added color
			//  -> Even though tint color may be unused, so that we're just multiplying by 1, but that's expected to be a quite rare case
			const int add[3] =
			{
				(int)(options.mAddedColor->r * 0xff + 0.5f),
				(int)(options.mAddedColor->g * 0xff + 0.5f),
				(int)(options.mAddedColor->b * 0xff + 0.5f)
			};
			for (int y = 0; y < bitmap.getSize().y; ++y)
			{
				uint8* dst = (uint8*)bitmap.getLinePointer(y);
				for (int x = 0; x < bitmap.getSize().x; ++x)
				{
					dst[0] = (uint8)clamp(((dst[0] * mult[0]) >> 8) + add[0], 0, 0xff);
					dst[1] = (uint8)clamp(((dst[1] * mult[1]) >> 8) + add[1], 0, 0xff);
					dst[2] = (uint8)clamp(((dst[2] * mult[2]) >> 8) + add[2], 0, 0xff);
					dst[3] = (uint8)clamp(((dst[3] * mult[3]) >> 8),          0, 0xff);
					dst += 4;
				}
			}
		}

		// Special handling for one-bit alpha if tint color enforces alpha blending
		if (nullptr != options.mTintColor && options.mTintColor->a < 1.0f && options.mBlendMode == BlendMode::ONE_BIT)
		{
			options.mBlendMode = BlendMode::ALPHA;
			const uint8 alphaValue = (uint8)(options.mTintColor->a * 255.0f + 0.5f);
			for (int y = 0; y < bitmap.getSize().y; ++y)
			{
				uint8* dst = (uint8*)bitmap.getLinePointer(y);
				for (int x = 0; x < bitmap.getSize().x; ++x)
				{
					dst[3] = (dst[3] > 0) ? alphaValue : 0;
					dst += 4;
				}
			}
		}
	}

	if (options.mSwapRedBlueChannels)
	{
		// Copy over data and swap red and blue channels
		const int numPixels = bitmap.getSize().x;
		for (int y = 0; y < bitmap.getSize().y; ++y)
		{
			uint32* dst = bitmap.getLinePointer(y);
			int k = 0;
			if constexpr (sizeof(void*) == 8)
			{
				// On 64-bit architectures: Process 2 pixels at once
				for (; k < numPixels; k += 2)
				{
					const uint64 colors = *(uint64*)dst;
					*(uint64*)dst = ((colors & 0x00ff000000ff0000ull) >> 16) | (colors & 0xff00ff00ff00ff00ull) | ((colors & 0x000000ff000000ffull) << 16);
					dst += 2;
				}
			}
			// Process single pixels
			for (; k < numPixels; ++k)
			{
				const uint32 color = *dst;
				*dst = ((color & 0x00ff0000) >> 16) | (color & 0xff00ff00) | ((color & 0x000000ff) << 16);
				++dst;
			}
		}
	}
}
