/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/rendering/utils/ComponentSprite.h"
#include "oxygen/rendering/utils/PaletteSprite.h"

class SpriteDump;


class SpriteCache : public SingleInstance<SpriteCache>
{
public:
	struct CacheItem
	{
		bool mUsesComponentSprite = false;
		SpriteBase* mSprite = nullptr;
		uint32 mChangeCounter = 0;
		bool mGotDumped = false;
	};

	enum ROMSpriteEncoding
	{
		ENCODING_NONE		= 0,
		ENCODING_CHARACTER	= 1,
		ENCODING_OBJECT		= 2,
		ENCODING_KOSINSKI	= 3
	};

public:
	SpriteCache();
	~SpriteCache();

	void clear();
	void loadAllSpriteDefinitions();

	bool hasSprite(uint64 key) const;
	const CacheItem* getSprite(uint64 key);
	CacheItem& getOrCreatePaletteSprite(uint64 key);
	CacheItem& getOrCreateComponentSprite(uint64 key);

	uint64 setupSpriteFromROM(uint32 patternsBaseAddress, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex, ROMSpriteEncoding encoding, int16 indexOffset = 0);

	SpriteDump& getSpriteDump();
	void dumpSprite(uint64 key, const std::string& categoryKey, uint8 spriteNumber, uint8 atex);

private:
	struct SheetCache
	{
		std::map<std::wstring, PaletteBitmap> mPaletteSpriteSheets;
		std::map<std::wstring, Bitmap> mComponentSpriteSheets;
	};

private:
	void loadSpriteDefinitions(const std::wstring& path);

private:
	std::unordered_map<uint64, CacheItem> mCachedSprites;
	SpriteDump* mSpriteDump = nullptr;
	uint32 mGlobalChangeCounter = 0;
};
