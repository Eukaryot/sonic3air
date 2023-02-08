/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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
		const float* mTransform = nullptr;		// Optional 2x2 transformation matrix
		const float* mInvTransform = nullptr;	// Must be set if mTransform is set, and be the inverse 2x2 matrix
		SamplingMode mSamplingMode = SamplingMode::POINT;
		BlendMode mBlendMode = BlendMode::OPAQUE;
		const Color* mTintColor = nullptr;
		const Color* mAddedColor = nullptr;
		bool mSwapRedBlueChannels = false;
		const BitmapViewMutable<uint8>* mDepthBuffer = nullptr;
		uint8 mDepthValue = 0;
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
		inline SpriteWrapper(uint32* data, Vec2i size, Vec2i pivot) : mBitmapView(data, size), mPivot(pivot) {}
		inline SpriteWrapper(uint32* data, Vec2i size, Vec2i pivot, Recti innerRect) : mBitmapView(data, size, innerRect), mPivot(pivot - innerRect.getPos()) {}
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

public:
	void blitColor(const OutputWrapper& output, const Color& color, BlendMode blendMode);
	void blitSprite(const OutputWrapper& output, const SpriteWrapper& sprite, Vec2i position, const Options& options);
	void blitIndexed(const OutputWrapper& output, const IndexedSpriteWrapper& sprite, const PaletteWrapper& palette, Vec2i position, const Options& options);

private:
	BitmapViewMutable<uint32> makeTempBitmap(Vec2i size);
	BitmapViewMutable<uint32> makeTempBitmapAsCopy(const BitmapView<uint32>& input, Vec2i size, Vec2i innerIndent);
	BitmapViewMutable<uint32> makeTempBitmapAsCopy(const BitmapView<uint8>& input, const PaletteWrapper& palette, Vec2i size, Vec2i innerIndent);
	BitmapViewMutable<uint32> makeTempBitmapAsTransformedCopy(Recti outputBoundingBox, const SpriteWrapper& sprite, Vec2i position, const Options& options);
	BitmapViewMutable<uint32> makeTempBitmapAsTransformedCopy(Recti outputBoundingBox, const IndexedSpriteWrapper& sprite, const PaletteWrapper& palette, Vec2i position, const Options& options);

	static Recti applyCropping(const Recti& viewportRect, const Recti& spriteRect, const Vec2i& position, const Options& options);
	static bool needsIntermediateProcessing(const Options& options);
	static void processIntermediateBitmap(BitmapViewMutable<uint32>& bitmap, const Options& options);

private:
	std::vector<uint32> mTempBitmapData;		// Defined here so its reserved memory can be reused for multiple blitting calls
};
