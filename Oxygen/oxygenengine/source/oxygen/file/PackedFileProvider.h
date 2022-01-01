/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/file/FilePackage.h"

class PackedFileInputStream;


class PackedFileProvider : public rmx::FileProvider
{
friend struct PackedFileProvDetail;

public:
	using PackedFile = FilePackage::PackedFile;

public:
	PackedFileProvider(const std::wstring& packageFilename);
	~PackedFileProvider();

	const bool isLoaded() const  { return mLoaded; }
	void unregisterPackedFileInputStream(PackedFileInputStream& inputStream);

	bool exists(const std::wstring& filename) override;
	bool readFile(const std::wstring& filename, std::vector<uint8>& outData) override;
	bool listFiles(const std::wstring& path, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries) override;
	bool listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries) override;
	bool listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories) override;
	InputStream* createInputStream(const std::wstring& filename) override;

private:
	PackedFile* findPackedFile(const std::wstring& filename);
	void loadPackedFile(PackedFile& packedFile);
	void invalidateAllPackedFileInputStreams();

private:
	struct Internal;
	Internal& mInternal;

	std::map<std::wstring, PackedFile> mPackedFiles;
	bool mLoaded = false;
	InputStream* mInputStream = nullptr;
	std::set<PackedFileInputStream*> mPackedFileInputStreams;	// Managed input streams created in "createInputStream" calls
};
