/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/file/ZipFileProvider.h"
#include "oxygen/file/FileStructureTree.h"
#include "oxygen/helper/Log.h"


// Other platforms than Windows with Visual C++ need to the zlib library dependency into their build separately
#if defined(PLATFORM_WINDOWS) && defined(_MSC_VER)
	#pragma comment(lib, "minizip.lib")
#endif

#ifdef __MINGW64__
	typedef uint64_t ZPOS64_T;
#endif

#include "unzip.h"


namespace detail
{
	voidpf openFile(voidpf opaque, const void* filename, int mode)
	{
		return FTX::FileSystem->createInputStream((const wchar_t*)filename);
	}

	int closeFile(voidpf opaque, voidpf stream)
	{
		delete (InputStream*)stream;
		return 0;
	}

#ifdef __MINGW64__
	unsigned int  readFile(voidpf opaque, voidpf stream, void* buf, unsigned int  size)
#else
	uLong readFile(voidpf opaque, voidpf stream, void* buf, uLong size)
#endif
	{
		InputStream* inputStream = (InputStream*)stream;
		if (nullptr == inputStream)
			return 0;

		return (unsigned int)inputStream->read(buf, (size_t)size);
	}

	ZPOS64_T tellFile(voidpf opaque, voidpf stream)
	{
		InputStream* inputStream = (InputStream*)stream;
		if (nullptr == inputStream)
			return 0;

		return (ZPOS64_T)inputStream->getPosition();
	}

	long seekFile(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin)
	{
		InputStream* inputStream = (InputStream*)stream;
		if (nullptr == inputStream)
			return 0;

		if (origin == SEEK_END)
		{
			offset = (ZPOS64_T)std::max<int64>(0, (int64)inputStream->getSize() - (int64)offset);
		}
		else if (origin == SEEK_CUR)
		{
			offset = std::min<ZPOS64_T>((ZPOS64_T)inputStream->getPosition() + offset, (ZPOS64_T)inputStream->getSize());
		}
		inputStream->setPosition((size_t)offset);
		return 0;
	}

	int testFileError(voidpf opaque, voidpf stream)
	{
		InputStream* inputStream = (InputStream*)stream;
		return (nullptr == inputStream) ? 1 : 0;
	}
}


struct ZipFileProvDetail
{
	static void buildFileEntries(std::vector<rmx::FileIO::FileEntry>& outFileEntries, const std::vector<const FileStructureTree::Entry*>& fileStructureEntries)
	{
		if (!fileStructureEntries.empty())
		{
			outFileEntries.reserve(outFileEntries.size() + fileStructureEntries.size());
			for (size_t k = 0; k < fileStructureEntries.size(); ++k)
			{
				void* customData = fileStructureEntries[k]->mCustomData;
				if (nullptr != customData)
				{
					ZipFileProvider::ContainedFile& containedFile = *(ZipFileProvider::ContainedFile*)customData;
					outFileEntries.emplace_back(containedFile.mFileEntry);
				}
			}
		}
	}
};


struct ZipFileProvider::Internal
{
	unzFile mZipFile;
	unz_global_info64 mGlobalInfo;
	FileStructureTree mFileStructureTree;
	std::vector<const FileStructureTree::Entry*> mEntriesBuffer;
};



ZipFileProvider::ZipFileProvider(const std::wstring& zipFilename) :
	mInternal(*new Internal())
{
	zlib_filefunc64_def filefunc;
	filefunc.zopen64_file = &detail::openFile;
	filefunc.zread_file = &detail::readFile;
	filefunc.ztell64_file = &detail::tellFile;
	filefunc.zseek64_file = &detail::seekFile;
	filefunc.zclose_file = &detail::closeFile;
	filefunc.zerror_file = &detail::testFileError;
	// Ignoring	"filefunc.zwrite_file", it's not needed here

	mInternal.mZipFile = unzOpen2_64(zipFilename.c_str(), &filefunc);
	const int result = unzGetGlobalInfo64(mInternal.mZipFile, &mInternal.mGlobalInfo);
	if (result == UNZ_OK)
	{
		mLoaded = scanZipFile(zipFilename);
	}

	if (mLoaded)
	{
		LOG_INFO("Loaded ZIP file '" << WString(zipFilename).toStdString() << "' with " << (uint32)mContainedFiles.size() << " entries");
	}
	else
	{
		LOG_INFO("Failed to load zip file '" << WString(zipFilename).toStdString() << "'");
	}
}

ZipFileProvider::~ZipFileProvider()
{
	delete &mInternal;
}

bool ZipFileProvider::exists(const std::wstring& filename)
{
	return (nullptr != findContainedFile(filename));
}

bool ZipFileProvider::readFile(const std::wstring& filename, std::vector<uint8>& outData)
{
	const ContainedFile* containedFile = readFile(filename);
	if (nullptr == containedFile)
		return false;

	outData = containedFile->mContent;
	return true;
}

bool ZipFileProvider::listFiles(const std::wstring& path, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries)
{
	if (mContainedFiles.empty())
		return false;

	mInternal.mEntriesBuffer.clear();
	if (!mInternal.mFileStructureTree.listFiles(mInternal.mEntriesBuffer, path))
		return false;

	ZipFileProvDetail::buildFileEntries(outFileEntries, mInternal.mEntriesBuffer);
	return true;
}

bool ZipFileProvider::listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries)
{
	if (mContainedFiles.empty())
		return false;

	mInternal.mEntriesBuffer.clear();
	if (!mInternal.mFileStructureTree.listFilesByMask(mInternal.mEntriesBuffer, filemask, recursive))
		return false;

	ZipFileProvDetail::buildFileEntries(outFileEntries, mInternal.mEntriesBuffer);
	return true;
}

bool ZipFileProvider::listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories)
{
	if (mContainedFiles.empty())
		return false;

	return mInternal.mFileStructureTree.listDirectories(outDirectories, path);
}

InputStream* ZipFileProvider::createInputStream(const std::wstring& filename)
{
	const ContainedFile* containedFile = readFile(filename);
	if (nullptr == containedFile)
		return nullptr;

	return new MemInputStream(&containedFile->mContent[0], containedFile->mContent.size());
}

bool ZipFileProvider::scanZipFile(const std::wstring& zipFilename)
{
	mContainedFiles.clear();
	mInternal.mFileStructureTree.clear();

	// Start with the first entry in the zip file in any case (this is actually unnecessary when called form the constructor)
	int result = unzGoToFirstFile(mInternal.mZipFile);
	if (result != UNZ_OK)
		return false;

	for (ZPOS64_T i = 0; i < mInternal.mGlobalInfo.number_entry; ++i)
	{
		// Get file info
		unz_file_info64 fileInfo;
		char localFilename[256];
		result = unzGetCurrentFileInfo64(mInternal.mZipFile, &fileInfo, localFilename, sizeof(localFilename), nullptr, 0, nullptr, 0);
		if (result != UNZ_OK)
			return false;

		// Get the local path inside the zip file
		const std::wstring localPath = String(localFilename).toStdWString();
		std::wstring localBasePath;
		std::wstring localName;
		{
			std::wstring name;
			std::wstring extension;
			rmx::FileIO::splitPath(localPath, &localBasePath, &name, &extension);
			localName = extension.empty() ? name : (name + L'.' + extension);
		}

		// Create a file entry, unless it's a directory
		if (!localName.empty())
		{
			ContainedFile& containedFile = mContainedFiles[FileStructureTree::getLowercaseStringHash(localPath)];
			containedFile.mFileEntry.mFilename = localName;
			containedFile.mFileEntry.mPath = localBasePath.empty() ? L"" : (localBasePath + L'/');
			containedFile.mFileEntry.mSize = (size_t)fileInfo.uncompressed_size;
			//containedFile.mFileEntry.mTime = ...;	// Meh, forget about the date/time, we don't need it anyways
		}
		else
		{
			// It's a directory, but just in case it's empty, add it to the file structure tree
			mInternal.mFileStructureTree.insertPath(localPath, nullptr);
		}

		// Next please
		if ((i + 1) < mInternal.mGlobalInfo.number_entry)
		{
			result = unzGoToNextFile(mInternal.mZipFile);
			if (result != UNZ_OK)
				return false;
		}
	}

	// Update file structure tree
	for (const auto& pair : mContainedFiles)
	{
		const ContainedFile& containedFile = pair.second;
		mInternal.mFileStructureTree.insertPath(containedFile.mFileEntry.mPath + containedFile.mFileEntry.mFilename, (void*)&containedFile);
	}
	mInternal.mFileStructureTree.sortTreeNodes();
	return true;
}

const ZipFileProvider::ContainedFile* ZipFileProvider::readFile(const std::wstring& filename)
{
	ContainedFile* containedFile = findContainedFile(filename);
	if (nullptr == containedFile)
		return nullptr;

	const rmx::FileIO::FileEntry& fileEntry = containedFile->mFileEntry;
	if (!containedFile->mContent.empty() || fileEntry.mSize == 0)
	{
		// Cached file is loaded already (or empty)
		return containedFile;
	}

#ifdef __MINGW64__
	int result = unzLocateFile(mInternal.mZipFile, *WString(fileEntry.mPath + fileEntry.mFilename).toString(), (unzFileNameComparer)1);
#else
	int result = unzLocateFile(mInternal.mZipFile, *WString(fileEntry.mPath + fileEntry.mFilename).toString(), 1);
#endif
	if (result != UNZ_OK)
		return nullptr;

	containedFile->mContent.resize(fileEntry.mSize);

	result = unzOpenCurrentFile(mInternal.mZipFile);
	if (result != UNZ_OK)
		return nullptr;

	const int readResult = unzReadCurrentFile(mInternal.mZipFile, &containedFile->mContent[0], (int)containedFile->mContent.capacity());
	if (readResult == (int)containedFile->mContent.capacity())
	{
		if (unzCloseCurrentFile(mInternal.mZipFile) == UNZ_OK)
		{
			// Success
			return containedFile;
		}
	}
	else
	{
		unzCloseCurrentFile(mInternal.mZipFile);
	}
	return nullptr;
}

ZipFileProvider::ContainedFile* ZipFileProvider::findContainedFile(const std::wstring& filePath)
{
	const auto it = mContainedFiles.find(FileStructureTree::getLowercaseStringHash(filePath));
	return (it == mContainedFiles.end()) ? nullptr : &it->second;
}

const ZipFileProvider::ContainedFile* ZipFileProvider::findContainedFile(const std::wstring& filePath) const
{
	const auto it = mContainedFiles.find(FileStructureTree::getLowercaseStringHash(filePath));
	return (it == mContainedFiles.end()) ? nullptr : &it->second;
}
