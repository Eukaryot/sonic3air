/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/palette/Palette.h"


class PaletteCollection : public SingleInstance<PaletteCollection>
{
public:
	const PaletteBase* getPalette(uint64 key, uint8 line) const;

	void clear();
	void loadPalettes();

private:
	void loadPalettesInDirectory(const std::wstring& path, bool isModded);
	void addSpritePalettes();

private:
	std::unordered_map<uint64, PaletteBase> mPalettes;
	std::unordered_map<uint64, PaletteBase*> mRedirections;
};
