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
	class FileSystem;


	class API_EXPORT FileProvider
	{
	friend class FileSystem;

	public:
		virtual ~FileProvider();

		virtual bool exists(const std::wstring& filename)  { return false; }
		virtual bool getFileSize(const std::wstring& filename, uint64& outFileSize)  { return false; }
		virtual bool getFileTime(const std::wstring& filename, time_t& outFileTime)  { return false; }
		virtual bool readFile(const std::wstring& filename, std::vector<uint8>& outData)  { return false; }

		virtual bool renameFile(const std::wstring& oldFilename, const std::wstring& newFilename)  { return false; }
		virtual bool listFiles(const std::wstring& path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries)  { return false; }
		virtual bool listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries)  { return false; }
		virtual bool listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories)  { return false; }
		virtual InputStream* createInputStream(const std::wstring& filename)  { return nullptr; }

	protected:
		std::set<FileSystem*> mRegisteredMountPointFileSystems;		// Usually just one
	};


	class API_EXPORT RealFileProvider : public FileProvider
	{
	public:
		bool exists(const std::wstring& filename) override;
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
