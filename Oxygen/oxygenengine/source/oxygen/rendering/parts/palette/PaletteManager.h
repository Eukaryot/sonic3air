/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/palette/Palette.h"


class PaletteManager
{
public:
	static const size_t MAIN_PALETTE_SIZE = 512;

public:
	static Color unpackColor(uint16 packedColor);

public:
	PaletteManager();

	void preFrameUpdate();

	Palette& getMainPalette(int paletteIndex);
	const Palette& getMainPalette(int paletteIndex) const;

	void writePaletteEntry(int paletteIndex, uint16 colorIndex, uint32 color);
	void writePaletteEntryPacked(int paletteIndex, uint16 colorIndex, uint16 packedColor);

	inline Color getBackdropColor() const  { return mMainPalette[0].getColor(mBackdropColorIndex); }
	inline void setBackdropColorIndex(uint16 paletteIndex)  { mBackdropColorIndex = paletteIndex; }

	void setPaletteSplitPositionY(int py);

	inline bool usesGlobalComponentTint() const  { return mUsesGlobalComponentTint; }
	inline const Vec4f& getGlobalComponentTintColor() const  { return mGlobalComponentTintColor; }
	inline const Vec4f& getGlobalComponentAddedColor() const { return mGlobalComponentAddedColor; }
	void setGlobalComponentTint(const Vec4f& tintColor, const Vec4f& addedColor);

	void applyGlobalComponentTint(Vec4f& color) const;
	void applyGlobalComponentTint(Vec4f& tintColor, Vec4f& addedColor) const;

	void serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion);

public:
	int mSplitPositionY = 0x1fff;	// Use some large value as default that is definitely larger than any responsible screen height; not not too high, as that can cause graphics issues on some mobile devices

private:
	Palette mMainPalette[2];		// [0] = Standard palette, [1] = Underwater palette (in S3AIR)
	uint16 mBackdropColorIndex = 0;

	bool mUsesGlobalComponentTint = false;
	Vec4f mGlobalComponentTintColor = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);	// Not using the Color class to be able to have negative channel values as well (especially for added color)
	Vec4f mGlobalComponentAddedColor = Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
};
