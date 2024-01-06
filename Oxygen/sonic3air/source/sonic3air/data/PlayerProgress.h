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
	struct PlayerProgressData
	{
	public:
		uint32 mFinishedZoneAct = 0;	// Bitmask, one bit per zone / act combination (e.g. bit 7 for CNZ 2)
		uint32 mFinishedZoneActByCharacter[3];
		uint32 mUnlockedSecrets = 0;	// Bitmask, see SharedDatabase::Secret::Type enum for bit indices
		std::map<uint32, uint32> mAchievementStates;

	public:
		PlayerProgressData();

		uint32 getAchievementState(uint32 id) const;
		bool isSecretUnlocked(uint32 id) const;
		void setSecretUnlocked(uint32 id);

	protected:
		bool isEqual(const PlayerProgressData& other) const;
	};
}


class PlayerProgress : public detail::PlayerProgressData, public SingleInstance<PlayerProgress>
{
public:
	bool load();
	bool save(bool force = false);

private:
	bool serialize(VectorBinarySerializer& serializer);

private:
	PlayerProgressData mWrittenInFile;
};
