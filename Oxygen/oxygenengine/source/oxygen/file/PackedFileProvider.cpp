/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/file/PackedFileProvider.h"
#include "oxygen/file/FileStructureTree.h"
#include "oxygen/helper/Logging.h"


struct PackedFileProvDetail
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
					FilePackage::PackedFile& packedFile = *(FilePackage::PackedFile*)customData;
					rmx::FileIO::FileEntry& fileEntry = vectorAdd(outFileEntries);
					const size_t slashPosition = packedFile.mPath.find_last_of(L"/\\");
					fileEntry.mFilename = (slashPosition == std::wstring::npos) ? packedFile.mPath : packedFile.mPath.substr(slashPosition + 1);
					fileEntry.mPath = (slashPosition == std::wstring::npos) ? L"" : packedFile.mPath.substr(0, slashPosition + 1);
					fileEntry.mSize = packedFile.mContent.size();
				}
			}
		}
	}
};


// At the moment, this class is not really necessary, as the provider never invalidates its created input streams.
// But maybe it will get more useful when implementing actual file streaming.
class PackedFileInputStream : public MemInputStream
{
public:
	inline PackedFileInputStream(PackedFileProvider& provider, const void* data, size_t size) : MemInputStream(data, size), mProvider(provider) {}
	inline ~PackedFileInputStream() { mProvider.unregisterPackedFileInputStream(*this); }

	inline bool valid() const override				{ return mIsValid && MemInputStream::valid(); }
	inline const char* getType() const override		{ return "packed"; }

	inline void setPosition(size_t pos) override	{ if (mIsValid) MemInputStream::setPosition(pos); }
	inline size_t getSize() const override			{ return mIsValid ? MemInputStream::getSize() : getPosition(); }
	inline size_t getRemaining() const override		{ return mIsValid ? MemInputStream::getRemaining() : 0; }

	using InputStream::read;
	inline size_t read(void* dst, size_t len) override			{ return mIsValid ? MemInputStream::read(dst, len) : 0; }
	inline void skip(size_t len) override						{ if (mIsValid) MemInputStream::skip(len); }
	inline bool tryRead(const void* data, size_t len) override	{ return mIsValid ? MemInputStream::tryRead(data, len) : false; }
	inline StreamingState getStreamingState() override			{ return mIsValid ? MemInputStream::getStreamingState() : StreamingState::COMPLETED; }

public:
	PackedFileProvider& mProvider;
	bool mIsValid = true;
};



struct PackedFileProvider::Internal
{
	FileStructureTree mFileStructureTree;
	std::vector<const FileStructureTree::Entry*> mEntriesBuffer;
};



PackedFileProvider* PackedFileProvider::createPackedFileProvider(std::wstring_view packageFilename)
{
	if (!FTX::FileSystem->exists(packageFilename))
		return nullptr;

	PackedFileProvider* provider = new PackedFileProvider(packageFilename);
	if (provider->isLoaded())
	{
		return provider;
	}
	else
	{
		// Oops, could not load package file
		delete provider;
		return nullptr;
	}
}

PackedFileProvider::PackedFileProvider(std::wstring_view packageFilename) :
	mInternal(*new Internal())
{
	// Load the package if there is one
	mLoaded = FilePackage::loadPackage(packageFilename, mPackedFiles, mInputStream, true);	// TODO: Use streaming instead of loading all content right away
	if (mLoaded)
	{
		RMX_LOG_INFO("Loaded file package '" << WString(packageFilename).toStdString() << "' with " << mPackedFiles.size() << " entries");

		// Setup file structure tree
		for (const auto& pair : mPackedFiles)
		{
			const PackedFile& packedFile = pair.second;
			mInternal.mFileStructureTree.insertPath(packedFile.mPath, (void*)&packedFile);
		}
		mInternal.mFileStructureTree.sortTreeNodes();
	}
	else
	{
		RMX_LOG_INFO("Failed to load file package '" << WString(packageFilename).toStdString() << "'");
	}
}

PackedFileProvider::~PackedFileProvider()
{
	delete &mInternal;
	delete mInputStream;
}

void PackedFileProvider::unregisterPackedFileInputStream(PackedFileInputStream& packedFileInputStream)
{
	mPackedFileInputStreams.erase(&packedFileInputStream);
}

bool PackedFileProvider::exists(const std::wstring& path)
{
	if (nullptr != findPackedFile(path))
		return true;

	// Fallback needed specifically if the path refers to a directory
	return mInternal.mFileStructureTree.pathExists(path);
}

bool PackedFileProvider::readFile(const std::wstring& filename, std::vector<uint8>& outData)
{
	PackedFile* packedFile = findPackedFile(filename);
	if (nullptr != packedFile)
	{
		loadPackedFile(*packedFile);
		outData.resize(packedFile->mContent.size());
		memcpy(&outData[0], &packedFile->mContent[0], packedFile->mContent.size());
		return true;
	}
	return false;
}

bool PackedFileProvider::listFiles(const std::wstring& path, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries)
{
	if (mPackedFiles.empty())
		return false;

	mInternal.mEntriesBuffer.clear();
	if (!mInternal.mFileStructureTree.listFiles(mInternal.mEntriesBuffer, path))
		return false;

	PackedFileProvDetail::buildFileEntries(outFileEntries, mInternal.mEntriesBuffer);
	return true;
}

bool PackedFileProvider::listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<rmx::FileIO::FileEntry>& outFileEntries)
{
	if (mPackedFiles.empty())
		return false;

	mInternal.mEntriesBuffer.clear();
	if (!mInternal.mFileStructureTree.listFilesByMask(mInternal.mEntriesBuffer, filemask, recursive))
		return false;

	PackedFileProvDetail::buildFileEntries(outFileEntries, mInternal.mEntriesBuffer);
	return true;
}

bool PackedFileProvider::listDirectories(const std::wstring& path, std::vector<std::wstring>& outDirectories)
{
	if (mPackedFiles.empty())
		return false;

	return mInternal.mFileStructureTree.listDirectories(outDirectories, path);
}

InputStream* PackedFileProvider::createInputStream(const std::wstring& filename)
{
	// Try to first read from package
	PackedFile* packedFile = findPackedFile(filename);
	if (nullptr != packedFile)
	{
		loadPackedFile(*packedFile);
		PackedFileInputStream* inputStream = new PackedFileInputStream(*this, &packedFile->mContent[0], packedFile->mContent.size());
		mPackedFileInputStreams.insert(inputStream);
		return inputStream;
	}

	return nullptr;
}

PackedFileProvider::PackedFile* PackedFileProvider::findPackedFile(const std::wstring& filename)
{
	if (!mPackedFiles.empty())
	{
		const auto it = mPackedFiles.find(filename);
		if (it != mPackedFiles.end())
		{
			return &it->second;
		}
	}
	return nullptr;
}

void PackedFileProvider::loadPackedFile(PackedFile& packedFile)
{
	if (!packedFile.mLoadedContent)
	{
		RMX_ASSERT(nullptr != mInputStream, "Input stream is not opened");
		packedFile.mContent.resize((size_t)packedFile.mSizeInFile);
		mInputStream->setPosition(packedFile.mPositionInFile);
		const size_t bytesRead = mInputStream->read(&packedFile.mContent[0], (size_t)packedFile.mSizeInFile);
		RMX_CHECK(packedFile.mSizeInFile == bytesRead, "Failed to load entry '" << WString(packedFile.mPath).toStdString() << "' from package", return);
		packedFile.mLoadedContent = true;
	}
}

void PackedFileProvider::invalidateAllPackedFileInputStreams()
{
	for (PackedFileInputStream* packedFileInputStream : mPackedFileInputStreams)
	{
		packedFileInputStream->mIsValid = false;
	}
	mPackedFileInputStreams.clear();
}
