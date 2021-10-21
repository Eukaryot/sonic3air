/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/data/TimeAttackData.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/helper/JsonHelper.h"


std::map<uint32, TimeAttackData::Table> TimeAttackData::mTables;


namespace
{
	const std::map<uint8, std::string> REC_PATH_CHAR_NAMES =
	{
		{ 0x10, "sonic" },
		{ 0x11, "sonic_max" },
		{ 0x20, "tails" },
		{ 0x30, "knuckles" }
	};

	uint32 makeKey(uint16 zoneAndAct, uint8 category)
	{
		return ((uint32)category << 16) + zoneAndAct;
	}

	void getSplittedTime(uint32 frames, uint32& min, uint32& sec, uint32& hsec)
	{
		min = frames / 3600;
		sec = (frames / 60) % 60;
		hsec = ((frames % 60) * 99 + 30) / 59;		// Same calculation as used by script for HUD display
	}
}


TimeAttackData::Table& TimeAttackData::createTable(uint16 zoneAndAct, uint8 category)
{
	return mTables[makeKey(zoneAndAct, category)];
}

TimeAttackData::Table* TimeAttackData::getTable(uint16 zoneAndAct, uint8 category)
{
	const auto it = mTables.find(makeKey(zoneAndAct, category));
	return (it == mTables.end()) ? nullptr : &it->second;
}

TimeAttackData::Table& TimeAttackData::loadTable(uint16 zoneAndAct, uint8 category, const std::wstring& filename)
{
	Table* timeAttackTable = getTable(zoneAndAct, category);
	if (nullptr == timeAttackTable)
	{
		timeAttackTable = &createTable(zoneAndAct, category);

		// Read JSON
		Json::Value root = JsonHelper::loadFile(filename);
		Json::Value& records = root["Records"];

		for (const Json::Value& rec : records)
		{
			const Json::Value file = rec["File"];
			const Json::Value time = rec["Time"];
			if (file.isString())
			{
				Entry& entry = vectorAdd(timeAttackTable->mEntries);
				entry.mFilename = *String(file.asString()).toWString();
				entry.mTime = parseTimeString(time.asString());
			}
		}
	}
	return *timeAttackTable;
}

void TimeAttackData::saveTable(uint16 zoneAndAct, uint8 category, const std::wstring& filename)
{
	Table* table = getTable(zoneAndAct, category);
	if (nullptr == table)
		return;

	Json::Value root;
	Json::Value& records = root["Records"];

	for (const Entry& entry : table->mEntries)
	{
		Json::Value& rec = records.append(Json::Value());
		rec["File"] = *WString(entry.mFilename).toString();
		rec["Time"] = TimeAttackData::getTimeString(entry.mTime);
	}

	JsonHelper::saveFile(filename, root);
}

std::wstring TimeAttackData::getSavePath(uint16 zoneAndAct, uint8 category, std::wstring* outRecBaseFilename)
{
	const SharedDatabase::Zone* currentZone = SharedDatabase::getZoneByInternalIndex(zoneAndAct >> 8);
	if (nullptr == currentZone)
		return L"";

	const std::string zoneAndActName = currentZone->mShortName + std::to_string((zoneAndAct & 0x01) + 1);
	const std::string characterName = REC_PATH_CHAR_NAMES.at(category);

	if (nullptr != outRecBaseFilename)
	{
		*outRecBaseFilename = *String(zoneAndActName + "_" + characterName).toWString();
	}
	return Configuration::instance().mAppDataPath + *String("timeattack/" + zoneAndActName + "_" + characterName).toWString();
}

std::string TimeAttackData::getTimeString(uint32 frames, int formatOption)
{
	uint32 min, sec, hsec;
	getSplittedTime(frames, min, sec, hsec);
	String str;
	if (formatOption == 0)
		str.format("%02d:%02d.%02d", min, sec, hsec);
	else
		str.format("%d' %02d\" %02d", min, sec, hsec);
	return *str;
}

uint32 TimeAttackData::parseTimeString(const std::string& timeString)
{
	const uint32 min = (timeString[0] - '0') * 10 + (timeString[1] - '0');
	const uint32 sec = (timeString[3] - '0') * 10 + (timeString[4] - '0');
	const uint32 hsec = (((timeString[6] - '0') * 10 + (timeString[7] - '0')) * 59 + 50) / 99;
	return min * 3600 + sec * 60 + hsec;
}
