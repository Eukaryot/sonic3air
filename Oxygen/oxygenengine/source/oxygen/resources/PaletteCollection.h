/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class PaletteCollection : public SingleInstance<PaletteCollection>
{
public:
	struct Palette
	{
		std::vector<uint32> mColors = { 0 };	// Colors in the palette, using ABGR32 format
		bool mIsModded = false;
	};

public:
	const Palette* getPalette(uint64 key, uint8 line) const;

	void clear();
	void loadPalettes();

private:
	void loadPalettesInDirectory(const std::wstring& path, bool isModded);

private:
	std::unordered_map<uint64, Palette> mPalettes;
};
