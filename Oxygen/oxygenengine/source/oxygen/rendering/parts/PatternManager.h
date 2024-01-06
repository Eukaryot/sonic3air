/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class PaletteBitmap;


class PatternManager
{
public:
	struct CacheItem
	{
		struct Pattern
		{
			uint8 mPixels[64] = { 0 };
		};
		Pattern mFlipVariation[4];
		uint8 mOriginalDataBackup[32];
		mutable uint8 mLastUsedAtex = 0;	// Only for debug output
	};

public:
	void refresh();

	uint8 getLastUsedAtex(uint16 patternIndex) const;
	void setLastUsedAtex(uint16 patternIndex, uint8 atex);

	inline const CacheItem* getPatternCache() const  { return mPatternCache; }
	const BitArray<0x800>& getChangeBits() const  { return mChangeBits; }

	void dumpAsPaletteBitmap(PaletteBitmap& output) const;

private:
	CacheItem mPatternCache[0x800];
	BitArray<0x800> mChangeBits;	// One bit for each pattern, so we know which ones were changed in the last "refresh" call
};
