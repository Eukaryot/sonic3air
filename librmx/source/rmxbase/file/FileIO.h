/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
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
		static bool exists(std::wstring_view path);
		static bool isFile(std::wstring_view path);
		static bool isDirectory(std::wstring_view path);

		static bool getFileSize(std::wstring_view filename, uint64& outSize);
		static bool getFileTime(std::wstring_view filename, time_t& outTime);

		static bool readFile(std::wstring_view filename, std::vector<uint8>& outData);
		static bool saveFile(std::wstring_view filename, const void* data, size_t size);
		static InputStream* createInputStream(std::wstring_view filename);

		static bool renameFile(const std::wstring& oldFilename, const std::wstring& newFilename);
		static bool renameDirectory(const std::wstring& oldFilename, const std::wstring& newFilename);

		static bool removeFile(std::wstring_view path);
		static bool removeDirectory(std::wstring_view path);

		static void createDirectory(std::wstring_view path);

		static void listFiles(std::wstring_view path, bool recursive, std::vector<FileEntry>& outFileEntries);
		static void listFilesByMask(std::wstring_view filemask, bool recursive, std::vector<FileEntry>& outFileEntries);
		static void listDirectories(std::wstring_view path, std::vector<std::wstring>& outDirectories);

		static bool isDirectoryPath(std::wstring_view path);
		static void normalizePath(std::wstring& path, bool isDirectory);
		static std::wstring_view normalizePath(std::wstring_view path, std::wstring& tempBuffer, bool isDirectory);

		static bool isValidFileName(std::wstring_view filename);
		static void sanitizeFileName(std::wstring& filename);

		static std::wstring getCurrentDirectory();
		static void setCurrentDirectory(std::wstring_view path);

		static void splitPath(std::string_view path, std::string* directory, std::string* name, std::string* extension);
		static void splitPath(std::wstring_view path, std::wstring* directory, std::wstring* name, std::wstring* extension);

		static bool matchesMask(std::wstring_view filename, std::wstring_view filemask);
		static void filterMaskMatches(std::vector<FileIO::FileEntry>& fileEntries, std::wstring_view filemask);
	};

}
