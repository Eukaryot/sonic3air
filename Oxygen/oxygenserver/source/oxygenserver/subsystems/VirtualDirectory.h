/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class VirtualDirectory
{
public:
	void startup();

private:
	struct FileContent
	{
		struct Chunk
		{
			uint64 mStartOffset = 0;
			uint64 mSize = 0;
			uint64 mHash = 0;
		};

		uint64 mHash = 0;
		uint64 mSize = 0;
		std::wstring mRealPath;
		std::vector<uint8> mCachedContent;
		std::vector<Chunk> mChunks;
	};

	struct FileEntry
	{
		std::wstring mName;
		uint64 mKey = 0;
	};

	struct Directory
	{
		std::wstring mName;
		std::vector<Directory> mSubDirectories;
		std::vector<FileEntry> mFiles;
	};

private:
	FileContent& addFileContent(uint64 key, uint64 size, const std::wstring& realPath);
	bool loadCachedFileContent(FileContent& content);
	bool setupFileContentChunks(FileContent& content);

	FileEntry& addFile(Directory& parentDirectory, const std::wstring& name, uint64 contentKey);
	Directory& addSubDirectory(Directory& parentDirectory, const std::wstring& name);

private:
	std::unordered_map<uint64, FileContent> mFileContents;	// Key is the content hash
	Directory mRootDirectory;
};
