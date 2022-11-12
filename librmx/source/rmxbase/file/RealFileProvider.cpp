/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace rmx
{

	bool RealFileProvider::exists(const std::wstring& path)
	{
		return FileIO::exists(path);
	}

	bool RealFileProvider::getFileSize(const std::wstring& filename, uint64& outFileSize)
	{
		return FileIO::getFileSize(filename, outFileSize);
	}

	bool RealFileProvider::getFileTime(const std::wstring& filename, time_t& outFileTime)
	{
		return FileIO::getFileTime(filename, outFileTime);
	}

	bool RealFileProvider::readFile(const std::wstring& filename, std::vector<uint8>& outData)
	{
		return FileIO::readFile(filename, outData);
	}

	bool RealFileProvider::renameFile(const std::wstring& oldFilename, const std::wstring& newFilename)
	{
		return FileIO::renameFile(oldFilename, newFilename);
	}

	bool RealFileProvider::listFiles(const std::wstring& path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries)
	{
		FileIO::listFiles(path, recursive, outFileEntries);
		return true;
	}

	bool RealFileProvider::listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries)
	{
		FileIO::listFilesByMask(filemask, recursive, outFileEntries);
		return true;
	}

	bool RealFileProvider::listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories)
	{
		FileIO::listDirectories(path, outDirectories);
		return true;
	}

	InputStream* RealFileProvider::createInputStream(const std::wstring& filename)
	{
		return FileIO::createInputStream(filename);
	}

}
