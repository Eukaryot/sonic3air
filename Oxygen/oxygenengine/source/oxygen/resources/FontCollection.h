/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>

class Mod;


class FontCollection : public SingleInstance<FontCollection>
{
public:
	struct Definition
	{
		std::wstring mDefinitionFile;
		const Mod* mMod = nullptr;
	};

	struct CollectedFont
	{
		std::string mKeyString;
		uint64 mKeyHash = 0;
		std::vector<Definition> mDefinitions;	// There can be multiple font definitions with the same key, thanks to overloading

		int mLoadedDefinitionIndex = -1;
		FontSourceBitmap* mFontSource = nullptr;
		Font mUnmodifiedFont;
		std::vector<Font*> mManagedFonts;
	};

	struct ManagedFont
	{
		Font* mFont = nullptr;
		std::string mKey;
	};

public:
	~FontCollection();

	Font* getFontByKey(uint64 keyHash);
	Font* createFontByKey(std::string_view key);

	bool registerManagedFont(Font& font, std::string_view key);

	void clear();
	void reloadAll();
	void collectFromMods();

private:
	void registerManagedFontInternal(Font& font, CollectedFont& collectedFont);
	void loadDefinitionsFromPath(std::wstring_view path, const Mod* mod);
	void updateLoadedFonts();

private:
	std::unordered_map<uint64, CollectedFont> mCollectedFonts;	// Includes all the fonts that were loaded from files; using "mKeyHash" as map key
	std::unordered_map<uint64, Font*> mFontsByKeyHash;			// Includes unmodified collected fonts and run-time created fonts (though not the managed fonts); using the font's key hash as map key
	ObjectPool<Font> mFontPool;
	std::vector<ManagedFont> mAllManagedFonts;
};
