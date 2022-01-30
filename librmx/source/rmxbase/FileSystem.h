/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class InputStream;

namespace rmx
{
	class FileProvider;


	class API_EXPORT FileSystem
	{
	friend class FileProvider;

	public:
		FileSystem();
		virtual ~FileSystem();

		virtual bool exists(std::wstring_view filename);
		virtual uint64 getFileSize(std::wstring_view filename);

		virtual bool readFile(std::wstring_view filename, std::vector<uint8>& outData);
		virtual bool saveFile(std::wstring_view filename, const void* data, size_t size);
		virtual InputStream* createInputStream(std::wstring_view filename);

		virtual void createDirectory(std::wstring_view path);
		virtual void listFiles(std::wstring_view path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries);
		virtual void listFilesByMask(std::wstring_view filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries);
		virtual void listDirectories(std::wstring_view path, std::vector<std::wstring>& outDirectories);

		// Wrapper functions
		bool exists(std::string_view path);
		uint64 getFileSize(std::string_view filename);
		bool readFile(std::string_view filename, std::vector<uint8>& outData);
		bool saveFile(std::wstring_view filename, const std::vector<uint8>& data);
		bool saveFile(std::string_view filename, const std::vector<uint8>& data);
		bool saveFile(std::string_view filename, const void* data, size_t size);
		InputStream* createInputStream(std::string_view filename);

		// File provider & mount points management
		void addManagedFileProvider(FileProvider& fileProvider);
		void clearMountPoints();
		void addMountPoint(FileProvider& fileProvider, std::wstring_view mountPoint, std::wstring_view prefixReplacement, int priority);

	public:
		// Misc functions
		static void normalizePath(std::wstring& path, bool isDirectory);
		static std::wstring_view normalizePath(std::wstring_view path, std::wstring& tempBuffer, bool isDirectory);

		static std::wstring getCurrentDirectory();
		static void setCurrentDirectory(std::wstring_view path);

		static void splitPath(std::string_view path, std::string* directory, std::string* name, std::string* extension);
		static void splitPath(std::wstring_view path, std::wstring* directory, std::wstring* name, std::wstring* extension);

	private:
		struct MountPoint
		{
			FileProvider* mFileProvider;
			int mPriority = 0;
			std::wstring mMountPoint;
			std::wstring mPrefixReplacement;
			bool mNeedsPrefixConversion = false;	// Set if mount point and prefix replacement are different
		};

	private:
		void onFileProviderDestroyed(FileProvider& fileProvider);

		const std::wstring* applyMountPoint(const MountPoint& mountPoint, const std::wstring& inPath, std::wstring& tempPath) const;
		void removeMountPointPath(const MountPoint& mountPoint, std::wstring& path) const;

	private:
		RealFileProvider mDefaultRealFileProvider;
		std::set<FileProvider*> mManagedFileProviders;	// List of file providers that get deleted automatically with this file system -- though file providers that have mount points here can be managed outside as well, they're not in this list then
		std::vector<MountPoint> mMountPoints;

		mutable std::wstring mTempPath;		// Only for temporary internal use
		mutable std::wstring mTempPath2;	// Only for temporary internal use
	};

}
