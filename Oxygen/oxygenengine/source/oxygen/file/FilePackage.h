/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class FilePackage
{
public:
	struct PackedFile
	{
		std::wstring mPath;
		uint32 mPositionInFile = 0;
		uint32 mSizeInFile = 0;
		bool mLoadedContent = false;
		std::vector<uint8> mContent;
	};

	struct PackageHeader
	{
		static const constexpr char SIGNATURE[] = "OPCK";
		static const constexpr uint32 CURRENT_FORMAT_VERSION = 3;
		static const constexpr size_t HEADER_SIZE = 20;

		uint32 mFormatVersion = CURRENT_FORMAT_VERSION;
		uint32 mContentVersion = 0;
		uint32 mEntryHeaderSize = 0;		// Including the meta data for entries, but excluding their contents
		size_t mNumEntries = 0;
	};

public:
	static bool loadPackage(const std::wstring& packageFilename, std::map<std::wstring, PackedFile>& outPackedFiles, InputStream*& inputStream, bool forceLoadAll, bool showErrors = true);
	static void createFilePackage(const std::wstring& packageFilename, const std::vector<std::wstring>& includedPaths, const std::vector<std::wstring>& excludedPaths, const std::wstring& comparisonPath, uint32 contentVersion, bool forceReplace = false);

private:
	static bool readPackageHeader(PackageHeader& outHeader, VectorBinarySerializer& serializer);
};
