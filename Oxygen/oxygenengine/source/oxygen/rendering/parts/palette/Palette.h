/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class PaletteBase
{
public:
	enum class Properties
	{
		READ_ONLY	= 1 << 0,	// Palette can't be written to
		MODDED		= 1 << 1,	// Palette is loaded from a mod
	};

public:
	virtual void initPalette(size_t size, BitFlagSet<Properties> properties);

	inline size_t getSize() const				{ return mColors.size(); }
	inline const uint32* getRawColors() const	{ return &mColors[0]; }

	inline uint32 getEntry(int index) const		{ return (index >= 0 && index < (int)getSize()) ? mColors[index] : 0; }
	inline Color getColor(int index) const		{ return Color::fromABGR32(getEntry(index)); }

	void dumpColors(uint32* outColors, size_t numColors) const;
	virtual void writeRawColors(const uint32* colors, size_t numColors);

	inline uint16 getChangeCounter() const		{ return mChangeCounter; }
	inline void increaseChangeCounter()			{ ++mChangeCounter; }

	inline BitFlagSet<Properties> getProperties() const  { return mProperties; }

protected:
	std::vector<uint32> mColors;	// Colors in the palette, using ABGR32 format
	uint16 mChangeCounter = 1;
	BitFlagSet<Properties> mProperties;
};


class Palette : public PaletteBase
{
friend class PaletteManager;

public:
	virtual void initPalette(size_t size, BitFlagSet<Properties> properties) override;

	uint16 getEntryPacked(uint16 colorIndex, bool allowExtendedPacked = false) const;

	void invalidatePackedColorCache();

	void setPaletteEntry(uint16 colorIndex, uint32 color);
	void setPaletteEntryPacked(uint16 colorIndex, uint32 color, uint16 packedColor);

	void serializePalette(VectorBinarySerializer& serializer, uint8 formatVersion);

private:
	struct PackedPaletteColor
	{
		uint16 mPackedColor = 0;
		bool mIsValid = false;
	};

private:
	std::vector<PackedPaletteColor> mPackedColorCache;	// Only used as an optimization
};
