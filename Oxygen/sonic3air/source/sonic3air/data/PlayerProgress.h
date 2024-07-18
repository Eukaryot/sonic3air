/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace detail
{
	struct PlayerAchievementsData
	{
	public:
		std::map<uint32, uint32> mAchievementStates;

	public:
		uint32 getAchievementState(uint32 id) const;

		bool isEqual(const PlayerAchievementsData& other) const;
		void serializeAchievements(VectorBinarySerializer& serializer);
	};

	struct PlayerUnlocksData
	{
	public:
		uint32 mUnlockedSecrets = 0;	// Bitmask, see SharedDatabase::Secret::Type enum for bit indices
		uint32 mFinishedZoneAct = 0;	// Bitmask, one bit per zone / act combination (e.g. bit 7 for CNZ 2)
		uint32 mFinishedZoneActByCharacter[3] = { 0 };

	public:
		bool isSecretUnlocked(uint32 id) const;
		void setSecretUnlocked(uint32 id);

		bool isEqual(const PlayerUnlocksData& other) const;
		void serializeSecrets(VectorBinarySerializer& serializer);
		void serializeFinishedZoneActs(VectorBinarySerializer& serializer);
	};
}


class PlayerProgress : public SingleInstance<PlayerProgress>
{
public:
	// Current progress
	detail::PlayerAchievementsData mAchievements;
	detail::PlayerUnlocksData mUnlocks;

public:
	bool load();
	void save(bool force = false);

private:
	bool loadLegacy(VectorBinarySerializer& serializer);

private:
	// Progress read from file
	detail::PlayerAchievementsData mSavedAchievements;
	detail::PlayerUnlocksData mSavedUnlocks;
};
