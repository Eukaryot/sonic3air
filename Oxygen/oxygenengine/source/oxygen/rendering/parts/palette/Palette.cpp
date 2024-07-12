/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/palette/Palette.h"


void PaletteBase::initPalette(uint64 key, size_t size, BitFlagSet<Properties> properties)
{
	mKey = key;
	mColors.resize(size);
	mProperties = properties;
}

void PaletteBase::dumpColors(uint32* outColors, size_t numColors) const
{
	numColors = std::min(numColors, getSize());
	if (numColors > 0)
		memcpy(outColors, getRawColors(), numColors * sizeof(uint32));
}

void PaletteBase::writeRawColors(const uint32* colors, size_t numColors)
{
	numColors = std::min(numColors, getSize());
	if (numColors > 0)
		memcpy(&mColors[0], colors, numColors * sizeof(uint32));
}


void Palette::initPalette(uint64 key, size_t size, BitFlagSet<Properties> properties)
{
	PaletteBase::initPalette(key, size, properties);
	mPackedColorCache.resize(size);
}

uint16 Palette::getEntryPacked(uint16 colorIndex, bool allowExtendedPacked) const
{
	RMX_CHECK(colorIndex < getSize(), "Invalid color index " << colorIndex, return 0);
	const Color color = getColor(colorIndex);
	if (allowExtendedPacked)	// For notes on extended packed color format, see "writePaletteEntryPacked"
	{
		const uint32 r = ((roundToInt(saturate(color.r) * 255.0f) + 0x04) / 0x09);
		const uint32 g = ((roundToInt(saturate(color.g) * 255.0f) + 0x04) / 0x09);
		const uint32 b = ((roundToInt(saturate(color.b) * 255.0f) + 0x04) / 0x09);
		return (uint16)((r) + (g << 5) + (b << 10) + 0x8000);
	}
	else
	{
		const uint32 r = ((roundToInt(saturate(color.r) * 255.0f) + 0x12) / 0x24);
		const uint32 g = ((roundToInt(saturate(color.g) * 255.0f) + 0x12) / 0x24);
		const uint32 b = ((roundToInt(saturate(color.b) * 255.0f) + 0x12) / 0x24);
		return (uint16)((r << 1) + (g << 5) + (b << 9));
	}
}

void Palette::invalidatePackedColorCache()
{
	for (PackedPaletteColor& packed : mPackedColorCache)
		packed.mIsValid = false;
}

void Palette::setPaletteEntry(uint16 colorIndex, uint32 color)
{
	if (mColors[colorIndex] != color)
	{
		mColors[colorIndex] = color;
		mPackedColorCache[colorIndex].mIsValid = false;
		increaseChangeCounter();
	}
}

void Palette::setPaletteEntryPacked(uint16 colorIndex, uint32 color, uint16 packedColor)
{
	if (mColors[colorIndex] != color)
	{
		mColors[colorIndex] = color;
		mPackedColorCache[colorIndex].mPackedColor = packedColor;
		mPackedColorCache[colorIndex].mIsValid = true;
		increaseChangeCounter();
	}
}

void Palette::serializePalette(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	for (uint32& color : mColors)
	{
		serializer.serialize(color);
	}

	if (serializer.isReading())
	{
		increaseChangeCounter();
		invalidatePackedColorCache();
	}
}
