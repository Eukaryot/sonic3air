/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/helper/PackageFileCrawler.h"


bool ResourcesCache::loadRom()
{
	mRom.clear();

	// Load ROM content
	Configuration& config = Configuration::instance();
	const GameProfile& gameProfile = GameProfile::instance();
	std::wstring romPath;
	bool loaded = false;
	bool saveRom = false;

	// First have a look at the game's app data, where the ROM gets copied to after it was found once
	if (!loaded && !config.mAppDataPath.empty())
	{
		romPath = config.mAppDataPath + L'/' + gameProfile.mRomAutoDiscover.mSteamRomName;
		loaded = loadRomFile(romPath);

		// If ROM is found is one of the next steps, make sure to copy it into the app data folder afterwards
		saveRom = !loaded;
	}

#if !defined(PLATFORM_ANDROID)
	// Try at last known ROM location, if there is one
	if (!loaded && !config.mLastRomPath.empty())
	{
		romPath = config.mLastRomPath;
		loaded = loadRomFile(romPath);
	}

	// Then check at the configuration ROM path
	if (!loaded && !config.mRomPath.empty())
	{
		romPath = config.mRomPath;
		loaded = loadRomFile(romPath);
	}
#endif

	// Or is it the Steam ROM right inside the installation directory?
	if (!loaded)
	{
		romPath = gameProfile.mRomAutoDiscover.mSteamRomName;
		loaded = loadRomFile(romPath);
	}

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
	// If still not loaded, search for Steam installation of the game
	if (!loaded && !gameProfile.mRomAutoDiscover.mSteamRomName.empty())
	{
		RMX_LOG_INFO("Trying to find Steam ROM");
		romPath = PlatformFunctions::tryGetSteamRomPath(gameProfile.mRomAutoDiscover.mSteamRomName);
		if (!romPath.empty())
		{
			loaded = loadRomFile(romPath);
		}
	}
#endif

	// If ROM was still not loaded, it's time to give up now...
	if (!loaded)
	{
		return false;
	}

	if (saveRom)
	{
		// Note that this updates the config
		saveRomToAppData();
	}
	else
	{
		// Update config if there was a change
		if (romPath != config.mLastRomPath)
		{
			config.mLastRomPath = romPath;
			config.saveSettings();
		}
	}

	// Done
	return true;
}

bool ResourcesCache::loadRomFromFile(const std::wstring& filename)
{
	if (!loadRomFile(filename))
		return false;

	saveRomToAppData();
	return true;
}

bool ResourcesCache::loadRomFromMemory(const std::vector<uint8>& content)
{
	mRom = content;
	if (!checkRomContent())
		return false;

	saveRomToAppData();
	return true;
}

void ResourcesCache::loadAllResources()
{
	// Load raw data incl. ROM injections
	mRawDataMap.clear();
	mRomInjections.clear();
	mRawDataPool.clear();
	loadRawData(L"data/rawdata", false);
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadRawData(mod->mFullPath + L"rawdata", true);
	}

	// Load palettes
	mPalettes.clear();
	loadRawData(L"data/palettes", false);
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadPalettes(mod->mFullPath + L"palettes", true);
	}
}

const std::vector<const ResourcesCache::RawData*>& ResourcesCache::getRawData(uint64 key) const
{
	static const std::vector<const RawData*> EMPTY;
	const auto it = mRawDataMap.find(key);
	return (it == mRawDataMap.end()) ? EMPTY : it->second;
}

const ResourcesCache::Palette* ResourcesCache::getPalette(uint64 key, uint8 line) const
{
	const auto it = mPalettes.find(key + line);
	return (it == mPalettes.end()) ? nullptr : &it->second;
}

Font* ResourcesCache::getFontByKey(const std::string& keyString, uint64 keyHash)
{
	// Try to find in map
	const auto it = mCachedFonts.find(keyHash);
	if (it != mCachedFonts.end())
	{
		return &it->second.mFont;
	}
	return nullptr;
}

Font* ResourcesCache::registerFontSource(const std::string& filename)
{
	const uint64 keyHash = rmx::getMurmur2_64(filename);
	CachedFont& cachedFont = mCachedFonts[keyHash];
	cachedFont.mKeyString = filename;
	cachedFont.mKeyHash = keyHash;
	
	if (cachedFont.mFont.loadFromFile("data/font/" + filename + ".json"))
	{
		return &cachedFont.mFont;
	}
	else
	{
		mCachedFonts.erase(keyHash);
		return nullptr;
	}
}

void ResourcesCache::applyRomInjections(uint8* rom, uint32 romSize) const
{
	for (const RawData* rawData : mRomInjections)
	{
		RMX_CHECK(rawData->mRomInjectAddress < romSize, "ROM injection at invalid address " << rmx::hexString(rawData->mRomInjectAddress, 6), continue);
		const uint32 size = std::min((uint32)rawData->mContent.size(), romSize - rawData->mRomInjectAddress);
		memcpy(&rom[rawData->mRomInjectAddress], &rawData->mContent[0], size);
	}
}

bool ResourcesCache::loadRomFile(const std::wstring& filename)
{
	const GameProfile::RomCheck& romCheck = GameProfile::instance().mRomCheck;
	mRom.reserve(romCheck.mSize > 0 ? romCheck.mSize : 0x400000);
	if (!FTX::FileSystem->readFile(filename, mRom))
		return false;

	return checkRomContent();
}

bool ResourcesCache::checkRomContent()
{
	// Check that it's the right ROM
	const GameProfile::RomCheck& romCheck = GameProfile::instance().mRomCheck;
	if (romCheck.mSize > 0)
	{
		if (mRom.size() != romCheck.mSize)
			return false;
	}

	for (auto& pair : romCheck.mOverwrites)
	{
		RMX_CHECK(pair.first < mRom.size(), "Invalid overwrite address", continue);
		mRom[pair.first] = pair.second;
	}

	if (romCheck.mChecksum != 0)
	{
		const uint32 crc = rmx::getCRC32(&mRom[0], mRom.size());
		if (crc != romCheck.mChecksum)
			return false;
	}
	return true;
}

void ResourcesCache::saveRomToAppData()
{
	if (!GameProfile::instance().mRomAutoDiscover.mSteamRomName.empty())
	{
		const std::wstring filepath = Configuration::instance().mAppDataPath + L'/' + GameProfile::instance().mRomAutoDiscover.mSteamRomName;
		const bool success = FTX::FileSystem->saveFile(filepath, mRom);
		if (success)
		{
			Configuration::instance().mLastRomPath = filepath;
		}
		else
		{
			RMX_ERROR("Failed to store a copy of the ROM in the app data folder", );
		}
	}
}

void ResourcesCache::loadRawData(const std::wstring& path, bool isModded)
{
	// Load raw data from the given path
	PackageFileCrawler fc;
	fc.addFiles(path + L"/*.json", true);
	for (size_t fileIndex = 0; fileIndex < fc.size(); ++fileIndex)
	{
		const FileCrawler::FileEntry& entry = *fc[fileIndex];
		const Json::Value root = JsonHelper::loadFile(entry.mPath + entry.mFilename);

		for (auto it = root.begin(); it != root.end(); ++it)
		{
			const Json::Value& entryJson = *it;
			if (!entryJson.isObject())
				continue;

			RawData* rawData = nullptr;
			if (entryJson["File"].isString())
			{
				const char* filename = entryJson["File"].asCString();
				const uint64 key = rmx::getMurmur2_64(String(it.key().asCString()));
				rawData = &mRawDataPool.createObject();
				rawData->mIsModded = isModded;
				if (!FTX::FileSystem->readFile(entry.mPath + String(filename).toStdWString(), rawData->mContent))
				{
					mRawDataPool.destroyObject(*rawData);
					continue;
				}
				mRawDataMap[key].push_back(rawData);
			}

			if (nullptr == rawData)
				continue;

			// Check if it's a ROM injection
			if (!entryJson["RomInject"].isNull())
			{
				rawData->mRomInjectAddress = (uint32)rmx::parseInteger(entryJson["RomInject"].asCString());
				mRomInjections.emplace_back(rawData);
			}
		}
	}
}

void ResourcesCache::loadPalettes(const std::wstring& path, bool isModded)
{
	// Load palettes from the given path
	PackageFileCrawler fc;
	fc.addFiles(path + L"/*.png", true);
	for (size_t fileIndex = 0; fileIndex < fc.size(); ++fileIndex)
	{
		const FileCrawler::FileEntry& entry = *fc[fileIndex];
		if (!FTX::FileSystem->exists(entry.mPath + entry.mFilename))
			continue;

		std::vector<uint8> content;
		if (!FTX::FileSystem->readFile(entry.mPath + entry.mFilename, content))
			continue;

		Bitmap bitmap;
		if (!bitmap.load(entry.mPath + entry.mFilename))
		{
			RMX_ERROR("Failed to load PNG at '" << *WString(entry.mPath + entry.mFilename).toString() << "'", );
			continue;
		}

		String name = WString(entry.mFilename).toString();
		name.remove(name.length() - 4, 4);

		uint64 key = rmx::getMurmur2_64(name);		// Hash is the key of the first palette, the others are enumerated from there
		const int numLines = std::min(bitmap.mHeight, 64);
		const int numColorsPerLine = std::min(bitmap.mWidth, 64);

		for (int y = 0; y < numLines; ++y)
		{
			Palette& palette = mPalettes[key];
			palette.mIsModded = isModded;
			palette.mColors.resize(numColorsPerLine);

			for (int x = 0; x < numColorsPerLine; ++x)
			{
				palette.mColors[x] = Color::fromABGR32(bitmap[x + y * bitmap.mWidth]);
			}
			++key;
		}
	}
}
