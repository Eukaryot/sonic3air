/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "rmxbase/file/FileProvider.h"


namespace rmx
{

	class API_EXPORT RealFileProvider : public FileProvider
	{
	public:
		bool exists(const std::wstring& path) override;
		bool getFileSize(const std::wstring& filename, uint64& outFileSize) override;
		bool getFileTime(const std::wstring& filename, time_t& outFileTime) override;
		bool readFile(const std::wstring& filename, std::vector<uint8>& outData) override;

		bool renameFile(const std::wstring& oldFilename, const std::wstring& newFilename) override;
		bool listFiles(const std::wstring& path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries) override;
		bool listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries) override;
		bool listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories) override;
		InputStream* createInputStream(const std::wstring& filename) override;

	private:
		std::wstring mRealLocation;
	};

}
