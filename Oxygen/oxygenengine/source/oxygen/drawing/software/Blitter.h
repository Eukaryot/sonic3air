/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>
#include "oxygen/rendering/RenderingDefinitions.h"


class Blitter
{
public:
	struct Options
	{
		// Transformation
		const float* mTransform = nullptr;			// Optional 2x2 transformation matrix
		const float* mInvTransform = nullptr;		// Must be set if mTransform is set, and be the inverse 2x2 matrix

		// Sampling & blending
		SamplingMode mSamplingMode = SamplingMode::POINT;
		BlendMode mBlendMode = BlendMode::OPAQUE;

		// Color handling
		const Vec4f* mTintColor = nullptr;
		const Vec4f* mAddedColor = nullptr;
		bool mSwapRedBlueChannels = false;

		// Depth test
		const BitmapViewMutable<uint8>* mDepthBuffer = nullptr;
		uint8 mDepthTestValue = 0;					// Blitter will never do a depth write, only depth test if mDepthBuffer is set
	};

	struct OutputWrapper
	{
		BitmapViewMutable<uint32> mBitmapView;
		Recti mViewportRect;

		inline OutputWrapper(Bitmap& bitmap) : mBitmapView(bitmap), mViewportRect(Vec2i(), bitmap.getSize()) {}
		inline OutputWrapper(Bitmap& bitmap, const Recti& rect) : mBitmapView(bitmap) { mViewportRect.intersect(rect, Recti(Vec2i(), bitmap.getSize())); }
		inline OutputWrapper(BitmapViewMutable<uint32> view) : mBitmapView(view), mViewportRect(Vec2i(), view.getSize()) {}
		inline OutputWrapper(BitmapViewMutable<uint32> view, const Recti& rect) : mBitmapView(view) { mViewportRect.intersect(rect, Recti(Vec2i(), view.getSize())); }
		inline OutputWrapper(uint32* data, Vec2i size) : mBitmapView(data, size), mViewportRect(Vec2i(), size) {}
		inline OutputWrapper(uint32* data, Vec2i size, const Recti& rect) : mBitmapView(data, size) { mViewportRect.intersect(rect, Recti(Vec2i(), size)); }
	};

	struct SpriteWrapper
	{
		BitmapView<uint32> mBitmapView;
		Vec2i mPivot;		// Note: This usually uses positive value, i.e. it's not negated like in oxygen's software renderer

		inline SpriteWrapper(const Bitmap& bitmap, Vec2i pivot) : mBitmapView(bitmap), mPivot(pivot) {}
		inline SpriteWrapper(const Bitmap& bitmap, Vec2i pivot, Recti innerRect) : mBitmapView(bitmap, innerRect), mPivot(pivot - innerRect.getPos()) {}
		inline SpriteWrapper(const uint32* data, Vec2i size, Vec2i pivot) : mBitmapView(data, size), mPivot(pivot) {}
		inline SpriteWrapper(const uint32* data, Vec2i size, Vec2i pivot, Recti innerRect) : mBitmapView(data, size, innerRect), mPivot(pivot - innerRect.getPos()) {}
	};

	struct IndexedSpriteWrapper
	{
		BitmapView<uint8> mBitmapView;
		Vec2i mPivot;		// Note: This usually uses positive value, i.e. it's not negated like in oxygen's software renderer

		inline IndexedSpriteWrapper(const uint8* data, Vec2i size, Vec2i pivot) : mBitmapView(data, size), mPivot(pivot) {}
		inline IndexedSpriteWrapper(const uint8* data, Vec2i size, Vec2i pivot, Recti innerRect) : mBitmapView(data, size, innerRect), mPivot(pivot - innerRect.getPos()) {}
	};

	struct PaletteWrapper
	{
		const uint32* mPalette = nullptr;
		size_t mNumEntries = 0;

		inline PaletteWrapper() {}
		inline PaletteWrapper(const uint32* palette, size_t numEntries) : mPalette(palette), mNumEntries(numEntries) {}
	};

	struct PixelSegment
	{
		Vec2i mPosition;
		int mNumPixels = 0;
	};

public:
	void blitColor(const OutputWrapper& output, const Color& color, BlendMode blendMode);
	void blitSprite(const OutputWrapper& output, const SpriteWrapper& sprite, Vec2i position, Options& options);
	void blitIndexed(const OutputWrapper& output, const IndexedSpriteWrapper& sprite, const PaletteWrapper& palette, Vec2i position, Options& options);
	void blitRectWithScaling(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options);
	void blitRectWithUVs(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options);

private:
	BitmapViewMutable<uint32> makeTempBitmap(Vec2i size);
	BitmapViewMutable<uint32> makeTempBitmapAsCopy(const BitmapView<uint32>& input, Vec2i size, Vec2i innerIndent);
	BitmapViewMutable<uint32> makeTempBitmapAsCopy(const BitmapView<uint8>& input, const PaletteWrapper& palette, Vec2i size, Vec2i innerIndent);
	BitmapViewMutable<uint32> makeTempBitmapAsTransformedCopy(Recti outputBoundingBox, const SpriteWrapper& sprite, Vec2i position, const Options& options);
	BitmapViewMutable<uint32> makeTempBitmapAsTransformedCopy(Recti outputBoundingBox, const IndexedSpriteWrapper& sprite, const PaletteWrapper& palette, Vec2i position, const Options& options);

	static Recti applyCropping(std::vector<PixelSegment>& outPixelSegments, const Recti& viewportRect, const Recti& spriteRect, const Vec2i& position, const Options& options);
	static bool needsIntermediateProcessing(const Options& options);
	static void processIntermediateBitmap(BitmapViewMutable<uint32>& bitmap, Options& options);

private:
	std::vector<uint32> mTempBitmapData;		// Defined here so its reserved memory can be reused for multiple blitting calls
	std::vector<PixelSegment> mPixelSegments;
};
