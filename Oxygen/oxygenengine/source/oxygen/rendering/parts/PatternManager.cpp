/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
	mChangeBits.clearAllBits();

	// Update pattern cache content
	const BitArray<0x800>& changeBits = EmulatorInterface::instance().getVRamChangeBits();
	for (int bitSetChunkIndex = 0; bitSetChunkIndex < 32; ++bitSetChunkIndex)	// Each chunk is 64 bits, each representing one pattern (= 32 bytes of VRAM)
	{
		if (changeBits.anyBitSetInChunk(bitSetChunkIndex))
		{
			const uint8* src = EmulatorInterface::instance().getVRam() + (size_t)bitSetChunkIndex * 0x800;
			for (int bitIndex = 0; bitIndex < 64; ++bitIndex)
			{
				// Check the change bit
				const int patternIndex = (bitSetChunkIndex << 6) + bitIndex;
				if (changeBits.isBitSet(patternIndex))
				{
					CacheItem& cacheItem = mPatternCache[patternIndex];

					// Check for actual changes
					//  -> This code is slightly optimized compared to a memcmp
					const uint64* cache = (uint64*)cacheItem.mOriginalDataBackup;
					const uint64* source = (uint64*)src;
					const bool changed = ((cache[0] != source[0]) | (cache[1] != source[1]) | (cache[2] != source[2]) | (cache[3] != source[3])) != 0;

					if (changed)
					{
						// Fill main pattern
						CacheItem::Pattern* patterns = cacheItem.mFlipVariation;
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
						mChangeBits.setBit(patternIndex);
					}
				}

				src += 0x20;
			}
		}
	}
	EmulatorInterface::instance().getVRamChangeBits().clearAllBits();
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
			output[x+y*512] = mPatternCache[patternIndex].mFlipVariation[0].mPixels[(x%8) + (y%8) * 8] + getLastUsedAtex((uint16)patternIndex);
		}
	}
}
