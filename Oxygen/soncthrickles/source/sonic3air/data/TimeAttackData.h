/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class TimeAttackData
{
public:
	struct Entry
	{
		std::wstring mFilename;
		uint32 mTime = 0;
	};
	struct Table
	{
		std::vector<Entry> mEntries;
	};

public:
	static Table& createTable(uint16 zoneAndAct, uint8 category);
	static Table* getTable(uint16 zoneAndAct, uint8 category);

	static Table& loadTable(uint16 zoneAndAct, uint8 category, const std::wstring& filename);
	static void saveTable(uint16 zoneAndAct, uint8 category, const std::wstring& filename);

	static std::wstring getSavePath(uint16 zoneAndAct, uint8 category, std::wstring* outRecBaseFilename = nullptr);

	static std::string getTimeString(uint32 frames, int formatOption = 0);
	static uint32 parseTimeString(const std::string& timeString);

private:
	static std::map<uint32, Table> mTables;
};
