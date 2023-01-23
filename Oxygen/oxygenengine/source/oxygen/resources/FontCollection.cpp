/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/rendering/RenderResources.h"


Font* FontCollection::getFontByKey(uint64 keyHash)
{
	// Try to find in map
	CollectedFont* collectedFont = mapFind(mCollectedFonts, keyHash);
	return (nullptr != collectedFont) ? &collectedFont->mUnmodifiedFont : nullptr;
}

void FontCollection::reloadAll()
{
	// Load main game fonts
	mCollectedFonts.clear();
	loadDefinitionsFromPath(L"data/font/", nullptr);

	collectFromMods();
}

void FontCollection::collectFromMods()
{
	// Remove all definitions previously collected from mods, but not the main game ones
	for (auto& [key, collectedFont] : mCollectedFonts)
	{
		for (int index = (int)collectedFont.mDefinitions.size()-1; index >= 0; --index)
		{
			if (nullptr != collectedFont.mDefinitions[index].mMod)
			{
				// Update the index of the loaded definition beforehand
				if (collectedFont.mLoadedDefinitionIndex >= index)
				{
					if (collectedFont.mLoadedDefinitionIndex == index)
						collectedFont.mLoadedDefinitionIndex = -1;
					else
						--collectedFont.mLoadedDefinitionIndex;
				}

				// Remove that definition
				collectedFont.mDefinitions.erase(collectedFont.mDefinitions.begin() + index);
				--index;
			}
		}
	}

	// Scan for font definitions in mods
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadDefinitionsFromPath(mod->mFullPath + L"font/", mod);
	}

	// Load the fonts
	updateLoadedFonts();
}

void FontCollection::registerManagedFont(Font& font, const std::string& key)
{
	const uint64 keyHash = rmx::getMurmur2_64(key);
	CollectedFont* collectedFont = mapFind(mCollectedFonts, keyHash);
	if (nullptr != collectedFont)
	{
		collectedFont->mManagedFonts.push_back(&font);

		// Update the font source in all font instances (note that it might also be a null pointer)
		for (Font* managedFont : collectedFont->mManagedFonts)
		{
			managedFont->injectFontSource(collectedFont->mFontSource);
		}
	}
}

void FontCollection::loadDefinitionsFromPath(std::wstring_view path, const Mod* mod)
{
	if (!FTX::FileSystem->exists(path))
		return;

	std::vector<rmx::FileIO::FileEntry> fileEntries;
	fileEntries.reserve(8);
	FTX::FileSystem->listFilesByMask(std::wstring(path) + L"*.json", false, fileEntries);

	for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
	{
		const std::wstring pureFilename = fileEntry.mFilename.substr(0, fileEntry.mFilename.length() - 5);	// Remove ".json"
		const std::string keyString = WString(pureFilename).toStdString();
		const uint64 keyHash = rmx::getMurmur2_64(keyString);

		// Get or create the collected font for this key
		CollectedFont& collectedFont = mCollectedFonts[keyHash];
		if (collectedFont.mManagedFonts.empty())	// This is true of the map entry was just created
		{
			// The list of managed font must always include the unmodified font
			collectedFont.mManagedFonts.push_back(&collectedFont.mUnmodifiedFont);
			collectedFont.mKeyHash = keyHash;
			collectedFont.mKeyString = keyString;
		}

		// Add new definition
		Definition& definition = vectorAdd(collectedFont.mDefinitions);
		definition.mDefinitionFile = fileEntry.mPath + fileEntry.mFilename;
		definition.mMod = mod;
	}
}

void FontCollection::updateLoadedFonts()
{
	std::vector<uint64> keysToRemove;
	for (auto& [key, collectedFont] : mCollectedFonts)
	{
		if (collectedFont.mDefinitions.empty() && collectedFont.mManagedFonts.size() <= 1)
		{
			// Font is unused and can be removed
			keysToRemove.push_back(key);
			continue;
		}

		// Nothing to do if the last definition (= the highest priority one) is already loaded
		if (collectedFont.mDefinitions.size() == (size_t)collectedFont.mLoadedDefinitionIndex + 1)
			continue;

		// Font source needs to be reloaded
		collectedFont.mLoadedDefinitionIndex = -1;
		SAFE_DELETE(collectedFont.mFontSource);

		// Start at the end of the definitions list, at those have the highest priority
		for (int index = (int)collectedFont.mDefinitions.size() - 1; index >= 0; --index)
		{
			const Definition& definition = collectedFont.mDefinitions[index];
			collectedFont.mFontSource = new FontSourceBitmap(WString(definition.mDefinitionFile).toString());
			if (collectedFont.mFontSource->isValid())
			{
				collectedFont.mLoadedDefinitionIndex = index;
				break;
			}
			// If loading failed, try the next definition
		}

		// Update the font source in all font instances (note that it might also be a null pointer)
		for (Font* font : collectedFont.mManagedFonts)
		{
			font->injectFontSource(collectedFont.mFontSource);
		}

		// If loading failed for all definitions, remove the collected font instance
		if (collectedFont.mLoadedDefinitionIndex == -1 && collectedFont.mManagedFonts.size() <= 1)
		{
			keysToRemove.push_back(key);
		}
	}

	for (uint64 key : keysToRemove)
	{
		mCollectedFonts.erase(key);
	}

	// Invalidate cached printed texts
	RenderResources::instance().mPrintedTextCache.clear();
}
