/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/file/FilePackage.h"


class ZipFileProvider : public rmx::FileProvider
{
friend struct ZipFileProvDetail;

public:
	ZipFileProvider(const std::wstring& zipFilename);
	~ZipFileProvider();

	const bool isLoaded() const  { return mLoaded; }

	bool exists(const std::wstring& path) override;
	bool readFile(const std::wstring& filename, std::vector<uint8>& outData) override;
	bool listFiles(const std::wstring& path, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries) override;
	bool listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries) override;
	bool listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories) override;
	InputStream* createInputStream(const std::wstring& filename) override;

private:
	struct ContainedFile
	{
		rmx::FileIO::FileEntry mFileEntry;
		std::vector<uint8> mContent;		// Cached content
	};

private:
	bool scanZipFile(const std::wstring& zipFilename);
	const ContainedFile* readFile(const std::wstring& filename);

	ContainedFile* findContainedFile(const std::wstring& filePath);
	const ContainedFile* findContainedFile(const std::wstring& filePath) const;

private:
	struct Internal;
	Internal& mInternal;

	std::map<uint64, ContainedFile> mContainedFiles;
	bool mLoaded = false;
};
