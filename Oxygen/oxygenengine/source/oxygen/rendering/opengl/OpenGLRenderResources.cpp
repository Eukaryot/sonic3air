/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"


namespace
{
	void writeToBufferIfNeeded(int firstPattern, int lastPattern, const PaletteBitmap& bitmap)
	{
	#if !defined(RMX_USE_GLES2)
		if (BufferTexture::supportsBufferTextures())
		{
			const int offset = firstPattern * 0x40;
			const int size = (lastPattern - firstPattern + 1) * 0x40;
			glBufferSubData(GL_TEXTURE_BUFFER, offset, size, bitmap.getData() + offset);
		}
		else
	#endif
		{
			// Is it the full image, or at least most of it?
			if (lastPattern - firstPattern >= 0x700)
			{
				// Update the whole texture
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 0x40, 0x800, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bitmap.getData());
			}
			else
			{
				// Update only the changed lines
				//  -> There's exactly one patterns per texture row
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, firstPattern, 0x40, (lastPattern - firstPattern + 1), GL_LUMINANCE, GL_UNSIGNED_BYTE, &bitmap[firstPattern * 0x40]);
			}
		}
	}
}


OpenGLRenderResources::OpenGLRenderResources(RenderParts& renderParts, OpenGLDrawerResources& drawerResources) :
	mRenderParts(renderParts),
	mDrawerResources(drawerResources)
{
	clearAllCaches();
}

void OpenGLRenderResources::initialize()
{
	// Main palette
	mMainPalette.mBitmap.create(mDrawerResources.getPaletteTextureSize());

	// Patterns
	mPatternCacheBitmap.create(0x40, 0x800);
	mPatternCacheTexture.create(BufferTexture::PixelFormat::UINT_8, mPatternCacheBitmap.getSize());

	// Planes & scrolling
	{
		const PlaneManager& planeManager = mRenderParts.getPlaneManager();
		for (int index = 0; index < 4; ++index)
		{
			mPlanePatternsTexture[index].create(BufferTexture::PixelFormat::UINT_16, 0x1000, 1, planeManager.getPlanePatternsBuffer((uint8)index));
		}

		for (int index = 0; index < 4; ++index)
		{
			mHScrollOffsetsTexture[index].create(BufferTexture::PixelFormat::INT_16, 0x100, 1, nullptr);
			mVScrollOffsetsTexture[index].create(BufferTexture::PixelFormat::INT_16, 0x20, 1, nullptr);
		}

		mEmptyScrollOffsetsTexture.create(BufferTexture::PixelFormat::UINT_16, 0x100, 1, mRenderParts.getScrollOffsetsManager().getScrollOffsetsH(0xff));
	}
}

void OpenGLRenderResources::refresh()
{
	const PlaneManager& planeManager = mRenderParts.getPlaneManager();

	// Update palettes
	{
		PaletteManager& paletteManager = mRenderParts.getPaletteManager();
		if (mDrawerResources.updatePalette(mMainPalette, paletteManager.getMainPalette(0), paletteManager.getMainPalette(1)))
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	// Update pattern cache texture
	{
		PaletteBitmap& bitmap = mPatternCacheBitmap;
		const PatternManager::CacheItem* patternCache = mRenderParts.getPatternManager().getPatternCache();
		const BitArray<0x800>& patternChangeBits = mRenderParts.getPatternManager().getChangeBits();

		mPatternCacheTexture.bindBuffer();
		struct Range
		{
			int mFirst = -1;
			int mLast = -1;
			inline bool valid() const  { return mFirst != -1; }
		};
		Range pendingChanges;
		Range currentChanges;

		for (int patternIndex = 0; patternIndex < 0x800; ++patternIndex)
		{
			if (!mAllPatternsDirty)
			{
				// Skip all non-changed patterns
				patternIndex = patternChangeBits.getNextSetBit(patternIndex);
				if (patternIndex < 0)
					break;
			}

			// Next collect as many successive changed patterns as possible
			currentChanges.mFirst = patternIndex;
			if (mAllPatternsDirty)
			{
				patternIndex = 0x800;
			}
			else
			{
				patternIndex = patternChangeBits.getNextClearedBit(patternIndex + 1);
				if (patternIndex < 0)
					patternIndex = 0x800;
			}
			currentChanges.mLast = patternIndex - 1;

			// Update pattern data in bitmap for all changed patterns
			for (int k = currentChanges.mFirst; k <= currentChanges.mLast; ++k)
			{
				const uint8* src = patternCache[k].mFlipVariation[0].mPixels;
				uint8* dst = &bitmap[k * 0x40];
				memcpy(dst, src, 0x40);
			}

			// We got a new range of patterns to upload to GPU, but possibly also an old one
			if (pendingChanges.valid())
			{
				// Should we merge them?
				//  -> This means we ignore small gaps between the changes, in order to keep the number of single upload calls lows,
				//      even if that means re-uploading non-changed data in between the changes as well
				if (currentChanges.mFirst - pendingChanges.mLast <= 0x20)
				{
					pendingChanges.mLast = currentChanges.mLast;
				}
				else
				{
					// Upload the pending changes range to GPU
					writeToBufferIfNeeded(pendingChanges.mFirst, pendingChanges.mLast, bitmap);
					pendingChanges = currentChanges;
				}
			}
			else
			{
				pendingChanges = currentChanges;
			}
		}
		mAllPatternsDirty = false;

		// Is there pending changes that need to be uploaded?
		if (pendingChanges.valid())
		{
			writeToBufferIfNeeded(pendingChanges.mFirst, pendingChanges.mLast, bitmap);
		}
		mPatternCacheTexture.unbindBuffer();
	}

	// Update plane pattern textures
	{
		const int numPatterns = planeManager.getPlayfieldSizeInPatterns().x * planeManager.getPlayfieldSizeInPatterns().y;
		for (int index = 0; index < 4; ++index)
		{
			if (!planeManager.isPlaneUsed(index))
				continue;

			const uint16* buffer = planeManager.getPlanePatternsBuffer((uint8)index);
			const bool hasChanged = (memcmp(mPlanePatternsData[index], buffer, numPatterns * 2) != 0);
			if (!hasChanged)
				continue;

			memcpy(mPlanePatternsData[index], buffer, numPatterns * 2);
			mPlanePatternsTexture[index].bufferData(buffer, numPatterns, 1);
		}
	}

	// Update scroll offset textures
	{
		const ScrollOffsetsManager& scrollOffsetsManager = mRenderParts.getScrollOffsetsManager();

		// Horizontal scroll offsets
		for (int index = 0; index < 4; ++index)
		{
			mHScrollOffsetsTexture[index].bufferData(scrollOffsetsManager.getScrollOffsetsH(index), 0x100, 1);
		}

		// Vertical scroll offsets
		if (scrollOffsetsManager.getVerticalScrolling())
		{
			for (int index = 0; index < 4; ++index)
			{
				mVScrollOffsetsTexture[index].bufferData(scrollOffsetsManager.getScrollOffsetsV(index), 0x20, 1);
			}
		}
	}
}

void OpenGLRenderResources::clearAllCaches()
{
	mAllPatternsDirty = true;

	// Palettes
	for (int k = 0; k < 2; ++k)
		--mMainPalette.mChangeCounters[k];		// This should suffice to have out-dated change counters again
}

const OpenGLTexture& OpenGLRenderResources::getPaletteTexture(const PaletteBase* primaryPalette, const PaletteBase* secondaryPalette)
{
	if (nullptr == primaryPalette && nullptr == secondaryPalette)
	{
		return getMainPaletteTexture();
	}
	else
	{
		if (nullptr == primaryPalette)
			primaryPalette = &mRenderParts.getPaletteManager().getMainPalette(0);
		if (nullptr == secondaryPalette)
			secondaryPalette = &mRenderParts.getPaletteManager().getMainPalette(1);
		return mDrawerResources.getCustomPaletteTexture(*primaryPalette, *secondaryPalette);
	}
}

const BufferTexture& OpenGLRenderResources::getPlanePatternsTexture(int planeIndex) const
{
	RMX_ASSERT(planeIndex >= 0 && planeIndex < 4, "Invalid plane index " << planeIndex);
	return mPlanePatternsTexture[planeIndex];
}

const BufferTexture& OpenGLRenderResources::getHScrollOffsetsTexture(int scrollOffsetsIndex) const
{
	if (scrollOffsetsIndex == 0xff)
		return mEmptyScrollOffsetsTexture;

	RMX_ASSERT(scrollOffsetsIndex >= 0 && scrollOffsetsIndex < 4, "Invalid scroll offsets index " << scrollOffsetsIndex);
	return mHScrollOffsetsTexture[scrollOffsetsIndex];
}

const BufferTexture& OpenGLRenderResources::getVScrollOffsetsTexture(int scrollOffsetsIndex) const
{
	if (scrollOffsetsIndex == 0xff)
		return mEmptyScrollOffsetsTexture;

	RMX_ASSERT(scrollOffsetsIndex >= 0 && scrollOffsetsIndex < 4, "Invalid scroll offsets index " << scrollOffsetsIndex);
	return mVScrollOffsetsTexture[scrollOffsetsIndex];
}

#endif
