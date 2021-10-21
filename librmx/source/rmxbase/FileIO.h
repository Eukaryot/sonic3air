/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class InputStream;

namespace rmx
{

	class API_EXPORT FileIO
	{
	public:
		struct FileEntry
		{
			std::wstring mFilename;
			std::wstring mPath;
			time_t mTime = 0;
			size_t mSize = 0;
		};

	public:
		static bool exists(const std::wstring& path);
		static uint64 getFileSize(const std::wstring& filename);

		static bool readFile(const std::wstring& filename, std::vector<uint8>& outData);
		static bool saveFile(const std::wstring& filename, const void* data, size_t size);
		static InputStream* createInputStream(const std::wstring& filename);

		static void createDirectory(const std::wstring& path);
		static void listFiles(const std::wstring& path, bool recursive, std::vector<FileEntry>& outFileEntries);
		static void listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileEntry>& outFileEntries);
		static void listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories);

		static void normalizePath(std::wstring& path, bool isDirectory);
		static const std::wstring& normalizePath(const std::wstring& path, std::wstring& tempBuffer, bool isDirectory);

		static std::wstring getCurrentDirectory();
		static void setCurrentDirectory(const std::wstring& path);

		static void splitPath(const std::string& path, std::string* directory, std::string* name, std::string* extension);
		static void splitPath(const std::wstring& path, std::wstring* directory, std::wstring* name, std::wstring* extension);

		static bool matchesMask(const std::wstring& filename, const std::wstring& filemask);
		static void filterMaskMatches(std::vector<FileIO::FileEntry>& fileEntries, const std::wstring& filemask);
	};

}
