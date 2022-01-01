/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	class API_EXPORT FileProviderSDL : public FileProvider
	{
	public:
		bool exists(const std::wstring& filename) override;
		bool getFileSize(const std::wstring& filename, uint64& outFileSize) override;
		bool readFile(const std::wstring& filename, std::vector<uint8>& outData) override;
		bool listFiles(const std::wstring& path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries) override;
		bool listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries) override;
		InputStream* createInputStream(const std::wstring& filename) override;
	};

}
