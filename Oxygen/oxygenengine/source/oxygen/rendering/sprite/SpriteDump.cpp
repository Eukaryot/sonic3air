/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/sprite/SpriteDump.h"
#include "oxygen/rendering/parts/PaletteManager.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/helper/JsonHelper.h"


void SpriteDump::load()
{
	// Open file
	const Json::Value root = JsonHelper::loadFile(Configuration::instance().mAnalysisDir + L"spritedump/index.json");
	if (root.isNull())
		return;

	for (auto it = root.begin(); it != root.end(); ++it)
	{
		const std::string categoryName = it.key().asString();
		Category& category = getOrCreateCategory(categoryName);
		category.mName = categoryName;
		for (auto it2 = it->begin(); it2 != it->end(); ++it2)
		{
			JsonHelper rootHelper(*it2);
			const uint8 spriteNumber = (uint8)rmx::parseInteger("0x" + it2.key().asString());

			Entry& entry = category.mEntries[spriteNumber];
			entry.mSpriteNumber = spriteNumber;
			rootHelper.tryReadInt("SizeX", entry.mSize.x);
			rootHelper.tryReadInt("SizeY", entry.mSize.y);
			rootHelper.tryReadInt("OffsetX", entry.mOffset.x);
			rootHelper.tryReadInt("OffsetY", entry.mOffset.y);
		}
	}

	mAnyChange = false;
}

void SpriteDump::save()
{
	if (!mAnyChange)
		return;

	Json::Value root;
	for (const auto& pair : mCategories)
	{
		const Category& category = pair.second;
		Json::Value categoryJson;
		for (const auto& pair2 : category.mEntries)
		{
			const Entry& entry = pair2.second;
			const std::string key = rmx::hexString(pair2.first, 2, "");

			Json::Value entryJson;
			entryJson["SizeX"] = entry.mSize.x;
			entryJson["SizeY"] = entry.mSize.y;
			entryJson["OffsetX"] = entry.mOffset.x;
			entryJson["OffsetY"] = entry.mOffset.y;
			categoryJson[key] = entryJson;
		}
		root[category.mName] = categoryJson;

		if (category.mChanged)
		{
			saveSpriteAtlas(category.mName);
		}
	}

	// Save file
	JsonHelper::saveFile(Configuration::instance().mAnalysisDir + L"/spritedump/index.json", root);
}

void SpriteDump::addSprite(const PaletteSprite& paletteSprite, std::string_view categoryName, uint8 spriteNumber, uint8 atex)
{
	String filename(0, *(WString(Configuration::instance().mAnalysisDir).toString() + "/spritedump/%s/%02x.bmp"), categoryName.data(), spriteNumber);
	if (!FTX::FileSystem->exists(*filename))
	{
		Color palette[0x100];
		VideoOut::instance().getRenderParts().getPaletteManager().getPalette(0).dumpColors(palette, 0x100);
		if (atex != 0)
		{
			for (int i = 0; i < 0x10; ++i)
				palette[i] = palette[atex + i];
		}

		PaletteSprite copy = paletteSprite;
		copy.accessBitmap().overwriteUnusedPaletteEntries(palette);

		std::vector<uint8> content;
		copy.accessBitmap().saveBMP(content, palette);
		FTX::FileSystem->saveFile(filename.toStdWString(), content);
	}

	Category& category = getOrCreateCategory(categoryName);
	if (category.mEntries.count(spriteNumber) == 0)
	{
		Entry& entry = category.mEntries[spriteNumber];
		entry.mSpriteNumber = spriteNumber;
		entry.mSize = paletteSprite.getBitmap().getSize();
		entry.mOffset = paletteSprite.mOffset;

		category.mChanged = true;
		mAnyChange = true;
	}
}

void SpriteDump::addSpriteWithTranslation(const PaletteSprite& paletteSprite, std::string_view categoryName, uint8 spriteNumber, uint8 atex)
{
	// Translate category key
	std::string translatedName = std::string(categoryName);
		 if (categoryName == "100000_148182_146620")  translatedName = "character_sonic";
	else if (categoryName == "140060_148182_146620")  translatedName = "character_sonic";
	else if (categoryName == "100000_148378_146816")  translatedName = "character_supersonic";
	else if (categoryName == "140060_148378_146816")  translatedName = "character_supersonic";
	else if (categoryName == "345010_347f8a_347e30")  translatedName = "character_sonic_snowboarding";
	else if (categoryName == "143d00_14a08a_148eb8")  translatedName = "character_tails";
	else if (categoryName == "3200e0_14a08a_148eb8")  translatedName = "character_tails";
	else if (categoryName == "336620_344d74_344bb8")  translatedName = "character_tails_tails";
	else if (categoryName == "1200e0_14bd0a_14a8d6")  translatedName = "character_knuckles";
	else if (categoryName == "0aaa7c_0abe14_0abdfc")  translatedName = "bluesphere_sonic";
	else if (categoryName == "28f95a_2908d2_2908ba")  translatedName = "bluesphere_tails";
	else if (categoryName == "2909e8_291106_2910e8")  translatedName = "bluesphere_tails_tails";
	else if (categoryName == "0abf22_0ad31a_0ad302")  translatedName = "bluesphere_knuckles";
	else if (categoryName == "382dc6_36430e_364016")  translatedName = "cutscene_knuckles_1";
	else if (categoryName == "172406_067078_066f36")  translatedName = "cutscene_knuckles_2";
	else if (categoryName == "17e274_066bda_066b6a")  translatedName = "cutscene_knuckles_3";
	else if (categoryName == "169812_066b10_066ad0")  translatedName = "cutscene_knuckles_4";
	else if (categoryName == "347850_348128_348020")  translatedName = "snowboard";
	else if (categoryName == "36732a_36156e_3615a8")  translatedName = "enemy_aiz_rhinobot";
	else if (categoryName == "0d8766_061abe_0619e0")  translatedName = "object_giantring";
	else if (categoryName == "0dcc76_083b6c_083b9e")  translatedName = "object_signpost";
	else return;	// This may get removed temporarily to output everything

	addSprite(paletteSprite, translatedName, spriteNumber, atex);
}

SpriteDump::Category& SpriteDump::getOrCreateCategory(std::string_view categoryName)
{
	const uint64 keyHash = rmx::getMurmur2_64(categoryName);
	const auto it = mCategories.find(keyHash);
	if (it == mCategories.end())
	{
		Category& category = mCategories[keyHash];
		category.mName = categoryName;
		return category;
	}
	else
	{
		return it->second;
	}
}

void SpriteDump::saveSpriteAtlas(std::string_view categoryName)
{
	const auto it = mCategories.find(rmx::getMurmur2_64(categoryName));
	if (it == mCategories.end())
		return;

	const Category& category = it->second;
	const String path(0, *(WString(Configuration::instance().mAnalysisDir).toString() + "/spritedump/%s"), categoryName.data());

	std::vector<std::pair<PaletteBitmap, const Entry*>> bitmaps;
	bitmaps.reserve(category.mEntries.size());
	Color palette[0x100];

	std::vector<uint8> buffer;
	Vec2i imgSize;
	for (const auto& pair : category.mEntries)
	{
		const Entry& entry = pair.second;
		String filename(0, "%s/%02x.bmp", *path, entry.mSpriteNumber);
		buffer.clear();
		if (FTX::FileSystem->readFile(*filename, buffer))
		{
			bitmaps.emplace_back(PaletteBitmap(), &entry);
			PaletteBitmap& bitmap = bitmaps.back().first;
			if ((bitmaps.size() == 1) ? bitmap.loadBMP(buffer, palette) : bitmap.loadBMP(buffer))
			{
				imgSize.x = std::max<int>(imgSize.x, bitmap.mWidth);
				imgSize.y = std::max<int>(imgSize.y, bitmap.mHeight);
			}
			else
			{
				bitmaps.pop_back();
			}
		}
	}

	PaletteBitmap output;
	output.create(imgSize.x * 16, imgSize.y * ((int)(bitmaps.size() + 15) / 16));
	output.clear(0xff);
	palette[0xff] = Color(0.15f, 0.15f, 0.15f);		// Use a dark gray background

	String json = "{";
	for (size_t i = 0; i < bitmaps.size(); ++i)
	{
		const PaletteBitmap& bitmap = bitmaps[i].first;
		const Entry& entry = *bitmaps[i].second;

		const Vec2i destPosition(imgSize.x * (int)(i % 16) + (imgSize.x - bitmap.mWidth) / 2,
								 imgSize.y * (int)(i / 16) + (imgSize.y - bitmap.mHeight) / 2);
		output.copyRect(bitmap, Recti(0, 0, bitmap.mWidth, bitmap.mHeight), destPosition);

		// Example:   "title_screen_air": { "File": "title_screen.png", "Rect": "0,0,152,18", "Center": "76,9" },

		if (i > 0)
		{
			json << ",";
		}
		json << "\r\n\t\"" << categoryName << "_" << rmx::hexString(entry.mSpriteNumber, 2) << "\": ";
		json << "{ \"File\": \"" << categoryName << ".bmp\", ";
		json << "\"Rect\": \"" << destPosition.x << "," << destPosition.y << "," << bitmap.mWidth << "," << bitmap.mHeight << "\", ";
		json << "\"Center\": \"" << (-entry.mOffset.x) << "," << (-entry.mOffset.y) << "\" }";
	}
	json << "\r\n}\r\n";

	buffer.clear();
	if (output.saveBMP(buffer, palette))
	{
		FTX::FileSystem->saveFile(*(path + "/" + categoryName + ".bmp"), buffer);
		FTX::FileSystem->saveFile(*(path + "/" + categoryName + ".json"), (uint8*)json.accessData(), json.length());
	}
}
