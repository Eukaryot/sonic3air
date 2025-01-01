/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/Game.h"

class EmulatorInterface;


class DiscordIntegration
{
public:
	struct Info
	{
		Game::Mode mGameMode = Game::Mode::UNDEFINED;
		uint32	mSubMode = 0xff;
		uint8	mCharacters = 0xff;
		uint16	mZoneAct = 0xffff;
		uint8	mChaosEmeralds = 0xff;
		uint8	mSuperEmeralds = 0xff;
		uint32	mLevelNumber = 0xffffffff;		// For Blue Sphere game
		uint32	mRecordTime = 0xffffffff;		// In frames

		std::string mModdedDetails;
		std::string mModdedState;
		std::string mModdedLargeImage;
		std::string mModdedSmallImage;

		inline void clear()
		{
			clearGameData();
			clearModdedData();
		}

		inline void clearGameData()
		{
			mGameMode = Game::Mode::UNDEFINED;
			mSubMode = 0xff;
			mCharacters = 0xff;
			mZoneAct = 0xffff;
			mChaosEmeralds = 0xff;
			mSuperEmeralds = 0xff;
			mLevelNumber = 0xffffffff;
			mRecordTime = 0xffffffff;
		}

		inline void clearModdedData()
		{
			mModdedDetails.clear();
			mModdedState.clear();
			mModdedLargeImage.clear();
			mModdedSmallImage.clear();
		}

		inline bool operator==(const Info& other) const
		{
			return (mGameMode == other.mGameMode && mSubMode == other.mSubMode &&
					mCharacters == other.mCharacters && mZoneAct == other.mZoneAct &&
					mChaosEmeralds == other.mChaosEmeralds && mSuperEmeralds == other.mSuperEmeralds &&
					mLevelNumber == other.mLevelNumber && mRecordTime == other.mRecordTime &&
					mModdedDetails == other.mModdedDetails && mModdedState == other.mModdedState &&
					mModdedLargeImage == other.mModdedLargeImage && mModdedSmallImage == other.mModdedSmallImage);
		}
	};

public:
	static void startup();
	static void shutdown();
	static void update();

	static void updateInfo(Game::Mode gameMode, uint32 subMode, EmulatorInterface& emulatorInterface);

	static void setModdedDetails(std::string_view text);
	static void setModdedState(std::string_view text);
	static void setModdedLargeImage(std::string_view imageName);
	static void setModdedSmallImage(std::string_view imageName);
};
