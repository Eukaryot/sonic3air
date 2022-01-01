/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/utils/PaletteBitmap.h"


class RenderUtils
{
public:
	struct PatternPixelContent
	{
		uint8 mPixels[0x40] = { 0 };	// Each pattern is 8*8 pixels
	};

	struct PatternData
	{
		const uint8* mPixels = nullptr;
		uint8 mAtex = 0;
	};

	struct SinglePattern
	{
		int mOffsetX = 0;
		int mOffsetY = 0;
		bool mFlipX = false;
		bool mFlipY = false;
		PatternData mPatternData;
	};

public:
	static Rectf getLetterBoxRect(const Rectf& frameRect, float aspectRatio);
	static Rectf getScaleToFillRect(const Rectf& frameRect, float aspectRatio);

	static void expandPatternDataFromVRAM(uint8* dst, const void* src_);
	static void expandPatternDataFromROM(uint8* dst, const void* src_);
	static void expandMultiplePatternDataFromROM(std::vector<PatternPixelContent>& patternBuffer, const uint8* src, uint32 numPatterns);

	static void fillPatternsFromSpriteData(std::vector<SinglePattern>& patterns, const uint8* data, const std::vector<PatternPixelContent>& patternBuffer, int16 indexOffset = 0);

	static void blitSpritePattern(PaletteBitmap& output, int px, int py, const PatternData& patternData, bool flipX = false, bool flipY = false);
	static void blitSpritePatterns(PaletteBitmap& output, int px, int py, const std::vector<SinglePattern>& patterns);
};
