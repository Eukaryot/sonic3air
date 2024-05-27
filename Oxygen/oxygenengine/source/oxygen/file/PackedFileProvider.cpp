/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
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


class StreamingPackedFileInputStream : public InputStream
{
public:
	inline StreamingPackedFileInputStream(PackedFileProvider& provider, InputStream& baseInputStream, size_t start, size_t size) :
		mProvider(provider), mBaseInputStream(baseInputStream), mStart(start), mSize(size)
	{
		baseInputStream.setPosition(start);
	}

	inline ~StreamingPackedFileInputStream()
	{
		mProvider.unregisterStreamingPackedFileInputStream(*this);
		delete &mBaseInputStream;
	}

	inline bool valid() const override				{ return mIsValid && mBaseInputStream.valid(); }
	inline void close() override					{ mBaseInputStream.close(); }
	inline const char* getType() const override		{ return "streamingPacked"; }

	inline void setPosition(size_t pos) override	{ pos += mStart; if (mIsValid) mBaseInputStream.setPosition(pos); }
	inline size_t getPosition() const				{ return mBaseInputStream.getPosition() - mStart; }
	inline size_t getSize() const override			{ return mIsValid ? mSize : 0; }
	inline size_t getRemaining() const override		{ return mIsValid ? (mSize - getPosition()) : 0; }

	using InputStream::read;
	inline size_t read(void* dst, size_t len) override			{ return mIsValid ? mBaseInputStream.read(dst, std::min(len, mSize - getPosition())) : 0; }
	inline void skip(size_t len) override						{ if (mIsValid) mBaseInputStream.skip(len); }
	inline bool tryRead(const void* data, size_t len) override	{ return mIsValid ? mBaseInputStream.tryRead(data, std::min(len, mSize - getPosition())) : false; }
	inline StreamingState getStreamingState() override			{ return mIsValid ? (getPosition() >= mSize ? StreamingState::COMPLETED : mBaseInputStream.getStreamingState()) : StreamingState::COMPLETED; }

public:
	PackedFileProvider& mProvider;
	InputStream& mBaseInputStream;
	size_t mStart = 0;
	size_t mSize = 0;
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

	PackedFileProvider* provider = new PackedFileProvider(packageFilename, PackedFileProvider::CacheType::NO_CACHING);
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

PackedFileProvider::PackedFileProvider(std::wstring_view packageFilename, CacheType cacheType) :
	mInternal(*new Internal())
{
	mPackageFilename = packageFilename;
	mCacheType = cacheType;

	// Load the package if there is one
	const bool forceLoadAll = (mCacheType == CacheType::CACHE_EVERYTHING);
	mLoaded = FilePackage::loadPackage(packageFilename, mPackedFiles, forceLoadAll);
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
}

void PackedFileProvider::unregisterPackedFileInputStream(PackedFileInputStream& packedFileInputStream)
{
	mPackedFileInputStreams.erase(&packedFileInputStream);
}

void PackedFileProvider::unregisterStreamingPackedFileInputStream(StreamingPackedFileInputStream& packedFileInputStream)
{
	mStreamingPackedFileInputStreams.erase(&packedFileInputStream);
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
		if (packedFile->mLoadedContent)
		{
			// Copy over the already cache content
			outData.resize(packedFile->mContent.size());
			memcpy(&outData[0], &packedFile->mContent[0], packedFile->mContent.size());
		}
		else
		{
			// Load from disk, with or without caching
			if (mCacheType == CacheType::NO_CACHING)
			{
				loadPackedFile(*packedFile, outData);
			}
			else
			{
				loadPackedFile(*packedFile);
				outData.resize(packedFile->mContent.size());
				memcpy(&outData[0], &packedFile->mContent[0], packedFile->mContent.size());
			}
		}
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
		return createPackedFileInputStream(*packedFile);
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
		// Load and cache file content
		if (loadPackedFile(packedFile, packedFile.mContent))
		{
			packedFile.mLoadedContent = true;
		}
	}
}

bool PackedFileProvider::loadPackedFile(PackedFile& packedFile, std::vector<uint8>& outData)
{
	InputStream* inputStream = FTX::FileSystem->createInputStream(mPackageFilename);
	if (nullptr == inputStream)
	{
		RMX_ASSERT(false, "Input stream could not be opened");
		return false;
	}

	outData.resize((size_t)packedFile.mSizeInFile);
	inputStream->setPosition(packedFile.mPositionInFile);
	const size_t bytesRead = inputStream->read(&outData[0], outData.size());
	delete inputStream;

	RMX_CHECK(packedFile.mSizeInFile == bytesRead, "Failed to load entry '" << WString(packedFile.mPath).toStdString() << "' from package '" << WString(mPackageFilename).toStdString() << "'", return false);
	return true;
}

InputStream* PackedFileProvider::createPackedFileInputStream(PackedFile& packedFile)
{
	if (mCacheType == CacheType::NO_CACHING)
	{
		InputStream* baseInputStream = FTX::FileSystem->createInputStream(mPackageFilename);
		if (nullptr == baseInputStream)
		{
			RMX_ASSERT(false, "Input stream could not be opened");
			return false;
		}

		StreamingPackedFileInputStream* inputStream = new StreamingPackedFileInputStream(*this, *baseInputStream, packedFile.mPositionInFile, packedFile.mSizeInFile);
		mStreamingPackedFileInputStreams.insert(inputStream);
		return inputStream;
	}
	else
	{
		loadPackedFile(packedFile);
		PackedFileInputStream* inputStream = new PackedFileInputStream(*this, &packedFile.mContent[0], packedFile.mContent.size());
		mPackedFileInputStreams.insert(inputStream);
		return inputStream;
	}
}

void PackedFileProvider::invalidateAllPackedFileInputStreams()
{
	for (PackedFileInputStream* inputStream : mPackedFileInputStreams)
		inputStream->mIsValid = false;
	for (StreamingPackedFileInputStream* inputStream : mStreamingPackedFileInputStreams)
		inputStream->mIsValid = false;
	mPackedFileInputStreams.clear();
	mStreamingPackedFileInputStreams.clear();
}
