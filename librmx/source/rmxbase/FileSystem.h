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

		virtual bool exists(const std::wstring& filename);
		virtual uint64 getFileSize(const std::wstring& filename);

		virtual bool readFile(const std::wstring& filename, std::vector<uint8>& outData);
		virtual bool saveFile(const std::wstring& filename, const void* data, size_t size);
		virtual InputStream* createInputStream(const std::wstring& filename);

		virtual void createDirectory(const std::wstring& path);
		virtual void listFiles(const std::wstring& path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries);
		virtual void listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries);
		virtual void listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories);

		// Wrapper functions
		bool exists(const std::string& path);
		uint64 getFileSize(const std::string& filename);
		bool readFile(const std::string& filename, std::vector<uint8>& outData);
		bool saveFile(const std::wstring& filename, const std::vector<uint8>& data);
		bool saveFile(const std::string& filename, const std::vector<uint8>& data);
		bool saveFile(const std::string& filename, const void* data, size_t size);
		InputStream* createInputStream(const std::string& filename);

		// File provider & mount points management
		void addManagedFileProvider(FileProvider& fileProvider);
		void clearMountPoints();
		void addMountPoint(FileProvider& fileProvider, const std::wstring& mountPoint, const std::wstring& prefixReplacement, int priority);

	public:
		// Misc functions
		static void normalizePath(std::wstring& path, bool isDirectory);
		static const std::wstring& normalizePath(const std::wstring& path, std::wstring& tempBuffer, bool isDirectory);

		static std::wstring getCurrentDirectory();
		static void setCurrentDirectory(const std::wstring& path);

		static void splitPath(const std::string& path, std::string* directory, std::string* name, std::string* extension);
		static void splitPath(const std::wstring& path, std::wstring* directory, std::wstring* name, std::wstring* extension);

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
		const std::wstring* removeMountPointPath(const MountPoint& mountPoint, const std::wstring& inPath, std::wstring& tempPath) const;
		void removeMountPointPath(const MountPoint& mountPoint, std::wstring& path) const;

	private:
		RealFileProvider mDefaultRealFileProvider;
		std::set<FileProvider*> mManagedFileProviders;	// List of file providers that get deleted automatically with this file system -- though file providers that have mount points here can be managed outside as well, they're not in this list then
		std::vector<MountPoint> mMountPoints;

		mutable std::wstring mTempPath;		// Only for temporary internal use
		mutable std::wstring mTempPath2;	// Only for temporary internal use
	};

}
