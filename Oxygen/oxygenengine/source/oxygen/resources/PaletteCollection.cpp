/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/PaletteCollection.h"
#include "oxygen/resources/SpriteCollection.h"
#include "oxygen/application/modding/ModManager.h"


void PaletteCollection::clear()
{
	mPalettes.clear();
	mRedirections.clear();
	++mGlobalChangeCounter;
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

	addSpritePalettes();
}

const PaletteBase* PaletteCollection::getPalette(uint64 key, uint8 line) const
{
	const PaletteBase* palette = mapFind(mPalettes, key + line);
	if (nullptr != palette)
		return palette;

	PaletteBase*const* palettePtr = mapFind(mRedirections, key + line);
	if (nullptr != palettePtr)
		return *palettePtr;

	return nullptr;
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
		{
			RMX_ERROR("Failed to load PNG at '" << *WString(fileEntry.mPath + fileEntry.mFilename).toString() << "': File loading failed", );
			continue;
		}

		Bitmap bitmap;
		if (!bitmap.load(fileEntry.mPath + fileEntry.mFilename))
		{
			RMX_ERROR("Failed to load PNG at '" << *WString(fileEntry.mPath + fileEntry.mFilename).toString() << "': Format not supported", );
			continue;
		}

		String name = WString(fileEntry.mFilename).toString();
		name.remove(name.length() - 4, 4);

		const uint64 paletteKey = rmx::getMurmur2_64(name);		// Hash is the key of the first palette, the others are enumerated from there
		const int numLines = std::min(bitmap.getHeight(), 64);
		const int numColorsPerLine = std::min(bitmap.getWidth(), 64);

		for (int line = 0; line < numLines; ++line)
		{
			PaletteBase& palette = mPalettes[paletteKey + line];
			palette.initPalette(paletteKey + line, numColorsPerLine, properties, name);
			palette.writeRawColors(bitmap.getPixelPointer(0, line), numColorsPerLine);
		}
	}
}

void PaletteCollection::addSpritePalettes()
{
	std::unordered_map<uint64, SpriteCollection::SpritePalette>& spritePalettes = SpriteCollection::instance().mSpritePalettes;
	std::unordered_map<uint64, uint64>& redirections = SpriteCollection::instance().mPaletteRedirections;

	// TODO: The properties flags set here assume that all sprite palettes are modded, but this might not actually be the case
	const BitFlagSet<PaletteBase::Properties> properties = makeBitFlagSet(PaletteBase::Properties::READ_ONLY, PaletteBase::Properties::MODDED);

	// Copy palettes
	for (const auto& pair : spritePalettes)
	{
		const SpriteCollection::SpritePalette& spritePalette = pair.second;
		const std::vector<uint32>& colors = spritePalette.mColors;
		if (colors.empty())
			continue;

		const uint64 paletteKey = pair.first;
		PaletteBase& palette = mPalettes[paletteKey];
		palette.initPalette(paletteKey, colors.size(), properties, (nullptr != spritePalette.mItem) ? spritePalette.mItem->mSourceInfo.mSourceIdentifier : "");
		palette.writeRawColors(&colors[0], colors.size());
	}

	// Copy and resolve redirections
	for (const std::pair<uint64, uint64>& pair : redirections)
	{
		PaletteBase* target = mapFind(mPalettes, pair.second);
		RMX_ASSERT(nullptr != target, "Broken redirection");
		if (nullptr != target)
		{
			mRedirections[pair.first] = target;
		}
	}

	// Clear palettes in sprite cache now that we loaded them all
	spritePalettes.clear();
	redirections.clear();
}
