/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class PersistentData : public SingleInstance<PersistentData>
{
public:
	struct Entry
	{
		std::string mKey;
		uint64 mKeyHash = 0;
		std::vector<uint8> mData;
	};

	struct File
	{
		std::string mFilePath;
		std::vector<Entry> mEntries;
	};

public:
	void clear();
	void loadFromBasePath(const std::wstring& basePath);

	void updatePersistentData();

	const std::vector<uint8>& getData(uint64 filePathHash, uint64 keyHash) const;
	void setData(std::string_view filePath, std::string_view key, const std::vector<uint8>& data);
	void setDataPartial(std::string_view filePath, std::string_view key, const std::vector<uint8>& data, size_t offset);
	void removeKey(uint64 filePathHash, uint64 keyHash);

	inline const std::unordered_map<uint64, File>& getFiles() const  { return mFiles; }

private:
	void initialSetup();

	Entry* findEntry(File& file, uint64 keyHash);
	const Entry* findEntry(const File& file, uint64 keyHash) const;
	bool removeEntry(File& file, uint64 keyHash);

	void setDataInternal(std::string_view filePath, uint64 filePathHash, std::string_view key, uint64 keyHash, const std::vector<uint8>& data);

	std::wstring getFullFilePath(const File& file) const;
	bool removeFile(File& file);
	bool saveFile(File& file);
	bool serializeFile(File& file, VectorBinarySerializer& serializer);

private:
	std::wstring mBasePath;
	std::unordered_map<uint64, File> mFiles;
	std::unordered_set<uint64> mPendingFileSaves;
	uint32 mChangeCounter = 0;
};
