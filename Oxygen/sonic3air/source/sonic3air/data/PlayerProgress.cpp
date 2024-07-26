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
#include "oxygen/simulation/PersistentData.h"


namespace detail
{
	uint32 PlayerAchievementsData::getAchievementState(uint32 id) const
	{
		const uint32* state = mapFind(mAchievementStates, id);
		return (nullptr == state) ? 0 : *state;
	}

	bool PlayerAchievementsData::isEqual(const PlayerAchievementsData& other) const
	{
		return (mAchievementStates == other.mAchievementStates);
	}

	void PlayerAchievementsData::serializeAchievements(VectorBinarySerializer& serializer)
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


	bool PlayerUnlocksData::isSecretUnlocked(uint32 id) const
	{
		return (id < 0x20) ? ((mUnlockedSecrets & (1 << id)) != 0) : true;
	}

	void PlayerUnlocksData::setSecretUnlocked(uint32 id)
	{
		if (id < 0x20)
		{
			mUnlockedSecrets |= (1 << id);
		}
	}

	bool PlayerUnlocksData::isEqual(const PlayerUnlocksData& other) const
	{
		if (mUnlockedSecrets != other.mUnlockedSecrets)
			return false;

		if (mFinishedZoneAct != other.mFinishedZoneAct)
			return false;

		for (int i = 0; i < 3; ++i)
		{
			if (mFinishedZoneActByCharacter[i] != other.mFinishedZoneActByCharacter[i])
				return false;
		}
		return true;
	}

	void PlayerUnlocksData::serializeSecrets(VectorBinarySerializer& serializer)
	{
		serializer & mUnlockedSecrets;
	}

	void PlayerUnlocksData::serializeFinishedZoneActs(VectorBinarySerializer& serializer)
	{
		serializer & mFinishedZoneAct;
		for (int i = 0; i < 3; ++i)
			serializer & mFinishedZoneActByCharacter[i];
	}
}


bool PlayerProgress::load()
{
	// Prefer loading from persistent data
	PersistentData& persistentData = PersistentData::instance();

	const std::vector<uint8>& achievementsData     = persistentData.getData(rmx::constMurmur2_64("s3air_achievements"), rmx::constMurmur2_64("Achievements"));
	const std::vector<uint8>& secretsData          = persistentData.getData(rmx::constMurmur2_64("s3air_unlocks"), rmx::constMurmur2_64("Secrets"));
	const std::vector<uint8>& finishedZoneActsData = persistentData.getData(rmx::constMurmur2_64("s3air_unlocks"), rmx::constMurmur2_64("FinishedZoneActs"));

	if (!achievementsData.empty() || !secretsData.empty() || !finishedZoneActsData.empty())
	{
		if (!achievementsData.empty())
		{
			VectorBinarySerializer serializer(true, achievementsData);
			mAchievements.serializeAchievements(serializer);
		}

		if (!secretsData.empty())
		{
			VectorBinarySerializer serializer(true, secretsData);
			mUnlocks.serializeSecrets(serializer);
		}

		if (!finishedZoneActsData.empty())
		{
			VectorBinarySerializer serializer(true, finishedZoneActsData);
			mUnlocks.serializeFinishedZoneActs(serializer);
		}
	}
	else
	{
		// Try to load from old "playerprogress.bin"
		std::vector<uint8> buffer;
		if (!FTX::FileSystem->readFile(Configuration::instance().mAppDataPath + L"playerprogress.bin", buffer))
			return false;

		VectorBinarySerializer serializer(true, buffer);
		if (!loadLegacy(serializer))
			return false;

		// Save immediately
		save(true);

		// Rename the old file
		FTX::FileSystem->renameFile(Configuration::instance().mAppDataPath + L"playerprogress.bin", Configuration::instance().mAppDataPath + L"playerprogress.bin.backup");
	}

	mSavedAchievements = mAchievements;
	mSavedUnlocks = mUnlocks;
	return true;
}

void PlayerProgress::save(bool force)
{
	PersistentData& persistentData = PersistentData::instance();

	// Achievements
	if (force || mAchievements.isEqual(mSavedAchievements))
	{
		std::vector<uint8> buffer;
		VectorBinarySerializer serializer(false, buffer);
		mAchievements.serializeAchievements(serializer);
		persistentData.setData("s3air_achievements", "Achievements", buffer);

		mSavedAchievements = mAchievements;
	}

	// Unlocks
	if (force || mUnlocks.isEqual(mSavedUnlocks))
	{
		std::vector<uint8> buffer;
		{
			VectorBinarySerializer serializer(false, buffer);
			mUnlocks.serializeSecrets(serializer);
			persistentData.setData("s3air_unlocks", "Secrets", buffer);
		}

		buffer.clear();
		{
			VectorBinarySerializer serializer(false, buffer);
			mAchievements.serializeAchievements(serializer);
			persistentData.setData("s3air_unlocks", "FinishedZoneActs", buffer);
		}

		mSavedUnlocks = mUnlocks;
	}
}

bool PlayerProgress::loadLegacy(VectorBinarySerializer& serializer)
{
	const uint16 FORMAT_VERSION = 0x0101;		// General player progress file format version
	const uint32 GAME_VERSION   = BUILD_NUMBER;	// Game build, just to have the information

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

	// Zone & acts progress
	mUnlocks.serializeFinishedZoneActs(serializer);

	// Secrets
	mUnlocks.serializeSecrets(serializer);

	// Achievements
	if (formatVersion >= 0x0101)
	{
		mAchievements.serializeAchievements(serializer);
	}

	return true;
}
