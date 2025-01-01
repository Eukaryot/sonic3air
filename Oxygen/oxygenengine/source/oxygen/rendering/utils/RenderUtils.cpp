/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/utils/RenderUtils.h"


namespace
{
	uint16 readSwapped16(const uint8* src)
	{
		return (uint16)((src[0] << 8) + src[1]);
	}
}



Recti RenderUtils::getLetterBoxRect(const Recti& frameRect, float aspectRatio)
{
	Recti rect = frameRect;
	const float frameRatio = frameRect.getAspectRatio();

	if (frameRatio < aspectRatio)
	{
		const int newHeight = roundToInt((float)rect.height * frameRatio / aspectRatio);
		rect.y += (rect.height - newHeight) / 2;
		rect.height = newHeight;
	}
	else
	{
		const int newWidth = roundToInt((float)rect.width * aspectRatio / frameRatio);
		rect.x += (rect.width - newWidth) / 2;
		rect.width = newWidth;
	}
	return rect;
}


Recti RenderUtils::getScaleToFillRect(const Recti& frameRect, float aspectRatio)
{
	Recti rect = frameRect;
	const float frameRatio = frameRect.getAspectRatio();

	if (frameRatio > aspectRatio)
	{
		const int newHeight = roundToInt((float)rect.height * frameRatio / aspectRatio);
		rect.y += (rect.height - newHeight) / 2;
		rect.height = newHeight;
	}
	else
	{
		const int newWidth = roundToInt((float)rect.width * aspectRatio / frameRatio);
		rect.x += (rect.width - newWidth) / 2;
		rect.width = newWidth;
	}
	return rect;
}


void RenderUtils::expandPatternDataFromVRAM(uint8* dst, const void* src_)
{
	uint32* src = (uint32*)src_;
	for (uint8 y = 0; y < 8; ++y)
	{
		const uint32 bp = src[y];
		dst[0] = (bp >> 12) & 0x0f;
		dst[1] = (bp >> 8)  & 0x0f;
		dst[2] = (bp >> 4)  & 0x0f;
		dst[3] = (bp >> 0)  & 0x0f;
		dst[4] = (bp >> 28) & 0x0f;
		dst[5] = (bp >> 24) & 0x0f;
		dst[6] = (bp >> 20) & 0x0f;
		dst[7] = (bp >> 16) & 0x0f;
		dst += 8;
	}
}

void RenderUtils::expandPatternDataFromROM(uint8* dst, const void* src_)
{
	uint32* src = (uint32*)src_;
	for (uint8 y = 0; y < 8; ++y)
	{
		const uint32 bp = src[y];
		dst[0] = (bp >> 4)  & 0x0f;
		dst[1] = (bp >> 0)  & 0x0f;
		dst[2] = (bp >> 12) & 0x0f;
		dst[3] = (bp >> 8)  & 0x0f;
		dst[4] = (bp >> 20) & 0x0f;
		dst[5] = (bp >> 16) & 0x0f;
		dst[6] = (bp >> 28) & 0x0f;
		dst[7] = (bp >> 24) & 0x0f;
		dst += 8;
	}
}

void RenderUtils::expandMultiplePatternDataFromROM(std::vector<PatternPixelContent>& patternBuffer, const uint8* src, uint32 numPatterns)
{
	patternBuffer.reserve(patternBuffer.size() + numPatterns);
	for (uint32 j = 0; j < numPatterns; ++j)
	{
		expandPatternDataFromROM(vectorAdd(patternBuffer).mPixels, src);
		src += 0x20;
	}
}

void RenderUtils::fillPatternsFromSpriteData(std::vector<RenderUtils::SinglePattern>& patterns, const uint8* data, const std::vector<PatternPixelContent>& patternBuffer, int16 indexOffset)
{
	const int px = (int16)readSwapped16(data + 4);
	const int py = int8(data[0]);
	const uint8 size = data[1];
	const uint16 index = readSwapped16(data + 2) - indexOffset;

	const int rows = 1 + (size & 3);
	const int columns = 1 + ((size >> 2) & 3);

	const bool flipX = (index & 0x0800) != 0;
	const bool flipY = (index & 0x1000) != 0;
	const uint8 atex = (index >> 9) & 0x30;

	for (int row = 0; row < rows; ++row)
	{
		for (int column = 0; column < columns; ++column)
		{
			const int vcol = flipX ? (columns - 1 - column) : column;
			const int vrow = flipY ? (rows - 1 - row) : row;
			const size_t patternIndex = (size_t)((index + vrow + vcol * rows) & 0x07ff);
			RMX_CHECK(patternIndex < patternBuffer.size(), "Invalid pattern index " << patternIndex << " (must be below " << patternBuffer.size() << ") used while creating a sprite", continue);

			SinglePattern& pattern = vectorAdd(patterns);
			pattern.mOffsetX = px + column * 8;
			pattern.mOffsetY = py + row * 8;
			pattern.mFlipX = flipX;
			pattern.mFlipY = flipY;
			pattern.mPatternData.mAtex = atex;
			pattern.mPatternData.mPixels = patternBuffer[patternIndex].mPixels;
		}
	}
}

void RenderUtils::blitSpritePattern(PaletteBitmap& output, int px, int py, const PatternData& patternData, bool flipX, bool flipY)
{
	const int32 minX = std::max<int32>(0, -px);
	const int32 maxX = std::min<int32>(8, output.getWidth() - px);
	const int32 minY = std::max<int32>(0, -py);
	const int32 maxY = std::min<int32>(8, output.getHeight() - py);

	for (int32 iy = minY; iy < maxY; ++iy)
	{
		const int32 srcY = flipY ? (7 - iy) : iy;
		const uint8* src = &patternData.mPixels[minX + srcY * 8];
		uint8* dst = output.getPixelPointer(px + minX, py + iy);

		if (flipX)
		{
			src += 7;

			for (int32 ix = minX; ix < maxX; ++ix)
			{
				// Check for transparency
				if (*src & 0x0f)
				{
					*dst = *src | patternData.mAtex;
				}
				--src;
				++dst;
			}
		}
		else
		{
			for (int32 ix = minX; ix < maxX; ++ix)
			{
				// Check for transparency
				if (*src & 0x0f)
				{
					*dst = *src | patternData.mAtex;
				}
				++src;
				++dst;
			}
		}
	}
}

void RenderUtils::blitSpritePatterns(PaletteBitmap& output, int px, int py, const std::vector<SinglePattern>& patterns)
{
	// Blit the patterns in reverse order (which is relevant in case they're overlapping, like with Tails' run sprites)
	for (int k = (int)patterns.size() - 1; k >= 0; --k)
	{
		const SinglePattern& pattern = patterns[k];
		RenderUtils::blitSpritePattern(output, px + pattern.mOffsetX, py + pattern.mOffsetY, pattern.mPatternData, pattern.mFlipX, pattern.mFlipY);
	}
}
