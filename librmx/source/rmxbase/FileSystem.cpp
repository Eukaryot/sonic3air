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

	FileSystem::FileSystem()
	{
		// By default, add a real file provider with mounted at root
		addMountPoint(mDefaultRealFileProvider, L"", L"", 0);
	}

	FileSystem::~FileSystem()
	{
		// Unregister from the file providers
		for (size_t k = 0; k < mMountPoints.size(); ++k)
		{
			mMountPoints[k].mFileProvider->mRegisteredMountPointFileSystems.erase(this);
		}

		// Clear the mount points before destroying the managed file provider, so that the calls to "addManagedFileProvider" made by the file provider descructors won't need to do anything
		mMountPoints.clear();

		// Destroy the managed file provider
		for (FileProvider* fileProvider : mManagedFileProviders)
		{
			delete fileProvider;
		}
		mManagedFileProviders.clear();
	}

	bool FileSystem::exists(const std::wstring& filename)
	{
		mTempPath2 = normalizePath(filename, mTempPath2, false);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				if (mountPoint.mFileProvider->exists(*localPath))
					return true;
			}
		}
		return false;
	}

	uint64 FileSystem::getFileSize(const std::wstring& filename)
	{
		mTempPath2 = normalizePath(filename, mTempPath2, false);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				uint64 fileSize = 0;
				if (mountPoint.mFileProvider->getFileSize(*localPath, fileSize))
					return fileSize;
			}
		}
		return 0;
	}

	bool FileSystem::readFile(const std::wstring& filename, std::vector<uint8>& outData)
	{
		mTempPath2 = normalizePath(filename, mTempPath2, false);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				if (mountPoint.mFileProvider->readFile(*localPath, outData))
					return true;
			}
		}
		return false;
	}

	bool FileSystem::saveFile(const std::wstring& filename, const void* data, size_t size)
	{
		// TODO: Use file providers here as well
		mTempPath2 = normalizePath(filename, mTempPath2, false);
		return FileIO::saveFile(mTempPath2, data, size);
	}

	InputStream* FileSystem::createInputStream(const std::wstring& filename)
	{
		mTempPath2 = normalizePath(filename, mTempPath2, false);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				InputStream* stream = mountPoint.mFileProvider->createInputStream(*localPath);
				if (nullptr != stream)
					return stream;
			}
		}
		return nullptr;
	}

	void FileSystem::createDirectory(const std::wstring& path)
	{
		// TODO: Use file providers here as well
		mTempPath2 = normalizePath(path, mTempPath2, true);
		FileIO::createDirectory(mTempPath2);
	}

	void FileSystem::listFiles(const std::wstring& path, bool recursive, std::vector<rmx::FileIO::FileEntry>& outEntries)
	{
		mTempPath2 = normalizePath(path, mTempPath2, false);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				mountPoint.mFileProvider->listFiles(*localPath, recursive, outEntries);

				if (mountPoint.mNeedsPrefixConversion || !mountPoint.mPrefixReplacement.empty())
				{
					for (FileIO::FileEntry& fileEntry : outEntries)
					{
						removeMountPointPath(mountPoint, fileEntry.mPath);
					}
				}
			}
		}
	}

	void FileSystem::listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<rmx::FileIO::FileEntry>& outEntries)
	{
		mTempPath2 = normalizePath(filemask, mTempPath2, false);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				mountPoint.mFileProvider->listFilesByMask(*localPath, recursive, outEntries);

				if (mountPoint.mNeedsPrefixConversion || !mountPoint.mPrefixReplacement.empty())
				{
					for (FileIO::FileEntry& fileEntry : outEntries)
					{
						removeMountPointPath(mountPoint, fileEntry.mPath);
					}
				}
			}
		}
	}

	void FileSystem::listDirectories(const std::wstring& path, std::vector<std::wstring>& outEntries)
	{
		mTempPath2 = normalizePath(path, mTempPath2, true);
		for (MountPoint& mountPoint : mMountPoints)
		{
			const std::wstring* localPath = applyMountPoint(mountPoint, mTempPath2, mTempPath);
			if (nullptr != localPath)
			{
				mountPoint.mFileProvider->listDirectories(*localPath, outEntries);
			}
			else
			{
				// Handle the special case that the mount point includes the given path
				//  -> In this case, we want the mount point itself to act as a virtual directory
				if (startsWith(mountPoint.mMountPoint, mTempPath2))
				{
					const size_t startPos = mTempPath2.size();
					size_t endPos = startPos;
					while (endPos < mountPoint.mMountPoint.size() && mountPoint.mMountPoint[endPos] != '/')
					{
						++endPos;
					}
					if (endPos < mountPoint.mMountPoint.size())
					{
						outEntries.emplace_back(mountPoint.mMountPoint, startPos, endPos - startPos);
					}
				}
			}
		}
	}

	bool FileSystem::exists(const std::string& path)
	{
		return exists(String(path).toStdWString());
	}

	uint64 FileSystem::getFileSize(const std::string& filename)
	{
		return getFileSize(String(filename).toStdWString());
	}

	bool FileSystem::readFile(const std::string& filename, std::vector<uint8>& outData)
	{
		return readFile(String(filename).toStdWString(), outData);
	}

	bool FileSystem::saveFile(const std::wstring& filename, const std::vector<uint8>& data)
	{
		return saveFile(filename, data.empty() ? nullptr : &data[0], data.size());
	}

	bool FileSystem::saveFile(const std::string& filename, const std::vector<uint8>& data)
	{
		return saveFile(String(filename).toStdWString(), data);
	}

	bool FileSystem::saveFile(const std::string& filename, const void* data, size_t size)
	{
		return saveFile(String(filename).toStdWString(), data, size);
	}

	InputStream* FileSystem::createInputStream(const std::string& filename)
	{
		return createInputStream(String(filename).toStdWString());
	}

	void FileSystem::addManagedFileProvider(FileProvider& fileProvider)
	{
		mManagedFileProviders.insert(&fileProvider);
	}

	void FileSystem::clearMountPoints()
	{
		// This also removes the default real file provider -- this way you can get rid of it
		mMountPoints.clear();
	}

	void FileSystem::addMountPoint(FileProvider& fileProvider, const std::wstring& mountPoint, const std::wstring& prefixReplacement, int priority)
	{
		MountPoint& newMountPoint = vectorAdd(mMountPoints);
		newMountPoint.mFileProvider = &fileProvider;
		newMountPoint.mPriority = priority;
		if (!mountPoint.empty() || !prefixReplacement.empty())
		{
			newMountPoint.mMountPoint = FileIO::normalizePath(mountPoint, newMountPoint.mMountPoint, true);
			newMountPoint.mPrefixReplacement = FileIO::normalizePath(prefixReplacement, newMountPoint.mPrefixReplacement, true);
			newMountPoint.mNeedsPrefixConversion = (newMountPoint.mMountPoint != newMountPoint.mPrefixReplacement);
		}

		fileProvider.mRegisteredMountPointFileSystems.insert(this);
		std::sort(mMountPoints.begin(), mMountPoints.end(), [](const MountPoint& a, const MountPoint& b) { return a.mPriority > b.mPriority; } );
	}

	void FileSystem::normalizePath(std::wstring& path, bool isDirectory)
	{
		FileIO::normalizePath(path, isDirectory);
	}

	const std::wstring& FileSystem::normalizePath(const std::wstring& path, std::wstring& tempBuffer, bool isDirectory)
	{
		return FileIO::normalizePath(path, tempBuffer, isDirectory);
	}

	std::wstring FileSystem::getCurrentDirectory()
	{
		return FileIO::getCurrentDirectory();
	}

	void FileSystem::setCurrentDirectory(const std::wstring& path)
	{
		FileIO::setCurrentDirectory(path);
	}

	void FileSystem::splitPath(const std::string& path, std::string* directory, std::string* name, std::string* extension)
	{
		FileIO::splitPath(path, directory, name, extension);
	}

	void FileSystem::splitPath(const std::wstring& path, std::wstring* directory, std::wstring* name, std::wstring* extension)
	{
		FileIO::splitPath(path, directory, name, extension);
	}

	void FileSystem::onFileProviderDestroyed(FileProvider& fileProvider)
	{
		// Remove all mount points of this file provider
		for (size_t k = 0; k < mMountPoints.size(); ++k)
		{
			if (&fileProvider == mMountPoints[k].mFileProvider)
			{
				mMountPoints.erase(mMountPoints.begin() + k);
				--k;
			}
		}
	}

	const std::wstring* FileSystem::applyMountPoint(const MountPoint& mountPoint, const std::wstring& inPath, std::wstring& tempPath) const
	{
		// Check if path starts with the mount point
		if (!mountPoint.mMountPoint.empty() && !startsWith(inPath, mountPoint.mMountPoint))
			return nullptr;

		if (mountPoint.mNeedsPrefixConversion)
		{
			tempPath = mountPoint.mPrefixReplacement + inPath.substr(mountPoint.mMountPoint.length());
			return &tempPath;
		}
		else
		{
			// No change
			return &inPath;
		}
	}

	const std::wstring* FileSystem::removeMountPointPath(const MountPoint& mountPoint, const std::wstring& inPath, std::wstring& tempPath) const
	{
		// Check if path starts with the mount point
		if (!mountPoint.mPrefixReplacement.empty() && !startsWith(inPath, mountPoint.mPrefixReplacement))
			return nullptr;

		if (mountPoint.mNeedsPrefixConversion)
		{
			tempPath = mountPoint.mMountPoint + inPath.substr(mountPoint.mPrefixReplacement.length());
			return &tempPath;
		}
		else
		{
			// No change
			return &inPath;
		}
	}

	void FileSystem::removeMountPointPath(const MountPoint& mountPoint, std::wstring& path) const
	{
		// Check if path starts with the mount point
		if (!mountPoint.mPrefixReplacement.empty() && !startsWith(path, mountPoint.mPrefixReplacement))
			return;

		if (mountPoint.mNeedsPrefixConversion)
		{
			path = mountPoint.mMountPoint + path.substr(mountPoint.mPrefixReplacement.length());
		}
	}

}
