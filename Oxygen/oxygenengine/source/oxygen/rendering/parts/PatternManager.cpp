/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/PatternManager.h"
#include "oxygen/rendering/utils/RenderUtils.h"
#include "oxygen/simulation/EmulatorInterface.h"


void PatternManager::refresh()
{
	// Update pattern cache content
	const uint8* src = EmulatorInterface::instance().getVRam();

	for (int name = 0; name < 0x800; ++name)
	{
		CacheItem& cacheItem = mPatternCache[name];
		cacheItem.mChanged = (memcmp(cacheItem.mOriginalDataBackup, src, 0x20) != 0);

		// Check for changes
		if (cacheItem.mChanged)
		{
			CacheItem::Pattern* patterns = cacheItem.mFlipVariation;

			// Fill main pattern
			RenderUtils::expandPatternDataFromVRAM(patterns[0].mPixels, src);

			// Fill other flip variations
			for (uint8 y = 0; y < 8; ++y)
			{
				uint8* v0 = &patterns[0].mPixels[y * 8];
				uint8* v1 = &patterns[1].mPixels[y * 8];
				uint8* v2 = &patterns[2].mPixels[(7 - y) * 8];
				uint8* v3 = &patterns[3].mPixels[(7 - y) * 8];

				for (uint8 x = 0; x < 8; ++x)
				{
					v1[x] = v0[7 - x];
				}
				memcpy(v2, v0, 8);
				memcpy(v3, v1, 8);
			}

			memcpy(cacheItem.mOriginalDataBackup, src, 0x20);
		}

		src += 0x20;
	}
}

uint8 PatternManager::getLastUsedAtex(uint16 patternIndex) const
{
	return mPatternCache[patternIndex & 0x07ff].mLastUsedAtex;
}

void PatternManager::setLastUsedAtex(uint16 patternIndex, uint8 atex)
{
	mPatternCache[patternIndex & 0x07ff].mLastUsedAtex = atex;
}

void PatternManager::dumpAsPaletteBitmap(PaletteBitmap& output) const
{
	output.create(512, 256);
	for (int y = 0; y < 256; ++y)
	{
		for (int x = 0; x < 512; ++x)
		{
			const int patternIndex = (x/8) + (y/8) * 64;
			output.mData[x+y*512] = mPatternCache[patternIndex].mFlipVariation[0].mPixels[(x%8) + (y%8) * 8] + getLastUsedAtex(patternIndex);
		}
	}
}
