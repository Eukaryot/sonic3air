/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace
{
	bool compareFilenames(const FileCrawler::FileEntry& entry1, const FileCrawler::FileEntry& entry2)
	{
		return (entry1.mFilename.compare(entry2.mFilename) < 0);
	}

	bool compareTimes(const FileCrawler::FileEntry& entry1, const FileCrawler::FileEntry& entry2)
	{
		return (entry1.mTime < entry2.mTime);
	}

	bool compareSizes(const FileCrawler::FileEntry& entry1, const FileCrawler::FileEntry& entry2)
	{
		return (entry1.mSize < entry2.mSize);
	}
}



std::vector<std::wstring> FileCrawler::getSubdirectories(const std::wstring& parentDirectory)
{
	std::vector<std::wstring> output;
	FTX::FileSystem->listDirectories(parentDirectory, output);
	return output;
}

FileCrawler::FileCrawler()
{
}

FileCrawler::~FileCrawler()
{
	clear();
}

void FileCrawler::clear()
{
	mFileEntries.clear();
}

void FileCrawler::addFiles(const WString& filemask, bool recursive)
{
	WString input(filemask);
	input.replace('\\', '/');

	if (input.endsWith(L"/"))
		input << L"*";
	addFilesInternal(input, recursive);
}

void FileCrawler::sort(SortMode mode)
{
	bool(*compareFunc)(const FileEntry&, const FileEntry&) = nullptr;
	switch (mode)
	{
		case SortMode::BY_FILENAME:	 compareFunc = &compareFilenames;  break;
		case SortMode::BY_EXTENSION: compareFunc = &compareFilenames;  break;	// TODO!
		case SortMode::BY_TIME:		 compareFunc = &compareTimes;	   break;
		case SortMode::BY_SIZE:		 compareFunc = &compareSizes;	   break;
	}

	if (nullptr != compareFunc)
	{
		std::sort(mFileEntries.begin(), mFileEntries.end(), compareFunc);
	}
}

void FileCrawler::invertOrder()
{
	for (size_t i = 0; i < mFileEntries.size() / 2; ++i)
	{
		const size_t k = mFileEntries.size() - 1 - i;
		std::swap(mFileEntries[i], mFileEntries[k]);
	}
}

const FileCrawler::FileEntry* FileCrawler::getFileEntry(size_t index) const
{
	if (index >= mFileEntries.size())
		return nullptr;
	return &mFileEntries[index];
}

bool FileCrawler::loadFile(size_t index, std::vector<uint8>& buffer)
{
	if (index >= mFileEntries.size())
		return false;

	return FTX::FileSystem->readFile(mFileEntries[index].mPath + mFileEntries[index].mFilename, buffer);
}

void FileCrawler::addFilesInternal(const WString& input, bool recursive)
{
	FTX::FileSystem->listFilesByMask(*input, recursive, mFileEntries);
}
