/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


// General info on palette usage:
//  - Entries 0x00...0x3f are used normally, as with VDP
//     -> Arranged in 4 sub-palettes of 16 entries each
//     -> Used by planes, VDP sprites and custom palette sprites
//  - Bit 0x40 is reserved as priority bit
//  - Bit 0x80 is used to switch to a different set of colors -> 0x80...0xbf
//     -> These also support the priority bit 0x40
//     -> Can be used by custom palette sprites only


class PaletteManager
{
public:
	static Color unpackColor(uint16 packedColor);

public:
	void preFrameUpdate();

	const uint32* getPalette(int paletteIndex) const;
	void getPalette(Color* palette, int paletteIndex) const;
	Color getPaletteEntry(int paletteIndex, uint8 colorIndex) const;
	uint16 getPaletteEntryPacked(int paletteIndex, uint8 colorIndex, bool allowExtendedPacked = false) const;
	void writePaletteEntry(int paletteIndex, uint8 colorIndex, uint32 color);
	void writePaletteEntryPacked(int paletteIndex, uint8 colorIndex, uint16 packedColor);

	inline Color getBackdropColor() const  { return Color::fromABGR32(mPalette[0][mBackdropColorIndex]); }
	inline void setBackdropColorIndex(uint8 paletteIndex)  { mBackdropColorIndex = paletteIndex; }

	void setPaletteSplitPositionY(uint8 py);

	inline bool usesGlobalComponentTint() const  { return mUsesGlobalComponentTint; }
	inline const Vec4f& getGlobalComponentTintColor() const  { return mGlobalComponentTintColor; }
	inline const Vec4f& getGlobalComponentAddedColor() const { return mGlobalComponentAddedColor; }
	void setGlobalComponentTint(const Vec4f& tintColor, const Vec4f& addedColor);

public:
	uint8 mSplitPositionY = 0xff;

private:
	struct PackedPaletteColor
	{
		uint16 mPackedColor = 0;
		bool mIsValid = false;
	};

private:
	uint32 mPalette[2][0x100] = { 0 };	// [0] = Standard palette, [1] = Underwater palette (in S3AIR)
	PackedPaletteColor mPackedPaletteCache[2][0x100] = { 0 };	// Only used as an optimization
	uint8 mBackdropColorIndex = 0;

	bool mUsesGlobalComponentTint = false;
	Vec4f mGlobalComponentTintColor = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);	// Not using the Color class to be able to have negative channel values as well (especially for added color)
	Vec4f mGlobalComponentAddedColor = Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
};
