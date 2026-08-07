/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/resources/PaletteCollection.h"
#include "oxygen/resources/RawDataCollection.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/modding/ModManager.h"
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
			romPath = config.mGameAppDataPath + romInfo.mSteamRomName;
			loaded = loadRomFile(romPath, romInfo);
			if (loaded)
				break;
		}

		// If ROM is not found yet, but in one of the next steps, then make sure to copy it into the app data folder afterwards
		saveRom = !loaded;
	}

#if !defined(PLATFORM_ANDROID)
	// Try at last known ROM location, if there is one
	//  -> Do this only for the S3AIR executable, it won't work when switching between projects in OxygenApp
	if (!loaded && !config.mLastRomPath.empty() && gameProfile.mIdentifier == "Sonic3AIR")
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
	PaletteCollection::instance().loadPalettes();
	RawDataCollection::instance().loadRawData();
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
	if (GameProfile::instance().mRomInfos.empty())
	{
		mRom = content;
		if (checkRomContent())
			return true;
	}
	else
	{
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
		const std::wstring filepath = Configuration::instance().mGameAppDataPath + mLoadedRomInfo->mSteamRomName;
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

