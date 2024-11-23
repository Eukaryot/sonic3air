/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/SpriteCollection.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/rendering/sprite/SpriteDump.h"
#include "oxygen/rendering/utils/Kosinski.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptRuntime.h"

#ifdef DEBUG
	// Notes on sprite dumping:
	//  - Sprite dumping will not replace existing files in "___internal/analysis/spritedump", so make sure to first remove all files / folders that you want to dump again
	//  - In order to match the image sizes of an old sprite dump again, you might want to temporarily remove the rounding to next multiple of 4, inside "PaletteSprite::createFromSpritePatterns"
	//#define CREATE_SPRITEDUMP
#endif


namespace
{

	void decodeROMSpriteData(EmulatorInterface& emulatorInterface, std::vector<RenderUtils::PatternPixelContent>& patternBuffer, uint32 patternsBaseAddress, uint32 patternAddress, SpriteCollection::ROMSpriteEncoding encoding)
	{
		switch (encoding)
		{
			case SpriteCollection::ROMSpriteEncoding::NONE:
			{
				// Uncompressed / unpacked data
				const uint16 numPatterns = patternAddress;
				RenderUtils::expandMultiplePatternDataFromROM(patternBuffer, emulatorInterface.getMemoryPointer(patternsBaseAddress, false, numPatterns * 0x20), numPatterns);
				break;
			}

			case SpriteCollection::ROMSpriteEncoding::CHARACTER:
			{
				// Variant for character sprites
				int numSprites = emulatorInterface.readMemory16(patternAddress);
				uint32 address = patternAddress + 2;

				for (int i = 0; i < numSprites; ++i)
				{
					const uint16 data = emulatorInterface.readMemory16(address);
					address += 2;

					uint32 src = patternsBaseAddress + (data & 0x0fff) * 0x20;
					const uint16 numPatterns = ((data & 0xf000) >> 12) + 1;

					RenderUtils::expandMultiplePatternDataFromROM(patternBuffer, emulatorInterface.getRom() + src, numPatterns);
				}
				break;
			}

			case SpriteCollection::ROMSpriteEncoding::OBJECT:
			{
				// Variant for other object sprites
				int numSprites = emulatorInterface.readMemory16(patternAddress) + 1;
				uint32 address = patternAddress + 2;

				for (int i = 0; i < numSprites; ++i)
				{
					const uint16 data = emulatorInterface.readMemory16(address);
					address += 2;

					uint32 src = patternsBaseAddress + (data & 0x7ff0) * 2;
					const uint16 numPatterns = (data & 0x000f) + 1;

					RenderUtils::expandMultiplePatternDataFromROM(patternBuffer, emulatorInterface.getRom() + src, numPatterns);
				}
				break;
			}

			case SpriteCollection::ROMSpriteEncoding::KOSINSKI:
			{
				// Using Kosinski compressed data
				uint8 buffer[0x1000];

				// Get the decompressed size
				uint16 size = emulatorInterface.readMemory16(patternsBaseAddress);
				if (size == 0xa000)
					size = 0x8000;
				uint32 inputAddress = patternsBaseAddress + 2;

				while (size > 0)
				{
					uint8* pointer = buffer;
					Kosinski::decompress(emulatorInterface, pointer, inputAddress);

					const uint16 bytes = std::min<uint16>(size, 0x1000);
					RMX_ASSERT((bytes & 0x1f) == 0, "Expected decompressed data size to be divisible by 0x20");
					RenderUtils::expandMultiplePatternDataFromROM(patternBuffer, buffer, bytes / 0x20);

					if (size < 0x1000)
						break;

					size -= bytes;
					inputAddress += 8;	// This is needed, but why...?
				}
				break;
			}
		}
	}

	void createPaletteSpriteFromROM(EmulatorInterface& emulatorInterface, PaletteSprite& paletteSprite, uint32 patternsBaseAddress, uint32 patternAddress, uint32 mappingAddress, SpriteCollection::ROMSpriteEncoding encoding, int16 indexOffset)
	{
		// Fill sprite pattern buffer
		static std::vector<RenderUtils::PatternPixelContent> patternBuffer;
		patternBuffer.clear();
		decodeROMSpriteData(emulatorInterface, patternBuffer, patternsBaseAddress, patternAddress, encoding);

		// Fill sprite patterns
		static std::vector<RenderUtils::SinglePattern> patterns;
		patterns.clear();
		if (!patternBuffer.empty())
		{
			EmulatorInterface& emulatorInterface = EmulatorInterface::instance();
			const int count = emulatorInterface.readMemory16(mappingAddress);
			uint32 address = mappingAddress + 2;

			for (int i = 0; i < count; ++i)
			{
				RenderUtils::fillPatternsFromSpriteData(patterns, emulatorInterface.getRom() + address, patternBuffer, indexOffset);
				address += 6;
			}
		}

		// Create the palette sprite
		paletteSprite.createFromSpritePatterns(patterns);
	}

	void createPaletteSpriteFromROM(EmulatorInterface& emulatorInterface, PaletteSprite& paletteSprite, uint32 patternsBaseAddress, uint32 tableAddress, uint32 mappingOffset, uint8 animationSprite, SpriteCollection::ROMSpriteEncoding encoding, int16 indexOffset)
	{
		const uint32 patternAddress = (encoding == SpriteCollection::ROMSpriteEncoding::NONE || encoding == SpriteCollection::ROMSpriteEncoding::KOSINSKI) ? tableAddress : (tableAddress + emulatorInterface.readMemory16(tableAddress + animationSprite * 2));
		const uint32 mappingAddress = mappingOffset + emulatorInterface.readMemory16(mappingOffset + animationSprite * 2);

		createPaletteSpriteFromROM(emulatorInterface, paletteSprite, patternsBaseAddress, patternAddress, mappingAddress, encoding, indexOffset);
	}

	bool isHexDigit(char ch)
	{
		return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
	}

}



void SpriteCollection::ROMSpriteData::serialize(VectorBinarySerializer& serializer)
{
	serializer.serialize(mPatternsBaseAddress);
	serializer.serialize(mTableAddress);
	serializer.serialize(mMappingOffset);
	serializer.serialize(mAnimationSprite);
	serializer.serializeAs<uint8>(mEncoding);
	serializer.serialize(mIndexOffset);
}

uint64 SpriteCollection::ROMSpriteData::getKey() const
{
	return (((uint64)mPatternsBaseAddress) << 42) ^ (((uint64)mTableAddress) << 25) ^ (((uint64)mMappingOffset) << 8) ^ (uint64)mAnimationSprite;
}


SpriteCollection::SpriteCollection()
{
}

SpriteCollection::~SpriteCollection()
{
	clear();

	if (nullptr != mSpriteDump)
	{
		mSpriteDump->save();
		SAFE_DELETE(mSpriteDump);
	}
}

void SpriteCollection::clear()
{
	// Delete the sprite instances
	for (auto& pair : mSpriteItems)
	{
		delete pair.second.mSprite;
	}
	mSpriteItems.clear();
	mSpritePalettes = SpritePalettes();
	++mGlobalChangeCounter;
}

void SpriteCollection::loadAllSpriteDefinitions()
{
	// Load or reload from all mods
	loadSpriteDefinitions(L"data/sprites", nullptr);
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadSpriteDefinitions(mod->mFullPath + L"sprites", mod);
	}
}

bool SpriteCollection::hasSprite(uint64 key) const
{
	return (mSpriteItems.count(key) != 0);
}

const SpriteCollection::Item* SpriteCollection::getSprite(uint64 key)
{
	Item* item = mapFind(mSpriteItems, key);
	if (nullptr != item)
	{
		// Resolve redirect
		while (nullptr != item->mRedirect)
		{
			item = item->mRedirect;
		}
		return item;
	}
	else
	{
		// Output an error
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			const std::string_view* str = LemonScriptRuntime::tryResolveStringHash(key);
			if (nullptr != str)
			{
				RMX_ERROR("Invalid sprite cache key with string '" << *str << "'", );
			}
			else
			{
				RMX_ERROR("Invalid sprite cache key with unknown hash " << rmx::hexString(key), );
			}
		}
		return nullptr;
	}
}

SpriteCollection::Item& SpriteCollection::getOrCreatePaletteSprite(uint64 key)
{
	Item* item = mapFind(mSpriteItems, key);
	if (nullptr != item)
	{
		RMX_CHECK(!item->mUsesComponentSprite, "Sprite is not a palette sprite", );
	}
	else
	{
		item = &createItem(key);
		item->mSprite = new PaletteSprite();
		item->mUsesComponentSprite = false;
	}
	return *item;
}

SpriteCollection::Item& SpriteCollection::getOrCreateComponentSprite(uint64 key)
{
	Item* item = mapFind(mSpriteItems, key);
	if (nullptr != item)
	{
		RMX_CHECK(item->mUsesComponentSprite, "Sprite is not a component sprite", );
	}
	else
	{
		item = &createItem(key);
		item->mSprite = new ComponentSprite();
		item->mUsesComponentSprite = true;
	}
	return *item;
}

SpriteCollection::Item& SpriteCollection::setupSpriteFromROM(EmulatorInterface& emulatorInterface, const ROMSpriteData& romSpriteData, uint8 atex)
{
	const uint64 key = romSpriteData.getKey();
	Item* item = mapFind(mSpriteItems, key);
	if (nullptr == item)
	{
		item = &getOrCreatePaletteSprite(key);
		item->mSourceInfo.mType = SourceInfo::Type::ROM_DATA;
		item->mSourceInfo.mROMSpriteData = romSpriteData;

		PaletteSprite& paletteSprite = *static_cast<PaletteSprite*>(item->mSprite);
		createPaletteSpriteFromROM(emulatorInterface, paletteSprite, romSpriteData.mPatternsBaseAddress, romSpriteData.mTableAddress, romSpriteData.mMappingOffset, romSpriteData.mAnimationSprite, romSpriteData.mEncoding, romSpriteData.mIndexOffset);

	#ifdef CREATE_SPRITEDUMP
		if (romSpriteData.mAnimationSprite != 0)	// TODO: Do this only for characters
		{
			String categoryKey(0, "%06x_%06x_%06x", romSpriteData.mPatternsBaseAddress, romSpriteData.mTableAddress, romSpriteData.mMappingOffset);
			getSpriteDump().addSpriteWithTranslation(paletteSprite, *categoryKey, romSpriteData.mAnimationSprite, atex);
			item->mGotDumped = true;
		}
	#endif
	}
	return *item;
}

void SpriteCollection::clearRedirect(uint64 sourceKey)
{
	Item* source = mapFind(mSpriteItems, sourceKey);
	if (nullptr != source)
	{
		source->mRedirect = nullptr;
	}
}

void SpriteCollection::setupRedirect(uint64 sourceKey, uint64 targetKey)
{
	Item* source = mapFind(mSpriteItems, sourceKey);
	if (nullptr == source)
	{
		source = &createItem(sourceKey);
	}

	Item* target = mapFind(mSpriteItems, targetKey);
	source->mRedirect = target;
}

SpriteDump& SpriteCollection::getSpriteDump()
{
	if (nullptr == mSpriteDump)
	{
		mSpriteDump = new SpriteDump();
		mSpriteDump->load();
	}
	return *mSpriteDump;
}

void SpriteCollection::dumpSprite(uint64 key, std::string_view categoryKey, uint8 spriteNumber, uint8 atex)
{
	Item* item = mapFind(mSpriteItems, key);
	if (nullptr != item && !item->mGotDumped)
	{
		if (!item->mUsesComponentSprite)
		{
			const PaletteSprite& paletteSprite = *static_cast<const PaletteSprite*>(item->mSprite);
			getSpriteDump().addSprite(paletteSprite, categoryKey, spriteNumber, atex);
		}
		else
		{
			RMX_ERROR("Can't dump component sprites (attempted to dump '" << categoryKey << "' sprite " << rmx::hexString(spriteNumber, 2) << ")", );
		}
		item->mGotDumped = true;
	}
}

SpriteCollection::Item& SpriteCollection::createItem(uint64 key)
{
	Item& item = mSpriteItems[key];
	item.mKey = key;
	item.mSprite = nullptr;
	item.mUsesComponentSprite = false;
	item.mChangeCounter = mGlobalChangeCounter;
	return item;
}

void SpriteCollection::loadSpriteDefinitions(const std::wstring& path, const Mod* mod)
{
	struct PaletteSpriteSheet
	{
		PaletteBitmap mBitmap;
		uint64 mFirstSpritePaletteKey = 0;
	};
	struct SheetCache
	{
		std::map<uint64, PaletteSpriteSheet> mPaletteSpriteSheets;
		std::map<uint64, Bitmap> mComponentSpriteSheets;
	};
	SheetCache sheetCache;

	std::vector<rmx::FileIO::FileEntry> fileEntries;
	fileEntries.reserve(8);
	FTX::FileSystem->listFilesByMask(path + L"/*.json", true, fileEntries);
	if (fileEntries.empty())
		return;

	std::vector<uint32> palette;

	++mGlobalChangeCounter;
	for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
	{
		const Json::Value spritesJson = JsonHelper::loadFile(fileEntry.mPath + fileEntry.mFilename);
		for (auto iterator = spritesJson.begin(); iterator != spritesJson.end(); ++iterator)
		{
			const String identifier(iterator.key().asString());
			uint64 key = 0;
			{
				// Check if it's an hex identifier or a string
				if (identifier.length() >= 3 && identifier[0] == '0' && identifier[1] == 'x')
				{
					bool isHex = true;
					for (int i = 2; i < identifier.length(); ++i)
					{
						if (!isHexDigit(identifier[i]))
						{
							isHex = false;
							break;
						}
					}
					if (isHex)
					{
						key = rmx::parseInteger(identifier);
					}
				}

				if (key == 0)
				{
					key = rmx::getMurmur2_64(identifier);
				}
			}

			std::wstring filename;
			Vec2i center;
			Recti rect;

			for (auto it = iterator->begin(); it != iterator->end(); ++it)
			{
				Json::String keyString = it.key().asString();
				if (keyString == "File")
				{
					JsonHelper::parseWString(filename, it);
				}
				else if (keyString == "Center")
				{
					JsonHelper::parseVec2i(center, it);
				}
				else if (keyString == "Rect")
				{
					JsonHelper::parseRecti(rect, it);
				}
			}

			if (!filename.empty())
			{
				// Check for overloading
				{
					Item* existingItem = mapFind(mSpriteItems, key);
					if (nullptr != existingItem)
					{
						// This sprite got overloaded e.g. by a mod -- remove the old version
						SAFE_DELETE(existingItem->mSprite);
					}
				}

				Item& item = createItem(key);
				item.mSourceInfo.mType = SourceInfo::Type::SPRITE_FILE;
				item.mSourceInfo.mSourceIdentifier = *identifier;
				item.mSourceInfo.mMod = mod;

				const std::wstring fullpath = fileEntry.mPath + filename;

				// Palette or RGBA?
				item.mUsesComponentSprite = WString(filename).endsWith(L".png");

				// If this is part of a sprite sheet, we set the sheet key
				uint64 sheetKey = 0;
				if (rect.width != 0)
					sheetKey = rmx::getMurmur2_64(fullpath);

				bool success = false;
				if (!item.mUsesComponentSprite)
				{
					// Load palette sprite (= 8-bit palette sprite)
					PaletteSprite* sprite = new PaletteSprite();
					item.mSprite = sprite;

					const uint64 paletteKey = rmx::getMurmur2_64("@" + iterator.key().asString());

					if (sheetKey != 0)
					{
						// Part of a sprite sheet
						PaletteSpriteSheet& sheet = sheetCache.mPaletteSpriteSheets[sheetKey];
						if (sheet.mBitmap.empty())
						{
							success = FileHelper::loadPaletteBitmap(sheet.mBitmap, fullpath, &palette);
							if (success)
							{
								sheet.mFirstSpritePaletteKey = paletteKey;
								addSpritePalette(paletteKey, palette);
							}
						}
						else
						{
							success = true;
							mSpritePalettes.mRedirections[paletteKey] = sheet.mFirstSpritePaletteKey;
						}

						if (success)
						{
							sprite->createFromBitmap(sheet.mBitmap, rect, -center);
						}
					}
					else
					{
						// The sprite is the whole bitmap
						PaletteBitmap bitmap;
						success = FileHelper::loadPaletteBitmap(bitmap, fullpath, &palette);
						if (success)
						{
							sprite->createFromBitmap(std::move(bitmap), -center);
							addSpritePalette(paletteKey, palette);
						}
					}
				}
				else
				{
					// Load component sprite (= 32-bit RGBA sprite)
					ComponentSprite* sprite = new ComponentSprite();
					item.mSprite = sprite;

					if (sheetKey != 0)
					{
						// Part of a sprite sheet
						Bitmap& bitmap = sheetCache.mComponentSpriteSheets[sheetKey];
						if (bitmap.empty())
						{
							success = FileHelper::loadBitmap(bitmap, fullpath);
						}
						else
						{
							success = true;
						}

						if (success)
						{
							sprite->accessBitmap().copy(bitmap, rect);
						}
					}
					else
					{
						// The sprite is the whole bitmap
						success = FileHelper::loadBitmap(static_cast<ComponentSprite*>(item.mSprite)->accessBitmap(), fullpath);
					}
					item.mSprite->mOffset = -center;
				}
			}
		}
	}
}

void SpriteCollection::addSpritePalette(uint64 paletteKey, std::vector<uint32>& palette)
{
	// After loading from a BMP, the palette is set to all opaque colors, but we usually need index 0 to be transparent
	palette[0] &= 0x00ffffff;
	mSpritePalettes.mPalettes[paletteKey] = palette;
}
