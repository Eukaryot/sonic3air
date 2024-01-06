/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/data/PlayerProgress.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/version.inc"

#include "oxygen/application/Configuration.h"


namespace
{
	const uint16 FORMAT_VERSION = 0x0101;		// General player progress file format version
	const uint32 GAME_VERSION   = BUILD_NUMBER;	// Game build, just to have the information
}


namespace detail
{
	PlayerProgressData::PlayerProgressData()
	{
		for (int i = 0; i < 3; ++i)
			mFinishedZoneActByCharacter[i] = 0;
	}

	uint32 PlayerProgressData::getAchievementState(uint32 id) const
	{
		const auto it = mAchievementStates.find(id);
		return (it == mAchievementStates.end()) ? 0 : it->second;
	}

	bool PlayerProgressData::isSecretUnlocked(uint32 id) const
	{
		return (id < 0x20) ? ((mUnlockedSecrets & (1 << id)) != 0) : true;
	}

	void PlayerProgressData::setSecretUnlocked(uint32 id)
	{
		if (id < 0x20)
		{
			mUnlockedSecrets |= (1 << id);
		}
	}

	bool PlayerProgressData::isEqual(const PlayerProgressData& other) const
	{
		if (mFinishedZoneAct != other.mFinishedZoneAct)
			return false;

		for (int i = 0; i < 3; ++i)
		{
			if (mFinishedZoneActByCharacter[i] != other.mFinishedZoneActByCharacter[i])
				return false;
		}

		if (mUnlockedSecrets != other.mUnlockedSecrets)
			return false;

		if (mAchievementStates != other.mAchievementStates)
			return false;

		return true;
	}
}


bool PlayerProgress::load()
{
	std::vector<uint8> buffer;
	if (!FTX::FileSystem->readFile(Configuration::instance().mAppDataPath + L"playerprogress.bin", buffer))
		return false;

	VectorBinarySerializer serializer(true, buffer);
	if (!serialize(serializer))
		return false;

	mWrittenInFile = *this;
	return true;
}

bool PlayerProgress::save(bool force)
{
	// Check for changes
	if (!force && isEqual(mWrittenInFile))
		return true;

	std::vector<uint8> buffer;
	VectorBinarySerializer serializer(false, buffer);
	if (!serialize(serializer))
		return false;

	if (!FTX::FileSystem->saveFile(Configuration::instance().mAppDataPath + L"playerprogress.bin", buffer))
		return false;

	mWrittenInFile = *this;
	return true;
}

bool PlayerProgress::serialize(VectorBinarySerializer& serializer)
{
	// Identifier
	char identifier[9] = "S3AIRPGR";
	serializer.serialize(identifier, 8);
	if (serializer.isReading())
	{
		if (memcmp(identifier, "S3AIRPGR", 8) != 0)
			return false;
	}

	// Format version
	uint16 formatVersion = FORMAT_VERSION;
	serializer & formatVersion;
	if (serializer.isReading())
	{
		if (formatVersion < 0x0100)
			return false;
	}

	// Game version
	uint32 gameVersion = GAME_VERSION;
	serializer & gameVersion;

	// Actual player progress data
	serializer & mFinishedZoneAct;
	for (int i = 0; i < 3; ++i)
		serializer & mFinishedZoneActByCharacter[i];

	// Secrets
	serializer & mUnlockedSecrets;

	// Achievements
	if (formatVersion >= 0x0101)
	{
		if (serializer.isReading())
		{
			mAchievementStates.clear();
			const uint32 count = serializer.read<uint32>();
			for (uint32 i = 0; i < count; ++i)
			{
				const uint32 id = serializer.read<uint32>();
				const uint32 state = serializer.read<uint32>();
				mAchievementStates[id] = state;
			}
		}
		else
		{
			serializer.writeAs<uint32>(mAchievementStates.size());
			for (const auto pair : mAchievementStates)
			{
				serializer.write(pair.first);
				serializer.write(pair.second);
			}
		}
	}

	return true;
}
