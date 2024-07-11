/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/PaletteCollection.h"
#include "oxygen/application/modding/ModManager.h"


const PaletteBase* PaletteCollection::getPalette(uint64 key, uint8 line) const
{
	return mapFind(mPalettes, key + line);
}

void PaletteCollection::clear()
{
	mPalettes.clear();
}

void PaletteCollection::loadPalettes()
{
	// Load or reload palettes
	clear();
	loadPalettesInDirectory(L"data/palettes", false);
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadPalettesInDirectory(mod->mFullPath + L"palettes", true);
	}
}

void PaletteCollection::loadPalettesInDirectory(const std::wstring& path, bool isModded)
{
	BitFlagSet<PaletteBase::Properties> properties = makeBitFlagSet(PaletteBase::Properties::READ_ONLY);
	if (isModded)
		properties.set(PaletteBase::Properties::MODDED);

	// Load palettes from the given path
	std::vector<rmx::FileIO::FileEntry> fileEntries;
	fileEntries.reserve(8);
	FTX::FileSystem->listFilesByMask(path + L"/*.png", true, fileEntries);
	for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
	{
		if (!FTX::FileSystem->exists(fileEntry.mPath + fileEntry.mFilename))
			continue;

		std::vector<uint8> content;
		if (!FTX::FileSystem->readFile(fileEntry.mPath + fileEntry.mFilename, content))
			continue;

		Bitmap bitmap;
		if (!bitmap.load(fileEntry.mPath + fileEntry.mFilename))
		{
			RMX_ERROR("Failed to load PNG at '" << *WString(fileEntry.mPath + fileEntry.mFilename).toString() << "'", );
			continue;
		}

		String name = WString(fileEntry.mFilename).toString();
		name.remove(name.length() - 4, 4);

		uint64 key = rmx::getMurmur2_64(name);		// Hash is the key of the first palette, the others are enumerated from there
		const int numLines = std::min(bitmap.getHeight(), 64);
		const int numColorsPerLine = std::min(bitmap.getWidth(), 64);

		for (int line = 0; line < numLines; ++line)
		{
			PaletteBase& palette = mPalettes[key + line];
			palette.initPalette(numColorsPerLine, properties);
			palette.writeRawColors(bitmap.getPixelPointer(0, line), numColorsPerLine);
		}
	}
}
