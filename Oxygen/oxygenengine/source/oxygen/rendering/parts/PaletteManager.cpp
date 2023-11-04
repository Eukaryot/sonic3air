/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/PaletteManager.h"
#include "oxygen/simulation/EmulatorInterface.h"


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
	memset(mChangeFlags, 0, sizeof(mChangeFlags));
}

void Palette::setAllPaletteChangeFlags()
{
	memset(mChangeFlags, 0xff, sizeof(mChangeFlags));
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
		mChangeFlags[colorIndex / 64] |= (uint64)1 << (colorIndex % 64);
		mPackedColorCache[colorIndex].mIsValid = false;
	}
}

void Palette::setPaletteEntryPacked(uint16 colorIndex, uint32 color, uint16 packedColor)
{
	if (mColor[colorIndex] != color)
	{
		mColor[colorIndex] = color;
		mChangeFlags[colorIndex / 64] |= (uint64)1 << (colorIndex % 64);
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


Color PaletteManager::unpackColor(uint16 packedColor)
{
	// Differentiate between:
	//  - Original hardware's packed colors (9-bit): 0000 BBB0 GGG0 RRR0
	//  - Extended packed colors (15-bit):           1BBB BBGG GGGR RRRR

	// Note that extended packed colors can be matched to the original packed colors when not using the lowermost 2 bits of each channel
	//  -> That also means when using these bits as well, they can even go higher than pure white at 0xff, but they will get clamped at that point

	uint32 color = 0xff000000;
	uint8& r = ((uint8*)&color)[0];
	uint8& g = ((uint8*)&color)[1];
	uint8& b = ((uint8*)&color)[2];
	if (packedColor & 0x8000)
	{
		r = (uint8)std::min(((packedColor) & 0x1f) * 0x09, 0xff);
		g = (uint8)std::min(((packedColor >> 5) & 0x1f) * 0x09, 0xff);
		b = (uint8)std::min(((packedColor >> 10) & 0x1f) * 0x09, 0xff);
	}
	else
	{
		r = ((packedColor >> 1) & 0x07) * 0x24;
		g = ((packedColor >> 5) & 0x07) * 0x24;
		b = ((packedColor >> 9) & 0x07) * 0x24;
	}
	return Color::fromABGR32(color);
}

void PaletteManager::preFrameUpdate()
{
	mSplitPositionY = 0xffff;
	mUsesGlobalComponentTint = false;
	mGlobalComponentTintColor = Color::WHITE;
	mGlobalComponentAddedColor = Color::TRANSPARENT;
}

Palette& PaletteManager::getPalette(int paletteIndex)
{
	return mPalette[(paletteIndex >= 0 && paletteIndex < 2) ? paletteIndex : 0];
}

const Palette& PaletteManager::getPalette(int paletteIndex) const
{
	return mPalette[(paletteIndex >= 0 && paletteIndex < 2) ? paletteIndex : 0];
}

void PaletteManager::writePaletteEntry(int paletteIndex, uint16 colorIndex, uint32 color)
{
	RMX_CHECK(colorIndex < Palette::NUM_COLORS, "Invalid color index " << colorIndex, return);
	if (paletteIndex == 0)
	{
		mPalette[0].setPaletteEntry(colorIndex, color);
	}

	// Secondary palette gets written in both cases
	mPalette[1].setPaletteEntry(colorIndex, color);
}

void PaletteManager::writePaletteEntryPacked(int paletteIndex, uint16 colorIndex, uint16 packedColor)
{
	RMX_CHECK(colorIndex < Palette::NUM_COLORS, "Invalid color index " << colorIndex, return);

	// Differentiate between:
	//  - Original hardware's packed colors (9-bit): 0000 BBB0 GGG0 RRR0
	//  - Extended packed colors (15-bit):           1BBB BBGG GGGR RRRR

	// Note that extended packed colors can be matched to the original packed colors when not using the lowermost 2 bits of each channel
	//  -> That also means when using these bits as well, they can even go higher than pure white at 0xff, but they will get clamped at that point

	Palette::PackedPaletteColor& cache = mPalette[paletteIndex].mPackedColorCache[colorIndex];
	if (cache.mPackedColor == packedColor && cache.mIsValid)
	{
		// Nothing to do... well except if this is the primary palette, then we should still make sure the secondary palette has the same color
		if (paletteIndex == 0)
		{
			mPalette[1].setPaletteEntryPacked(colorIndex, mPalette[0].getEntry(colorIndex), packedColor);
		}
		return;
	}

	unsigned int color = (colorIndex & 0x0f) ? 0xff000000 : 0;
	unsigned int packed = (unsigned int)packedColor;
	if (packed & 0x8000)
	{
		static const unsigned int CONV[0x20] = { 0, 9, 18, 27, 36, 45, 54, 63, 72, 81, 90, 99, 108, 117, 126, 135, 144, 153, 162, 171, 180, 189, 198, 207, 216, 225, 234, 243, 252, 255, 255, 255 };
		color = color | (CONV[((packed))       & 0x1f])
					  | (CONV[((packed) >> 5)  & 0x1f] << 8)
					  | (CONV[((packed) >> 10) & 0x1f] << 16);
	}
	else
	{
		static const unsigned int CONV[8] = { 0, 36, 72, 108, 144, 180, 216, 252 };
		color = color | (CONV[(packed >> 1) & 0x07])
					  | (CONV[(packed >> 5) & 0x07] << 8)
					  | (CONV[(packed >> 9) & 0x07] << 16);
	}

	if (paletteIndex == 0)
	{
		mPalette[0].setPaletteEntryPacked(colorIndex, color, packedColor);
	}

	// Secondary palette gets written in both cases
	mPalette[1].setPaletteEntryPacked(colorIndex, color, packedColor);
}

void PaletteManager::resetAllPaletteChangeFlags()
{
	for (int i = 0; i < 2; ++i)
		mPalette[i].resetAllPaletteChangeFlags();
}

void PaletteManager::setAllPaletteChangeFlags()
{
	for (int i = 0; i < 2; ++i)
		mPalette[i].setAllPaletteChangeFlags();
}

void PaletteManager::setPaletteSplitPositionY(uint8 py)
{
	mSplitPositionY = py;
}

void PaletteManager::setGlobalComponentTint(const Vec4f& tintColor, const Vec4f& addedColor)
{
	mUsesGlobalComponentTint = true;
	mGlobalComponentTintColor = tintColor;
	mGlobalComponentAddedColor = addedColor;
}

void PaletteManager::applyGlobalComponentTint(Color& color) const
{
	if (mUsesGlobalComponentTint)
	{
		color = mGlobalComponentAddedColor + color * mGlobalComponentTintColor;
	}
}

void PaletteManager::applyGlobalComponentTint(Color& tintColor, Color& addedColor) const
{
	if (mUsesGlobalComponentTint)
	{
		tintColor *= mGlobalComponentTintColor;
		addedColor += mGlobalComponentAddedColor;
	}
}

void PaletteManager::serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	if (formatVersion >= 4)
	{
		serializePalette(serializer, mPalette[0]);
		serializePalette(serializer, mPalette[1]);
	}
	else
	{
		uint16 buffer[0x40];
		if (serializer.isReading())
		{
			serializer.serialize(buffer, 0x80);
			for (int i = 0; i < 0x40; ++i)
			{
				writePaletteEntryPacked(0, i, buffer[i]);
				writePaletteEntryPacked(1, i, buffer[i]);
			}
		}
		else
		{
			for (int i = 0; i < 0x40; ++i)
				buffer[i] = getPalette(0).getEntryPacked(i, true);
			serializer.serialize(buffer, 0x80);
		}
	}
}

void PaletteManager::serializePalette(VectorBinarySerializer& serializer, Palette& palette)
{
	for (size_t k = 0; k < Palette::NUM_COLORS; ++k)
	{
		serializer.serialize(palette.mColor[k]);

		if (serializer.isReading())
		{
			palette.setAllPaletteChangeFlags();
			palette.invalidatePackedColorCache();
		}
	}
}
