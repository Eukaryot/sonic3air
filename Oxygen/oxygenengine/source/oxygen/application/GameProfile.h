/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class GameProfile final : public SingleInstance<GameProfile>
{
public:
	typedef std::pair<uint32, uint32> AddressRange;

	struct RomCheck
	{
		uint32 mSize = 0;
		uint64 mChecksum = 0;
	};
	struct RomInfo
	{
		std::string mSteamGameName;
		std::wstring mSteamRomName;
		uint64 mHeaderChecksum = 0;
		std::vector<std::pair<uint32, uint8>> mOverwrites;		// First value: address -- second value: byte value to write there
		std::vector<AddressRange> mBlankRegions;				// Start address and end address are both included
		std::wstring mDiffFileName;
	};

	struct DataPackage
	{
		std::wstring mFilename;
		bool mRequired = true;

		inline DataPackage() {}
		inline DataPackage(const std::wstring& filename, bool required) : mFilename(filename), mRequired(required) {}
	};

	struct LemonStackEntry
	{
		std::string mFunctionName;
		std::string mLabelName;
	};
	struct StackLookupEntry
	{
		std::vector<AddressRange> mAsmStack;		// Start address and end address are both included in the ranges
		std::vector<LemonStackEntry> mLemonStack;
	};

public:
	bool loadOxygenProjectFromFile(const std::wstring& filename);
	bool loadOxygenProjectFromJson(const Json::Value& jsonRoot);

public:
	// Meta data
	std::string mShortName;
	std::string mFullName;

	// ROM
	RomCheck mRomCheck;
	std::vector<RomInfo> mRomInfos;

	// Paths
	std::wstring mGameDataPath;		// As a path relative to the project directory; can stay empty to use the default path

	// Data packages
	std::vector<DataPackage> mDataPackages;

	// Emulation-relevant data
	bool mPushPopAddressOnCall = false;
	std::pair<uint32, uint32> mAsmStackRange;
	std::vector<StackLookupEntry> mStackLookups;
};
