/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/file/FilePackage.h"

class PackedFileInputStream;
class StreamingPackedFileInputStream;


class PackedFileProvider : public rmx::FileProvider
{
friend struct PackedFileProvDetail;

public:
	using PackedFile = FilePackage::PackedFile;

	enum class CacheType
	{
		NO_CACHING,			// Always load files from disk, no caching
		CACHE_WHEN_LOADED,	// When a file gets loaded, cache its content
		CACHE_EVERYTHING	// Load and cache the whole package
	};

public:
	static PackedFileProvider* createPackedFileProvider(std::wstring_view packageFilename);

public:
	PackedFileProvider(std::wstring_view packageFilename, CacheType cacheType);
	~PackedFileProvider();

	const bool isLoaded() const  { return mLoaded; }
	void unregisterPackedFileInputStream(PackedFileInputStream& inputStream);
	void unregisterStreamingPackedFileInputStream(StreamingPackedFileInputStream& inputStream);

	bool exists(const std::wstring& path) override;
	bool readFile(const std::wstring& filename, std::vector<uint8>& outData) override;
	bool listFiles(const std::wstring& path, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries) override;
	bool listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries) override;
	bool listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories) override;
	InputStream* createInputStream(const std::wstring& filename) override;

private:
	PackedFile* findPackedFile(const std::wstring& filename);
	void loadPackedFile(PackedFile& packedFile);
	bool loadPackedFile(PackedFile& packedFile, std::vector<uint8>& outData);
	InputStream* createPackedFileInputStream(PackedFile& packedFile);
	void invalidateAllPackedFileInputStreams();

private:
	struct Internal;
	Internal& mInternal;

	CacheType mCacheType = CacheType::NO_CACHING;
	std::wstring mPackageFilename;
	std::map<std::wstring, PackedFile> mPackedFiles;
	bool mLoaded = false;

	// Managed input streams created in "createInputStream" calls
	std::set<PackedFileInputStream*> mPackedFileInputStreams;
	std::set<StreamingPackedFileInputStream*> mStreamingPackedFileInputStreams;
};
