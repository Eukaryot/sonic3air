/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/platform/PlatformFunctions.h"


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
		for (const GameProfile::RomInfo& romInfo : gameProfile.mRomInfos)
		{
			romPath = config.mAppDataPath + romInfo.mSteamRomName;
			loaded = loadRomFile(romPath, romInfo);
			if (loaded)
				break;
		}

		// If ROM is not found yet, but in one of the next steps, then make sure to copy it into the app data folder afterwards
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
		for (const GameProfile::RomInfo& romInfo : gameProfile.mRomInfos)
		{
			romPath = romInfo.mSteamRomName;
			loaded = loadRomFile(romPath, romInfo);
			if (loaded)
				break;
		}
	}

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
	// If still not loaded, search for Steam installation of the game
	if (!loaded && !gameProfile.mRomInfos.empty())
	{
		RMX_LOG_INFO("Trying to find Steam ROM");
		for (const GameProfile::RomInfo& romInfo : gameProfile.mRomInfos)
		{
			romPath = PlatformFunctions::tryGetSteamRomPath(romInfo.mSteamRomName);
			if (!romPath.empty())
			{
				loaded = loadRomFile(romPath, romInfo);
				if (loaded)
					break;
			}
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
	if (!loadRomMemory(content))
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
	loadPalettes(L"data/palettes", false);
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
	return mapFind(mPalettes, key + line);
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
	std::vector<uint8> content;
	content.reserve(romCheck.mSize > 0 ? romCheck.mSize : 0x400000);
	if (!FTX::FileSystem->readFile(filename, content))
		return false;

	return loadRomMemory(content);
}

bool ResourcesCache::loadRomFile(const std::wstring& filename, const GameProfile::RomInfo& romInfo)
{
	const GameProfile::RomCheck& romCheck = GameProfile::instance().mRomCheck;
	mRom.reserve(romCheck.mSize > 0 ? romCheck.mSize : 0x400000);
	if (!FTX::FileSystem->readFile(filename, mRom))
		return false;

	// If ROM info defines a required header checksum, make sure it fits (this is meant to be an early-out before doing the potentially expensive code below)
	const uint64 headerChecksum = getHeaderChecksum(mRom);
	if (romInfo.mHeaderChecksum != 0 && romInfo.mHeaderChecksum != headerChecksum)
		return false;

	if (applyRomModifications(romInfo))
	{
		if (checkRomContent())
		{
			mLoadedRomInfo = &romInfo;
			return true;
		}
	}
	return false;
}

bool ResourcesCache::loadRomMemory(const std::vector<uint8>& content)
{
	const uint64 headerChecksum = getHeaderChecksum(content);
	for (const GameProfile::RomInfo& romInfo : GameProfile::instance().mRomInfos)
	{
		// If ROM info defines a required header checksum, make sure it fits (this is meant to be an early-out before doing the potentially expensive code below)
		if (romInfo.mHeaderChecksum != 0 && romInfo.mHeaderChecksum != headerChecksum)
			continue;

		mRom = content;
		if (applyRomModifications(romInfo))
		{
			if (checkRomContent())
			{
				mLoadedRomInfo = &romInfo;
				return true;
			}
		}
	}
	return false;
}

uint64 ResourcesCache::getHeaderChecksum(const std::vector<uint8>& content)
{
	if (content.empty())
		return 0;
	
	// Regard the first 512 byte as header
	return rmx::getMurmur2_64(&content[0], std::min<size_t>(512, content.size()));
}

bool ResourcesCache::applyRomModifications(const GameProfile::RomInfo& romInfo)
{
	if (!romInfo.mDiffFileName.empty())
	{
		// Load diff file if needed
		std::vector<uint8>* content = nullptr;
		const auto it = mDiffFileCache.find(&romInfo);
		if (it == mDiffFileCache.end())
		{
			content = &mDiffFileCache[&romInfo];
			FTX::FileSystem->readFile(romInfo.mDiffFileName, *content);
		}
		else
		{
			content = &it->second;
		}

		// Apply diff file by XORing it in
		if (content->size() != mRom.size())
			return false;

		uint64* ptr = (uint64*)&mRom[0];
		uint64* diff = (uint64*)&(*content)[0];
		const size_t count = content->size() / 8;
		for (size_t i = 0; i < count; ++i)
		{
			ptr[i] ^= diff[i];
		}
	}

	for (auto& pair : romInfo.mBlankRegions)
	{
		if (pair.first > pair.second || pair.second >= mRom.size())
			return false;
		memset(&mRom[pair.first], 0, pair.second - pair.first + 1);
	}

	for (auto& pair : romInfo.mOverwrites)
	{
		if (pair.first >= mRom.size())
			return false;
		mRom[pair.first] = pair.second;
	}

	return true;
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

	if (romCheck.mChecksum != 0)
	{
		const uint64 checksum = rmx::getMurmur2_64(&mRom[0], mRom.size());
		if (checksum != romCheck.mChecksum)
			return false;
	}

	// ROM check succeeded 
	mDiffFileCache.clear();		// This cache is not needed again now
	return true;
}

void ResourcesCache::saveRomToAppData()
{
	if (nullptr != mLoadedRomInfo && !mLoadedRomInfo->mSteamRomName.empty())
	{
		const std::wstring filepath = Configuration::instance().mAppDataPath + mLoadedRomInfo->mSteamRomName;
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
	std::vector<rmx::FileIO::FileEntry> fileEntries;
	fileEntries.reserve(8);
	FTX::FileSystem->listFilesByMask(path + L"/*.json", true, fileEntries);
	for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
	{
		const Json::Value root = JsonHelper::loadFile(fileEntry.mPath + fileEntry.mFilename);

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
				if (!FTX::FileSystem->readFile(fileEntry.mPath + String(filename).toStdWString(), rawData->mContent))
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
	std::vector<rmx::FileIO::FileEntry> fileEntries;
	fileEntries.reserve(8);
	FTX::FileSystem->listFilesByMask(path + L"/*.png", true, fileEntries);
	for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
	{
		if (!FTX::FileSystem->exists(fileEntry.mPath + fileEntry.mFilename))
			continue;

		std::vector<uint8> content;
		if (!FTX::FileSystem->readFile(fileEntry.mPath + fileEntry.mFilename, content))
			continue;

		Bitmap bitmap;
		if (!bitmap.load(fileEntry.mPath + fileEntry.mFilename))
		{
			RMX_ERROR("Failed to load PNG at '" << *WString(fileEntry.mPath + fileEntry.mFilename).toString() << "'", );
			continue;
		}

		String name = WString(fileEntry.mFilename).toString();
		name.remove(name.length() - 4, 4);

		uint64 key = rmx::getMurmur2_64(name);		// Hash is the key of the first palette, the others are enumerated from there
		const int numLines = std::min(bitmap.getHeight(), 64);
		const int numColorsPerLine = std::min(bitmap.getWidth(), 64);

		for (int y = 0; y < numLines; ++y)
		{
			Palette& palette = mPalettes[key];
			palette.mIsModded = isModded;
			palette.mColors.resize(numColorsPerLine);

			for (int x = 0; x < numColorsPerLine; ++x)
			{
				palette.mColors[x] = Color::fromABGR32(bitmap.getPixel(x, y));
			}
			++key;
		}
	}
}
