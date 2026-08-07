/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"
#include <fstream>

#ifdef PLATFORM_WINDOWS
	#include <filesystem>
	namespace std_filesystem = std::filesystem;
	#define USE_STD_FILESYSTEM

	#include <direct.h>
	#include <io.h>

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC) || defined(PLATFORM_WEB)
	#include <filesystem>
	namespace std_filesystem = std::filesystem;
	#define USE_STD_FILESYSTEM

	#include <dirent.h>
	#include <sys/stat.h>

#elif defined(PLATFORM_ANDROID) || defined(PLATFORM_SWITCH) || defined(PLATFORM_IOS) || defined(PLATFORM_VITA)
	// This requires Android NDK 22
	#include <filesystem>
	namespace std_filesystem = std::filesystem;
	#define USE_STD_FILESYSTEM

	#include <dirent.h>
	#include <sys/stat.h>
#endif
#if defined(PLATFORM_WEB)
	#include <emscripten.h>
#endif


namespace rmx
{
	namespace
	{
		bool createDir(const WString& path, bool recursive)
		{
		#ifdef USE_STD_FILESYSTEM
			// Create directory or hierarchy of directories (if recursive == true)
			if (recursive)
			{
				WString subpath;
				subpath.expand(path.length() + 1);
				int pos = 0;
				while (pos < path.length())
				{
					pos = path.findChars(L"/\\", pos + 1, +1);
					subpath.makeSubString(path, 0, pos);
					if (!std_filesystem::exists(*subpath))
					{
						if (!createDir(subpath, false))
							return false;
					}
				}
				return true;
			}
			else
			{
				// Create a single directory
			#ifdef PLATFORM_WINDOWS
				return (_wmkdir(*path) == 0);
			#elif defined(USE_UTF8_PATHS)
				return (mkdir(*path.toUTF8(), 0777) == 0);		// Probably the only time I ever used octal notation...
			#else
				#error "Unsupported platform"
			#endif
			}
		#else
			// TODO
			RMX_ASSERT(false, "Not implemented: createDir (in FileIO.cpp)");
			return false;
		#endif
		}

		bool matchesPattern(const wchar_t* string, const wchar_t* pattern)
		{
			switch (pattern[0])
			{
				case L'\0':	return (string[0] == 0);
				case L'?':	return (string[0] != 0) && matchesPattern(string + 1, pattern + 1);
				case L'*':	return (matchesPattern(string, pattern + 1)) || (string[0] != 0 && matchesPattern(string + 1, pattern));
				default:	return (pattern[0] == string[0]) && matchesPattern(string + 1, pattern + 1);
			}
		}

		void listDirectoryContentInternal(std::vector<FileIO::FileEntry>* outFileEntries, std::vector<std::wstring>* outSubDirectories, std::wstring_view basePath_, std::wstring_view filemask, bool recursive)
		{
			const std::wstring basePath(basePath_);
			std::vector<std::wstring> subDirectoriesBuffer;
			std::vector<std::wstring>& subDirectories = (nullptr != outSubDirectories) ? *outSubDirectories : subDirectoriesBuffer;

		#ifdef PLATFORM_WINDOWS

			// Find files
			_wfinddata_t fileinfo;
			intptr_t handle = _wfindfirst((basePath + L"*").c_str(), &fileinfo);
			bool success = (handle != -1);
			while (success)
			{
				// Ignore "." and ".." entries
				const std::wstring name = fileinfo.name;
				if (name != L"." && name != L"..")
				{
					const bool isDirectory = (fileinfo.attrib & 0x10) != 0;
					if (isDirectory)
					{
						// Directory
						if (recursive || nullptr != outSubDirectories)
						{
							subDirectories.push_back(name);
						}
					}
					else
					{
						// File
						if (nullptr != outFileEntries)
						{
							// Check for wildcard match
							const WString filename = fileinfo.name;
							if (filemask.empty() || matchesPattern(*filename, filemask.data()))
							{
								FileIO::FileEntry& entry = vectorAdd(*outFileEntries);
								entry.mFilename = fileinfo.name;
								entry.mPath = basePath;
								entry.mTime = fileinfo.time_write;
								entry.mSize = (size_t)fileinfo.size;
							}
						}
					}
				}

				success = (_wfindnext(handle, &fileinfo) == 0);
			}
			_findclose(handle);

		#elif defined(USE_UTF8_PATHS)

			const std::string basePathUTF8 = *WString(basePath).toUTF8();
			DIR* dp = opendir(basePathUTF8.c_str());
			if (nullptr == dp)
				return;

			while (true)
			{
				struct dirent* dirp = readdir(dp);
				if (nullptr == dirp)
					break;

				// Ignore "." and ".." entries
				std::string name = dirp->d_name;
				if (name == "." || name == "..")
					continue;

				struct stat fileinfo;
				if (stat((basePathUTF8 + name).c_str(), &fileinfo) != 0)
					continue;

				if (S_ISDIR(fileinfo.st_mode))
				{
					// Directory
					if (recursive || nullptr != outSubDirectories)
					{
						subDirectories.emplace_back(*String(name).toWString());
					}
				}
				else
				{
					// File
					if (nullptr != outFileEntries)
					{
						// Check for wildcard match
						const WString filename = String(name).toWString();
						if (filemask.empty() || matchesPattern(*filename, filemask.data()))
						{
							FileIO::FileEntry& entry = vectorAdd(*outFileEntries);
							entry.mFilename = filename.toStdWString();
							entry.mPath = basePath;
							entry.mTime = fileinfo.st_mtime;
							entry.mSize = (size_t)fileinfo.st_size;
						}
					}
				}
			}
			closedir(dp);

		#else
			#error "Unsupported platform"
		#endif

			if (recursive)
			{
				// Go into subdirectories
				for (const std::wstring& subDirectory : subDirectories)
				{
					listDirectoryContentInternal(outFileEntries, outSubDirectories, basePath + subDirectory + L'/', filemask, recursive);
				}
			}
		}

		struct FileNameCharacterValidityLookup
		{
			explicit FileNameCharacterValidityLookup(bool allowSlash)
			{
				for (int k = 32; k < 128; ++k)		// Exclude non-printable ASCII characters
				{
					mIsCharacterValid.setBit(k);
				}
				const std::wstring invalidCharacters = L"\"<>:|?*";		// Technically not all of these characters are problematic on all platforms, but we treat them all as illegal to avoid cross-platform issues
				for (wchar_t ch : invalidCharacters)
				{
					mIsCharacterValid.clearBit(ch);
				}
				if (!allowSlash)
				{
					mIsCharacterValid.clearBit(L'/');
					mIsCharacterValid.clearBit(L'\\');
				}
			}

			bool isValid(wchar_t ch) const  { return ch >= 128 || mIsCharacterValid.isBitSet(ch); }

			bool allValid(std::wstring_view str) const
			{
				for (wchar_t ch : str)
				{
					if (!isValid(ch))
						return false;
				}
				return true;
			}

			BitArray<128> mIsCharacterValid;
		};

		static FileNameCharacterValidityLookup mFileNameCharacterValidityLookup(false);
		static FileNameCharacterValidityLookup mFilePathCharacterValidityLookup(true);

	}


	bool FileIO::exists(std::wstring_view path)
	{
	#ifdef USE_STD_FILESYSTEM
		const std_filesystem::path fspath(path.data());
		return std_filesystem::exists(fspath);
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::exists");
		return false;
	#endif
	}

	bool FileIO::isFile(std::wstring_view path)
	{
	#ifdef USE_STD_FILESYSTEM
		const std_filesystem::path fspath(path.data());
		return std_filesystem::is_regular_file(fspath);
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::isFile");
		return false;
	#endif
	}

	bool FileIO::isDirectory(std::wstring_view path)
	{
	#ifdef USE_STD_FILESYSTEM
		const std_filesystem::path fspath(path.data());
		return std_filesystem::is_directory(fspath);
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::isDirectory");
		return false;
	#endif
	}

	bool FileIO::getFileSize(std::wstring_view filename, uint64& outSize)
	{
		mLastErrorCode.clear();

	#if defined(USE_STD_FILESYSTEM) && !defined(PLATFORM_MAC)
		const std_filesystem::path fspath(filename.data());
		const std::uintmax_t size = std_filesystem::file_size(fspath, mLastErrorCode);
		if (mLastErrorCode)
			return false;
		outSize = (uint64)size;
		return true;
	#else
		FileHandle file(filename, FILE_ACCESS_READ);
		if (!file.isOpen())
			return false;
		outSize = file.getSize();
		return true;
	#endif
	}

	bool FileIO::getFileTime(std::wstring_view filename, time_t& outTime)
	{
		mLastErrorCode.clear();

	#if defined(USE_STD_FILESYSTEM)
		const std_filesystem::path fspath(filename.data());
		const std_filesystem::file_time_type time = std_filesystem::last_write_time(fspath, mLastErrorCode);
		if (mLastErrorCode)
			return false;

		// This is the C++17 solution for converting the time -- see https://stackoverflow.com/questions/61030383/how-to-convert-stdfilesystemfile-time-type-to-time-t
		const std::chrono::system_clock::time_point timePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std_filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		outTime = std::chrono::system_clock::to_time_t(timePoint);
		return true;
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::getFileTime");
		return false;
	#endif
	}

	bool FileIO::readFile(std::wstring_view filename, std::vector<uint8>& outData)
	{
		// Read from file system
	#ifdef USE_UTF8_PATHS
		std::ifstream stream(rmx::convertToUTF8(filename).c_str(), std::ios::binary);
	#else
		std::ifstream stream(filename.data(), std::ios::binary);
	#endif
		if (!stream.good())
			return false;

		stream.seekg(0, std::ios::end);
		const uint32 size = (uint32)stream.tellg();
		stream.seekg(0, std::ios::beg);

		outData.resize(size);
		if (size != 0)
		{
			stream.read((char*)&outData[0], size);
		}
		return true;
	}

	bool FileIO::saveFile(std::wstring_view filename, const void* data, size_t size)
	{
		// Create directory if needed
		const size_t slashPosition = filename.find_last_of(L"/\\");
		if (slashPosition != std::string::npos)
		{
			createDirectory(filename.substr(0, slashPosition));
		}

	#ifdef USE_UTF8_PATHS
		std::ofstream stream(rmx::convertToUTF8(filename).c_str(), std::ios::binary);
	#else
		std::ofstream stream(filename.data(), std::ios::binary);
	#endif
		if (!stream.good())
			return false;

		if (size != 0 && nullptr != data)
		{
			stream.write((char*)data, size);
		}
		stream.close();

		#if defined(PLATFORM_WEB)
			EM_ASM({
				FS.syncfs(false, function(err)
				{
					if (err) console.error('FS.syncfs failed:', err);
				});
			});
		#endif
		return true;
	}

	InputStream* FileIO::createInputStream(std::wstring_view filename)
	{
		InputStream* inputStream = new FileInputStream(filename);
		if (!inputStream->valid())
		{
			delete inputStream;
			return nullptr;
		}
		return inputStream;
	}

	bool FileIO::renameFile(const std::wstring& oldFilename, const std::wstring& newFilename)
	{
		mLastErrorCode.clear();

	#if defined(USE_STD_FILESYSTEM)
		const std_filesystem::path fspathOld(oldFilename.data());
		const std_filesystem::path fspathNew(newFilename.data());
		std_filesystem::rename(fspathOld, fspathNew, mLastErrorCode);
		return !mLastErrorCode;
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::renameFile");
		return false;
	#endif
	}

	bool FileIO::renameDirectory(const std::wstring& oldFilename, const std::wstring& newFilename)
	{
		mLastErrorCode.clear();

	#if defined(USE_STD_FILESYSTEM)
		const std_filesystem::path fspathOld(oldFilename.data());
		const std_filesystem::path fspathNew(newFilename.data());
		std_filesystem::rename(fspathOld, fspathNew, mLastErrorCode);
		return !mLastErrorCode;
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::renameDirectory");
		return false;
	#endif
	}

	bool FileIO::removeFile(std::wstring_view path)
	{
		mLastErrorCode.clear();

	#if defined(USE_STD_FILESYSTEM)
		const std_filesystem::path fspath(path);
		std_filesystem::remove(fspath, mLastErrorCode);
		return !mLastErrorCode;
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::removeFile");
		return false;
	#endif
	}

	bool FileIO::removeDirectory(std::wstring_view path)
	{
		mLastErrorCode.clear();

	#if defined(USE_STD_FILESYSTEM)
		const std_filesystem::path fspath(path);
		std_filesystem::remove_all(fspath, mLastErrorCode);
		return !mLastErrorCode;
	#else
		RMX_ASSERT(false, "Not implemented: FileIO::removeDirectory");
		return false;
	#endif
	}

	bool FileIO::createDirectory(std::wstring_view path)
	{
		return createDir(path, true);
	}

	void FileIO::listFiles(std::wstring_view path, bool recursive, std::vector<FileEntry>& outFileEntries)
	{
		std::wstring basePath = std::wstring(path);
		normalizePath(basePath, true);
		listDirectoryContentInternal(&outFileEntries, nullptr, basePath, L"", recursive);
	}

	void FileIO::listFilesByMask(std::wstring_view filemask_, bool recursive, std::vector<FileEntry>& outFileEntries)
	{
		std::wstring filemask = std::wstring(filemask_);
		normalizePath(filemask, false);

		std::wstring basePath;
		std::wstring mask;
		{
			const WString input(filemask);
			const int lastSlashPosition = input.findChar(L'/', input.length() - 1, -1);
			if (lastSlashPosition >= 0)
			{
				basePath = *input.getSubString(0, lastSlashPosition + 1);	// Including slash!
				mask = *input.getSubString(lastSlashPosition + 1, -1);
			}
			else
			{
				basePath = L"";
				mask = *input;
			}
		}

		listDirectoryContentInternal(&outFileEntries, nullptr, basePath, mask, recursive);
	}

	void FileIO::listDirectories(std::wstring_view path, std::vector<std::wstring>& outDirectories)
	{
		std::wstring basePath = std::wstring(path);
		normalizePath(basePath, true);
		listDirectoryContentInternal(nullptr, &outDirectories, basePath, L"", false);
	}

	bool FileIO::isDirectoryPath(std::wstring_view path)
	{
		if (path.empty())
			return false;
		return (path.back() == L'/' || path.back() == L'\\');
	}

	void FileIO::normalizePath(std::wstring& path, bool isDirectory)
	{
		if (path.empty())
			return;

		std::wstring tempBuffer;
		path = normalizePath(path, tempBuffer, isDirectory);
	}

	std::wstring_view FileIO::normalizePath(std::wstring_view path, std::wstring& tempBuffer, bool isDirectory)
	{
		if (path.empty())
			return path;

	#ifdef PLATFORM_VITA
		// Assume that the path is always normal when it begins with ux0:/data
		const WString t(path);
		if (t.startsWith(L"ux0:/data/") || t.startsWith(L"ux0:data/"))
			return path;
	#endif

		// Split the path into a list of directory / file names
		std::wstring_view names[32];
		size_t numNames = 0;
		size_t numNonRemovableNames = 0;	// Counts names like "" and ".." at the front that can't be removed by ".."
		bool anyChange = false;

		for (size_t pos = 0; pos < path.length(); ++pos)
		{
			// Search for the next slash
			const size_t startPos = pos;
			while (pos < path.length() && path[pos] != L'/' && path[pos] != L'\\')
			{
				++pos;
			}

			// Backward slashes count as a change, we want to correct that to forward slashes
			if (pos < path.length() && path[pos] == L'\\')
			{
				anyChange = true;
			}

			if (pos == startPos)
			{
				if (pos == 0)
				{
					// A slash at the start, that's okay
					RMX_CHECK(numNames < 32, "Too many directory names in path: " << WString(path).toStdString(), return path);
					names[numNames] = std::wstring_view();
					++numNames;
					++numNonRemovableNames;
					continue;
				}

				if (pos >= path.length())
				{
					// A slash at the end, that's okay as well (no need to add as an empty name)
					if (isDirectory)
						continue;
				}

				anyChange = true;
				continue;
			}

			// Handle special directory names
			if (path[startPos] == L'.')
			{
				if (pos - startPos == 1)
				{
					// Skip single dot directory names
					anyChange = true;
					continue;
				}

				if (pos - startPos == 2 && path[startPos + 1] == L'.')
				{
					// Two dots means going one directory up, at least if that's possible
					if (numNames > numNonRemovableNames)
					{
						--numNames;
						anyChange = true;
						continue;
					}
					++numNonRemovableNames;
				}
			}

			// It's a normal directory name
			RMX_CHECK(numNames < 32, "Too many directory names in path: " << WString(path).toStdString(), return path);
			names[numNames] = std::wstring_view(&path[startPos], pos - startPos);
			++numNames;
		}

		// Force directories to end in a slash in any case
		if (!anyChange && isDirectory && !path.empty() && path.back() != L'/')
		{
			anyChange = true;
		}

		if (anyChange)
		{
			// Build output path from collected names
			tempBuffer.clear();
			if (numNames > 0)
			{
				tempBuffer.reserve(path.length());
				tempBuffer += names[0];
				for (size_t k = 1; k < numNames; ++k)
				{
					tempBuffer += L'/';
					tempBuffer += names[k];
				}
				if (isDirectory)
					tempBuffer += L'/';
			}
			return tempBuffer;
		}
		else
		{
			// No change, so input path is fine as result
			return path;
		}
	}

	bool FileIO::isValidFileName(std::wstring_view filename)
	{
		if (filename.empty())
			return false;
		if (!mFileNameCharacterValidityLookup.allValid(filename))
			return false;
		if (filename == L"." || rmx::startsWith(filename, L".."))
			return false;
		return true;
	}

	bool FileIO::isValidPathName(std::wstring_view pathname)
	{
		if (pathname.empty())
			return false;
		if (!mFilePathCharacterValidityLookup.allValid(pathname))
			return false;
		return true;
	}

	void FileIO::sanitizeFileName(std::wstring& filename)
	{
		// Replace all characters that are not valid in a file name
		//  -> Note that this does replace slashes with underscores
		for (wchar_t& ch : filename)
		{
			if (!mFileNameCharacterValidityLookup.isValid(ch))
				ch = L'_';
		}
	}

	void FileIO::sanitizePathName(std::wstring& pathname)
	{
		// Replace all characters that are not valid in a path name
		//  -> Note that this does not replace slashes (though it converts backslashes to forward slashes)
		for (wchar_t& ch : pathname)
		{
			if (!mFileNameCharacterValidityLookup.isValid(ch))
				ch = L'_';
			else if (ch == L'\\')
				ch = L'/';
		}
	}

	std::wstring FileIO::getCurrentDirectory()
	{
	#ifdef USE_STD_FILESYSTEM
		return std_filesystem::current_path().wstring();
	#else
		return L"";
	#endif
	}

	void FileIO::setCurrentDirectory(std::wstring_view path)
	{
	#ifdef USE_STD_FILESYSTEM
		const std_filesystem::path fspath(path.data());
		std_filesystem::current_path(fspath);
	#endif
	}

	void FileIO::splitPath(std::string_view path, std::string* directory, std::string* name, std::string* extension)
	{
		const std::size_t slash = path.find_last_of("/\\");
		if (nullptr != directory)
		{
			if (slash != std::wstring::npos)
				*directory = path.substr(0, slash);
			else
				directory->clear();
		}

		const std::size_t dot = path.find_last_of('.');
		if (dot != std::wstring::npos && dot > slash)
		{
			if (nullptr != name)
				*name = path.substr(slash + 1, dot - slash - 1);
			if (nullptr != extension)
				*extension = path.substr(dot + 1);
		}
		else
		{
			if (nullptr != name)
				*name = path.substr(slash + 1);
			if (nullptr != extension)
				extension->clear();
		}
	}

	void FileIO::splitPath(std::wstring_view path, std::wstring* directory, std::wstring* name, std::wstring* extension)
	{
		const std::size_t slash = path.find_last_of(L"/\\");
		if (nullptr != directory)
		{
			if (slash != std::wstring::npos)
				*directory = path.substr(0, slash);
			else
				directory->clear();
		}

		const std::size_t dot = path.find_last_of(L'.');
		if (dot != std::wstring::npos && (dot > slash || slash == std::wstring::npos))
		{
			if (nullptr != name)
				*name = path.substr(slash + 1, dot - slash - 1);
			if (nullptr != extension)
				*extension = path.substr(dot + 1);
		}
		else
		{
			if (nullptr != name)
				*name = path.substr(slash + 1);
			if (nullptr != extension)
				extension->clear();
		}
	}

	bool FileIO::matchesMask(std::wstring_view filename, std::wstring_view filemask)
	{
		const WString mask(filemask);

		RMX_ASSERT(mask.findChar(L'?', 0, 1) >= mask.length(), "Questionmark wildcard character in file mask is not allowed");
		const int wildcardPosition = mask.findChar(L'*', 0, 1);
		if (wildcardPosition >= mask.length())
		{
			// Must an exact match
			return (filename == filemask);
		}
		RMX_ASSERT(mask.findChar(L'*', wildcardPosition + 1, 1) >= mask.length(), "Multiple wildcard characters in file mask are not allowed");

		const WString name(filename);
		const WString prefix = mask.getSubString(0, wildcardPosition);
		if (prefix.length() > 0 && !name.startsWith(*prefix))
			return false;

		const WString postfix = mask.getSubString(wildcardPosition + 1, -1);
		if (postfix.length() > 0 && !name.endsWith(*postfix, postfix.length()))
			return false;

		return true;
	}

	void FileIO::filterMaskMatches(std::vector<FileIO::FileEntry>& fileEntries, std::wstring_view filemask)
	{
		if (fileEntries.empty())
			return;

		const size_t position = filemask.find_last_of(L'/');
		const std::wstring_view mask = (position == std::wstring::npos) ? filemask : filemask.substr(position + 1);

		size_t insertionIndex = 0;
		for (size_t index = 0; index < fileEntries.size(); ++index)
		{
			const bool matches = matchesMask(fileEntries[index].mFilename, mask);
			if (matches)
			{
				if (index != insertionIndex)
				{
					fileEntries[insertionIndex] = fileEntries[index];
				}
				++insertionIndex;
			}
		}
		fileEntries.resize(insertionIndex);
	}

}
