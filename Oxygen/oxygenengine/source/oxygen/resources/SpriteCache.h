/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/rendering/sprite/ComponentSprite.h"
#include "oxygen/rendering/sprite/PaletteSprite.h"

class EmulatorInterface;
class SpriteDump;


class SpriteCache : public SingleInstance<SpriteCache>
{
public:
	struct CacheItem
	{
		uint64 mKey = 0;
		bool mUsesComponentSprite = false;
		SpriteBase* mSprite = nullptr;
		uint32 mChangeCounter = 0;
		CacheItem* mRedirect = nullptr;
		bool mGotDumped = false;
	};

	enum class ROMSpriteEncoding : uint8
	{
		NONE		= 0,
		CHARACTER	= 1,
		OBJECT		= 2,
		KOSINSKI	= 3
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

	uint64 setupSpriteFromROM(EmulatorInterface& emulatorInterface, uint32 patternsBaseAddress, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, uint8 atex, ROMSpriteEncoding encoding, int16 indexOffset = 0);

	void clearRedirect(uint64 sourceKey);
	void setupRedirect(uint64 sourceKey, uint64 targetKey);

	inline uint32 getGlobalChangeCounter() const  { return mGlobalChangeCounter; }

	SpriteDump& getSpriteDump();
	void dumpSprite(uint64 key, std::string_view categoryKey, uint8 spriteNumber, uint8 atex);

private:
	CacheItem& createCacheItem(uint64 key);
	void loadSpriteDefinitions(const std::wstring& path);

private:
	std::unordered_map<uint64, CacheItem> mCachedSprites;
	SpriteDump* mSpriteDump = nullptr;
	uint32 mGlobalChangeCounter = 0;
};
