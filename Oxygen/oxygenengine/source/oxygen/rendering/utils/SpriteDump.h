/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class PaletteSprite;


class SpriteDump
{
public:
	void load();
	void save();

	void addSprite(const PaletteSprite& paletteSprite, const std::string& categoryName, uint8 spriteNumber, uint8 atex);
	void addSpriteWithTranslation(const PaletteSprite& paletteSprite, const std::string& categoryName, uint8 spriteNumber, uint8 atex);

private:
	struct Category;

	Category& getOrCreateCategory(const std::string& categoryName);
	void saveSpriteAtlas(const std::string& categoryName);

private:
	struct Entry
	{
		uint8 mSpriteNumber = 0;
		Vec2i mSize;
		Vec2i mOffset;
	};
	struct Category
	{
		std::string mName;
		std::map<uint8, Entry> mEntries;
		bool mChanged = false;
	};
	std::map<uint64, Category> mCategories;
	bool mAnyChange = false;
};
