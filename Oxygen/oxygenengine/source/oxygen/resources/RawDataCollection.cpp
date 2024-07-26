/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/RawDataCollection.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/JsonHelper.h"


const std::vector<const RawDataCollection::RawData*>& RawDataCollection::getRawData(uint64 key) const
{
	static const std::vector<const RawData*> EMPTY;
	const auto it = mRawDataMap.find(key);
	return (it == mRawDataMap.end()) ? EMPTY : it->second;
}

void RawDataCollection::clear()
{
	mRawDataMap.clear();
	mRomInjections.clear();
	mRawDataPool.clear();
}

void RawDataCollection::loadRawData()
{
	// Load or reload raw data incl. ROM injections
	clear();
	loadRawDataInDirectory(L"data/rawdata", false);
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadRawDataInDirectory(mod->mFullPath + L"rawdata", true);
	}
}

void RawDataCollection::applyRomInjections(uint8* rom, uint32 romSize) const
{
	for (const RawData* rawData : mRomInjections)
	{
		RMX_ASSERT(rawData->mRomInjectAddress.has_value(), "No ROM injection address given for what is stored as a ROM injection");
		const uint32 address = *rawData->mRomInjectAddress;
		RMX_CHECK(address < romSize, "ROM injection at invalid address " << rmx::hexString(address, 6), continue);

		const uint32 size = std::min((uint32)rawData->mContent.size(), romSize - address);
		memcpy(&rom[address], &rawData->mContent[0], size);
	}
}

void RawDataCollection::loadRawDataInDirectory(const std::wstring& path, bool isModded)
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
				const uint64 key = rmx::getMurmur2_64(it.key().asCString());
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
