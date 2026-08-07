/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>
#include <optional>


class RawDataCollection : public SingleInstance<RawDataCollection>
{
public:
	struct RawData
	{
		std::vector<uint8> mContent;
		std::optional<uint32> mRomInjectAddress;
		bool mIsModded = false;
	};

public:
	const std::vector<const RawData*>& getRawData(uint64 key) const;

	void clear();
	void loadRawData();
	void applyRomInjections(uint8* rom, uint32 romSize) const;

private:
	void loadRawDataInDirectory(const std::wstring& path, bool isModded);

private:
	std::unordered_map<uint64, std::vector<const RawData*>> mRawDataMap;
	std::vector<const RawData*> mRomInjections;
	ObjectPool<RawData> mRawDataPool;
};
