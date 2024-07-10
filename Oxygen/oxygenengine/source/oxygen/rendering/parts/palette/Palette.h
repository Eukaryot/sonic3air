/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class Palette
{
friend class PaletteManager;

public:
	static const constexpr size_t NUM_COLORS = 0x200;

public:
	inline size_t getSize() const				{ return NUM_COLORS; }
	inline const uint32* getRawColors() const	{ return mColors; }

	inline uint32 getEntry(int index) const	{ return (index >= 0 && index < (int)getSize()) ? mColors[index] : 0; }
	inline Color getColor(int index) const	{ return Color::fromABGR32(getEntry(index)); }
	uint16 getEntryPacked(uint16 colorIndex, bool allowExtendedPacked = false) const;

	inline const BitArray<NUM_COLORS>& getChangeFlags() const	{ return mChangeFlags; }
	void resetAllPaletteChangeFlags();
	void setAllPaletteChangeFlags();

	void invalidatePackedColorCache();

	void setPaletteEntry(uint16 colorIndex, uint32 color);
	void setPaletteEntryPacked(uint16 colorIndex, uint32 color, uint16 packedColor);

	void dumpColors(uint32* outColors, int numColors) const;
	void serializePalette(VectorBinarySerializer& serializer);

private:
	struct PackedPaletteColor
	{
		uint16 mPackedColor = 0;
		bool mIsValid = false;
	};

private:
	uint32 mColors[NUM_COLORS] = { 0 };							// Colors in the palette, using ABGR32 format
	BitArray<NUM_COLORS> mChangeFlags;							// One flag per color; only actually used and reset by hardware rendering
	PackedPaletteColor mPackedColorCache[NUM_COLORS] = { 0 };	// Only used as an optimization
};
