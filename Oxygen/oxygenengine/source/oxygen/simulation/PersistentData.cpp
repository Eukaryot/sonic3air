/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/PersistentData.h"
#include "oxygen/application/Configuration.h"


namespace
{
	const char* FORMAT_IDENTIFIER = "OXY.PDATA";
	const uint16 FORMAT_VERSION = 0x0100;		// First version
}


void PersistentData::clear()
{
	mEntries.clear();
}

bool PersistentData::loadFromFile(const std::wstring& filename)
{
	mFilename = filename;
	clear();

	std::vector<uint8> content;
	if (!FTX::FileSystem->readFile(filename, content))
		return false;

	VectorBinarySerializer serializer(true, content);
	return serialize(serializer);
}

bool PersistentData::saveToFile()
{
	if (mFilename.empty())
		return false;

	std::vector<uint8> content;
	VectorBinarySerializer serializer(false, content);
	if (!serialize(serializer))
		return false;

	return FTX::FileSystem->saveFile(mFilename, content);
}

const std::vector<uint8>& PersistentData::getData(uint64 keyHash) const
{
	static const std::vector<uint8> EMPTY;
	const auto it = mEntries.find(keyHash);
	return (it == mEntries.end()) ? EMPTY : it->second.mData;
}

void PersistentData::setData(std::string_view key, const std::vector<uint8>& data)
{
	const uint64 keyHash = rmx::getMurmur2_64(key);
	const auto it = mEntries.find(keyHash);
	if (it == mEntries.end())
	{
		Entry& entry = mEntries[keyHash];
		entry.mKey = key;
		entry.mData = data;
		saveToFile();
	}
	else
	{
		// Check for changes
		bool anyChange = (it->second.mData.size() != data.size());
		if (!anyChange)
		{
			anyChange = (memcmp(&it->second.mData[0], &data[0], data.size()) != 0);
		}

		if (anyChange)
		{
			// Intentionally overwriting the whole data, not just parts of it
			it->second.mData = data;
			saveToFile();
		}
	}
}

bool PersistentData::serialize(VectorBinarySerializer& serializer)
{
	// Identifier
	if (serializer.isReading())
	{
		char identifier[10];
		serializer.read(identifier, 9);
		if (memcmp(identifier, FORMAT_IDENTIFIER, 9) != 0)
			return false;
	}
	else
	{
		serializer.write(FORMAT_IDENTIFIER, 9);
	}

	// Format version
	uint16 formatVersion = FORMAT_VERSION;
	serializer& formatVersion;
	if (serializer.isReading())
	{
		RMX_CHECK(formatVersion >= 0x0100, "Invalid persistent data file format version", return false);
		RMX_CHECK(formatVersion <= FORMAT_VERSION, "Can't read persistent data file, as it's using a newer format version", return false);
	}

	// Data entries
	if (serializer.isReading())
	{
		const size_t count = (size_t)serializer.read<uint32>();
		std::string key;
		for (size_t i = 0; i < count; ++i)
		{
			serializer.serialize(key);
			const uint64 keyHash = rmx::getMurmur2_64(key);
			Entry& entry = mEntries[keyHash];
			entry.mKey = key;
			const size_t size = (size_t)serializer.read<uint32>();
			entry.mData.resize(size);
			serializer.read(&entry.mData[0], size);
		}
	}
	else
	{
		serializer.writeAs<uint32>(mEntries.size());
		for (const auto& pair : mEntries)
		{
			serializer.write(pair.second.mKey);
			serializer.writeAs<uint32>(pair.second.mData.size());
			serializer.write(&pair.second.mData[0], pair.second.mData.size());
		}
	}

	return true;
}
