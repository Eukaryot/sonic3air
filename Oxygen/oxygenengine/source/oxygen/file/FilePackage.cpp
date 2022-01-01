/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/file/FilePackage.h"
#include "oxygen/helper/Utils.h"


bool FilePackage::loadPackage(const std::wstring& packageFilename, std::map<std::wstring, PackedFile>& outPackedFiles, InputStream*& inputStream, bool forceLoadAll)
{
	// Try to load the package
	RMX_ASSERT(nullptr == inputStream, "Input stream was already opened");
	inputStream = FTX::FileSystem->createInputStream(packageFilename);
	if (nullptr == inputStream)
		return false;

	std::vector<uint8> content;
	content.resize(PackageHeader::HEADER_SIZE);
	if (inputStream->read(&content[0], PackageHeader::HEADER_SIZE) != PackageHeader::HEADER_SIZE)
		return false;

	VectorBinarySerializer serializer(true, content);
	PackageHeader header;
	if (!readPackageHeader(header, serializer))
	{
		RMX_CHECK(header.mFormatVersion == PackageHeader::CURRENT_FORMAT_VERSION, "Unsupported format version " << header.mFormatVersion << " of file '" << WString(packageFilename).toStdString() << "'", return false);
		RMX_ERROR("Invalid signature of file '" << WString(packageFilename).toStdString() << "'", );
		return false;
	}

	// Load table of contents
	content.resize(PackageHeader::HEADER_SIZE + header.mEntryHeaderSize);
	if (inputStream->read(&content[PackageHeader::HEADER_SIZE], header.mEntryHeaderSize) != header.mEntryHeaderSize)
		return false;

	// Read entry headers
	for (size_t i = 0; i < header.mNumEntries; ++i)
	{
		const std::wstring key = serializer.read<std::wstring>();
		PackedFile& packedFile = outPackedFiles[key];
		packedFile.mPath = key;
		packedFile.mPositionInFile = serializer.read<uint32>();
		packedFile.mSizeInFile = serializer.read<uint32>();
	}

	if (forceLoadAll)
	{
		// Read entry contents
		for (auto& pair : outPackedFiles)
		{
			pair.second.mContent.resize((size_t)pair.second.mSizeInFile);
			inputStream->setPosition(pair.second.mPositionInFile);
			const size_t bytesRead = inputStream->read(&pair.second.mContent[0], (size_t)pair.second.mSizeInFile);
			RMX_CHECK(pair.second.mSizeInFile == bytesRead, "Failed to load entry '" << WString(pair.first).toStdString() << "' from package", continue);
			pair.second.mLoadedContent = true;
		}
	}
	return true;
}

void FilePackage::createFilePackage(const std::wstring& packageFilename, const std::vector<std::wstring>& includedPaths, const std::vector<std::wstring>& excludedPaths, const std::wstring& comparisonPath, uint32 contentVersion, bool forceReplace)
{
	// Collect file contents
	std::map<std::wstring, PackedFile> packedFiles;
	{
		FileCrawler fc;
		for (const std::wstring& includedPath : includedPaths)
		{
			fc.addFiles(includedPath, true);
			// TODO: Remove duplicates
		}

		for (size_t i = 0; i < fc.size(); ++i)
		{
			const auto& entry = *fc[i];
			std::wstring path = entry.mPath + entry.mFilename;

			bool add = true;
			for (const std::wstring& excludedPath : excludedPaths)
			{
				if (utils::startsWith(path, excludedPath))
				{
					add = false;
					break;
				}
			}

			if (add)
			{
				std::vector<uint8> content;
				if (FTX::FileSystem->readFile(path, content))
				{
					PackedFile& packedFile = packedFiles[path];
					packedFile.mContent.swap(content);
				}
			}
		}
	}

	// Check against existing file, if there is one already
	if (!forceReplace && !comparisonPath.empty())
	{
		std::map<std::wstring, PackedFile> existingPackedFiles;
		InputStream* inputStream = nullptr;
		if (loadPackage(comparisonPath + packageFilename, existingPackedFiles, inputStream, true))
		{
			delete inputStream;

			// Compare
			bool isEqual = (existingPackedFiles.size() == packedFiles.size());
			if (isEqual)
			{
				for (const auto& pair : packedFiles)
				{
					PackedFile* packedFile = mapFind(existingPackedFiles, pair.first);
					if (nullptr == packedFile || packedFile->mContent != pair.second.mContent)
					{
						isEqual = false;
						break;
					}
				}
			}

			if (isEqual)
			{
				// Do not overwrite the existing file, as content has not changed (even though the content version might have changed)
				return;
			}
		}
	}

	// Collect output content
	std::vector<uint8> output;
	size_t entryHeaderSize = 0;
	{
		VectorBinarySerializer serializer(false, output);

		serializer.write(PackageHeader::SIGNATURE, 4);
		const uint32 formatVersion = PackageHeader::CURRENT_FORMAT_VERSION;
		serializer.write(formatVersion);
		serializer.write(contentVersion);

		const size_t headerSizePosition = output.size();
		serializer.writeAs<uint32>(0);		// Will get overwritten

		serializer.writeAs<uint32>(packedFiles.size());
		for (auto& pair : packedFiles)
		{
			serializer.write(pair.first);
			pair.second.mPositionInFile = (uint32)output.size();	// Temporarily misusing this variable to store the position where to write the content's position in file when it got determined
			serializer.writeAs<uint32>(0);							// Will get overwritten
			serializer.writeAs<uint32>(pair.second.mContent.size());
		}

		// Write entry header size
		entryHeaderSize = output.size() - PackageHeader::HEADER_SIZE;
		*(uint32*)&output[headerSizePosition] = (uint32)entryHeaderSize;

		for (auto& pair : packedFiles)
		{
			const uint32 position = (uint32)output.size();
			serializer.write(&pair.second.mContent[0], pair.second.mContent.size());
			*(uint32*)&output[pair.second.mPositionInFile] = position;
			pair.second.mPositionInFile = position;
		}
	}

	// Save output file
	FTX::FileSystem->saveFile(packageFilename, output);
}

bool FilePackage::readPackageHeader(PackageHeader& outHeader, VectorBinarySerializer& serializer)
{
	// Read header
	char signature[4];
	serializer.read(signature, 4);
	if (memcmp(signature, PackageHeader::SIGNATURE, 4) != 0)
		return false;

	outHeader.mFormatVersion = serializer.read<uint32>();
	if (outHeader.mFormatVersion != PackageHeader::CURRENT_FORMAT_VERSION)
		return false;

	outHeader.mContentVersion = serializer.read<uint32>();
	outHeader.mEntryHeaderSize = serializer.read<uint32>();
	outHeader.mNumEntries = (size_t)serializer.read<uint32>();

	RMX_ASSERT(serializer.getReadPosition() == PackageHeader::HEADER_SIZE, "Got wrong package header size");
	return true;
}
