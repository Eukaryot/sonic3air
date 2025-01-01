/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/rendering/sprite/ComponentSprite.h"
#include "oxygen/rendering/sprite/PaletteSprite.h"

class EmulatorInterface;
class Mod;
class SpriteDump;


class SpriteCollection : public SingleInstance<SpriteCollection>
{
friend class PaletteCollection;	// For direct access to mSpritePalettes

public:
	enum class ROMSpriteEncoding : uint8
	{
		NONE		= 0,
		CHARACTER	= 1,
		OBJECT		= 2,
		KOSINSKI	= 3
	};

	struct ROMSpriteData
	{
		uint32 mPatternsBaseAddress = 0;
		uint32 mTableAddress = 0;
		uint32 mMappingOffset = 0;
		uint8  mAnimationSprite = 0;
		ROMSpriteEncoding mEncoding = ROMSpriteEncoding::NONE;
		int16  mIndexOffset = 0;

		void serialize(VectorBinarySerializer& serializer);
		uint64 getKey() const;
	};

	struct SourceInfo
	{
		enum class Type : uint8
		{
			UNKNOWN,
			SPRITE_FILE,
			ROM_DATA
		};

		Type mType = Type::UNKNOWN;
		ROMSpriteData mROMSpriteData;	// Only for type ROM_DATA
		std::string mSourceIdentifier;	// Only for type SPRITE_FILE
		const Mod* mMod = nullptr;		// Only for type SPRITE_FILE
	};

	struct Item
	{
		uint64 mKey = 0;
		bool mUsesComponentSprite = false;
		SpriteBase* mSprite = nullptr;
		uint32 mChangeCounter = 0;
		Item* mRedirect = nullptr;
		SourceInfo mSourceInfo;
		bool mGotDumped = false;
	};

public:
	SpriteCollection();
	~SpriteCollection();

	void clear();
	void loadAllSpriteDefinitions();

	bool hasSprite(uint64 key) const;
	const Item* getSprite(uint64 key);
	Item& getOrCreatePaletteSprite(uint64 key);
	Item& getOrCreateComponentSprite(uint64 key);

	inline const std::unordered_map<uint64, Item>& getAllSprites() const  { return mSpriteItems; }

	Item& setupSpriteFromROM(EmulatorInterface& emulatorInterface, const ROMSpriteData& romSpriteData, uint8 atex);

	void clearRedirect(uint64 sourceKey);
	void setupRedirect(uint64 sourceKey, uint64 targetKey);

	inline uint32 getGlobalChangeCounter() const  { return mGlobalChangeCounter; }

	SpriteDump& getSpriteDump();
	void dumpSprite(uint64 key, std::string_view categoryKey, uint8 spriteNumber, uint8 atex);

private:
	struct SpritePalette
	{
		const Item* mItem = nullptr;
		std::vector<uint32> mColors;
	};

private:
	Item& createItem(uint64 key);
	void loadSpriteDefinitions(const std::wstring& path, const Mod* mod);
	void addSpritePalette(uint64 paletteKey, const Item& item, std::vector<uint32>& palette);

private:
	std::unordered_map<uint64, Item> mSpriteItems;

	std::unordered_map<uint64, SpritePalette> mSpritePalettes;
	std::unordered_map<uint64, uint64> mPaletteRedirections;

	SpriteDump* mSpriteDump = nullptr;
	uint32 mGlobalChangeCounter = 0;
};
