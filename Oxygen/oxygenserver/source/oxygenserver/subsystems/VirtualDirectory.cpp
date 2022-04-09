/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/subsystems/VirtualDirectory.h"


void VirtualDirectory::startup()
{
	// Just as a test: Add a single file
	addFileContent(0x1234, 0x1000, L"s3air_test_content.bin");
	Directory& s3airDir = addSubDirectory(mRootDirectory, L"sonic3air");
	addFile(s3airDir, L"test.bin", 0x1234);
}

VirtualDirectory::FileContent& VirtualDirectory::addFileContent(uint64 key, uint64 size, const std::wstring& realPath)
{
	RMX_CHECK(mFileContents.count(key) == 0, "Duplicate file content insertion", return mFileContents[key]);

	FileContent& content = mFileContents[key];
	content.mSize = size;
	content.mRealPath = realPath;
	content.mCachedContent.clear();
	content.mChunks.clear();
	return content;
}

bool VirtualDirectory::loadCachedFileContent(FileContent& content)
{
	if (!content.mCachedContent.empty())
		return true;

	if (!FTX::FileSystem->readFile(content.mRealPath, content.mCachedContent))
		return false;

	return true;
}

bool VirtualDirectory::setupFileContentChunks(FileContent& content)
{
	if (!content.mChunks.empty())
		return true;

	if (content.mCachedContent.empty())
	{
		if (!loadCachedFileContent(content))
			return false;
		if (content.mCachedContent.empty())
			return true;
	}

	const constexpr size_t MAX_CHUNK_SIZE = 0x100000;	// 1 MB
	const size_t numChunks = (content.mCachedContent.size() + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;
	content.mChunks.resize(numChunks);
	for (size_t k = 0; k < numChunks; ++k)
	{
		FileContent::Chunk& chunk = content.mChunks[k];
		chunk.mStartOffset = (uint64)(k * MAX_CHUNK_SIZE);
		chunk.mSize = std::min<uint64>((uint64)MAX_CHUNK_SIZE, (uint64)content.mCachedContent.size() - chunk.mStartOffset);
		chunk.mHash = rmx::getMurmur2_64(&content.mCachedContent[(size_t)chunk.mStartOffset], (size_t)chunk.mSize);
	}
	return true;
}

VirtualDirectory::FileEntry& VirtualDirectory::addFile(Directory& parentDirectory, const std::wstring& name, uint64 contentKey)
{
	// Check if file entry already exists
	for (FileEntry& existingFile : parentDirectory.mFiles)
	{
		if (existingFile.mName == name)
		{
			existingFile.mKey = contentKey;
			return existingFile;
		}
	}

	// Add new file entry
	FileEntry& file = vectorAdd(parentDirectory.mFiles);
	file.mName = name;
	file.mKey = contentKey;
	return file;
}

VirtualDirectory::Directory& VirtualDirectory::addSubDirectory(Directory& parentDirectory, const std::wstring& name)
{
	// Check if directory already exists
	for (Directory& existingDir : parentDirectory.mSubDirectories)
	{
		if (existingDir.mName == name)
		{
			return existingDir;
		}
	}

	// Add new directory
	Directory& directory = vectorAdd(parentDirectory.mSubDirectories);
	directory.mName = name;
	return directory;
}
