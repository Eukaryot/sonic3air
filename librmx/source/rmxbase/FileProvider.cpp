/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


namespace rmx
{

	FileProvider::~FileProvider()
	{
		for (FileSystem* fileSystem : mRegisteredMountPointFileSystems)
		{
			fileSystem->onFileProviderDestroyed(*this);
		}
	}


	bool RealFileProvider::exists(const std::wstring& filename)
	{
		return FileIO::exists(filename);
	}

	bool RealFileProvider::readFile(const std::wstring& filename, std::vector<uint8>& outData)
	{
		return FileIO::readFile(filename, outData);
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
