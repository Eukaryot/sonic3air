/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/palette/Palette.h"


uint16 Palette::getEntryPacked(uint16 colorIndex, bool allowExtendedPacked) const
{
	RMX_CHECK(colorIndex < Palette::NUM_COLORS, "Invalid color index " << colorIndex, return 0);
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

void Palette::resetAllPaletteChangeFlags()
{
	mChangeFlags.clearAllBits();
}

void Palette::setAllPaletteChangeFlags()
{
	mChangeFlags.setAllBits();
}

void Palette::invalidatePackedColorCache()
{
	for (size_t k = 0; k < NUM_COLORS; ++k)
		mPackedColorCache[k].mIsValid = false;
}

void Palette::setPaletteEntry(uint16 colorIndex, uint32 color)
{
	if (mColor[colorIndex] != color)
	{
		mColor[colorIndex] = color;
		mChangeFlags.setBit(colorIndex);
		mPackedColorCache[colorIndex].mIsValid = false;
	}
}

void Palette::setPaletteEntryPacked(uint16 colorIndex, uint32 color, uint16 packedColor)
{
	if (mColor[colorIndex] != color)
	{
		mColor[colorIndex] = color;
		mChangeFlags.setBit(colorIndex);
		mPackedColorCache[colorIndex].mPackedColor = packedColor;
		mPackedColorCache[colorIndex].mIsValid = true;
	}
}

void Palette::dumpColors(Color* outColors, int numColors) const
{
	for (int i = 0; i < numColors; ++i)
	{
		outColors[i] = getColor(i);
	}
}

void Palette::serializePalette(VectorBinarySerializer& serializer)
{
	for (size_t k = 0; k < NUM_COLORS; ++k)
	{
		serializer.serialize(mColor[k]);

		if (serializer.isReading())
		{
			setAllPaletteChangeFlags();
			invalidatePackedColorCache();
		}
	}
}
