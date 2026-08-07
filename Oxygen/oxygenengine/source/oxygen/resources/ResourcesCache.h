/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/GameProfile.h"


class ResourcesCache : public SingleInstance<ResourcesCache>
{
public:
	bool loadRom();
	bool loadRomFromFile(const std::wstring& filename);
	bool loadRomFromMemory(const std::vector<uint8>& content);

	void loadAllResources();

	inline const std::vector<uint8>& getUnmodifiedRom() const  { return mRom; }

private:
	bool loadRomFile(const std::wstring& filename);
	bool loadRomFile(const std::wstring& filename, const GameProfile::RomInfo& romInfo);
	bool loadRomMemory(const std::vector<uint8>& content);
	uint64 getHeaderChecksum(const std::vector<uint8>& content);
	bool applyRomModifications(const GameProfile::RomInfo& romInfo);
	bool checkRomContent();
	void saveRomToAppData();

private:
	std::vector<uint8> mRom;	// This is the original, unmodified ROM (i.e. without any raw data injections or ROM writes)
	const GameProfile::RomInfo* mLoadedRomInfo = nullptr;
	std::map<const GameProfile::RomInfo*, std::vector<uint8>> mDiffFileCache;
};
