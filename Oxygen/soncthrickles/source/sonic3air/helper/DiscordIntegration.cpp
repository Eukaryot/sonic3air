/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/helper/DiscordIntegration.h"

// Only some platforms support Discord integration
//  - Windows
//  - Linux, except if it's ARM architecture (like when building on a RasPi)
//  - Mac. Sorry about having 3 checks. Command line build has issues finding the discord dylib since it can't be embedded. And temporarily ignore discord for ARM64 until they add support.
#if (defined(PLATFORM_WINDOWS) && !defined(__GNUC__)) || (defined(PLATFORM_LINUX) && !defined(__arm__)) || (defined(PLATFORM_MAC) && !defined(NO_DISCORD) && !defined(__aarch64__))
	#define SUPPORT_DISCORD
#endif

#ifdef SUPPORT_DISCORD

	#include "sonic3air/data/SharedDatabase.h"
	#include "sonic3air/data/TimeAttackData.h"
	#include "oxygen/simulation/EmulatorInterface.h"
	#include "discord_game_sdk/cpp/discord.h"

	namespace discordintegration
	{
		bool active = false;
		discord::Core* core = nullptr;
		discord::Activity activity;

		DiscordIntegration::Info currentInfo;
		DiscordIntegration::Info newInfo;
	}
	using namespace discordintegration;

#endif


void DiscordIntegration::startup()
{
#ifdef SUPPORT_DISCORD
	const discord::Result result = discord::Core::Create(648262663750549506, DiscordCreateFlags_NoRequireDiscord, &core);
	if (result == discord::Result::Ok)
	{
		active = true;
		core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {} );
	}
#endif
}

void DiscordIntegration::shutdown()
{
#ifdef SUPPORT_DISCORD
	if (active)
	{
		SAFE_DELETE(core);
	}
#endif
}

void DiscordIntegration::update()
{
#ifdef SUPPORT_DISCORD
	if (active)
	{
		const discord::Result result = core->RunCallbacks();
		if (result == discord::Result::NotRunning)
		{
			active = false;
		}
	}
#endif
}

void DiscordIntegration::updateInfo(Game::Mode gameMode, uint32 subMode, EmulatorInterface& emulatorInterface)
{
#ifdef SUPPORT_DISCORD

	// Update game data in info struct contents
	newInfo.clearGameData();
	newInfo.mGameMode = gameMode;
	if (gameMode == Game::Mode::NORMAL_GAME || gameMode == Game::Mode::ACT_SELECT || gameMode == Game::Mode::TIME_ATTACK)
	{
		const uint8 innerGameMode = emulatorInterface.readMemory8(0xfffff600) & 0x7f;
		if (innerGameMode < 0x0c || innerGameMode == 0x4c)	// Title Screen or Data Select
		{
			newInfo.mGameMode = Game::Mode::UNDEFINED;
		}
		else if (innerGameMode == 0x1c || innerGameMode == 0x24 || innerGameMode == 0x28)	// S3&K Level Select
		{
			newInfo.mGameMode = Game::Mode::UNDEFINED;
		}
		else
		{
			newInfo.mSubMode = subMode;
			newInfo.mCharacters = (uint8)emulatorInterface.readMemory16(0xffffff0a);
			newInfo.mZoneAct = emulatorInterface.readMemory16(0xffffee4e);
			newInfo.mChaosEmeralds = emulatorInterface.readMemory8(0xffffffb0);
			newInfo.mSuperEmeralds = emulatorInterface.readMemory8(0xffffffb1);

			if (gameMode == Game::Mode::TIME_ATTACK)
			{
				TimeAttackData::Table* timeAttackTable = TimeAttackData::getTable(newInfo.mZoneAct, newInfo.mSubMode);
				if (nullptr != timeAttackTable && !timeAttackTable->mEntries.empty())
				{
					newInfo.mRecordTime = timeAttackTable->mEntries[0].mTime;
				}
			}
		}
	}
	else if (gameMode == Game::Mode::BLUE_SPHERE)
	{
		if (emulatorInterface.readMemory32(0xffffb000) != 0)	// Ignore first frames, we get uninitialized values there
		{
			newInfo.mCharacters = (uint8)emulatorInterface.readMemory16(0xffffff0a);
			newInfo.mLevelNumber = emulatorInterface.readMemory32(0xffffffa6) + 1;
		}
	}
	else if (gameMode == Game::Mode::COMPETITION)
	{
		if (emulatorInterface.readMemory16(0xffffffe8) != 0)	// Inside a stage?
		{
			newInfo.mSubMode = emulatorInterface.readMemory8(0xffffef48);
			newInfo.mZoneAct = emulatorInterface.readMemory16(0xfffffe10);
		}
	}

	// Any change?
	if (!active || newInfo == currentInfo)
		return;

	currentInfo = newInfo;

	bool allowModdedContent = false;
	switch (currentInfo.mGameMode)
	{
		case Game::Mode::NORMAL_GAME:
		case Game::Mode::ACT_SELECT:
		case Game::Mode::COMPETITION:
		case Game::Mode::BLUE_SPHERE:
		case Game::Mode::TITLE_SCREEN:
			allowModdedContent = true;
			break;

		default:
			allowModdedContent = false;
			break;
	}

	std::string details;
	std::string state;
	std::string largeImage = "sonic3air";
	std::string smallImage;

	switch (currentInfo.mGameMode)
	{
		case Game::Mode::NORMAL_GAME:	break;		// Not showing the game mode in this case (to save space)
		case Game::Mode::ACT_SELECT:	break;		// Not showing the game mode in this case (to save space)
		case Game::Mode::TIME_ATTACK:	details = "Time Attack";		break;
		case Game::Mode::COMPETITION:	details = "Competition Mode";	largeImage = "gamemode_competition";	break;
		case Game::Mode::BLUE_SPHERE:	details = "Blue Sphere Game";	largeImage = "gamemode_bluesphere";		break;
		case Game::Mode::TITLE_SCREEN:	details = "Title Screen";		largeImage = "sonic3air";				break;
		default:						details = "In the Menus";		largeImage = "menus";					break;
	}

	switch (currentInfo.mGameMode)
	{
		case Game::Mode::NORMAL_GAME:
		case Game::Mode::ACT_SELECT:
		case Game::Mode::TIME_ATTACK:
		{
			if (currentInfo.mCharacters != 0xff)
			{
				if (currentInfo.mGameMode == Game::Mode::TIME_ATTACK)
				{
					if (currentInfo.mSubMode == 0x11)
					{
						details += " (Sonic - Max Control)";  smallImage = "character_sonic";
					}
					else
					{
						switch (currentInfo.mCharacters)
						{
							case 1:  details += " (Sonic)";		smallImage = "character_sonic";  break;
							case 2:  details += " (Tails)";		smallImage = "character_tails";  break;
							case 3:  details += " (Knuckles)";	smallImage = "character_knuckles";  break;
						}
					}
				}
				else
				{
					if (!details.empty())
						details += ", ";

					switch (currentInfo.mCharacters)
					{
						case 0:  details += "Sonic & Tails";	smallImage = "character_sonic_tails";	  break;
						case 1:  details += "Sonic";			smallImage = "character_sonic";			  break;
						case 2:  details += "Tails";			smallImage = "character_tails";			  break;
						case 3:  details += "Knuckles";			smallImage = "character_knuckles";		  break;
						case 4:  details += "Knuckles & Tails";	smallImage = "character_knuckles_tails";  break;
					}
				}
			}

			if (currentInfo.mZoneAct != 0xffff)
			{
				const SharedDatabase::Zone* zone = SharedDatabase::getZoneByInternalIndex(currentInfo.mZoneAct >> 8);
				if (nullptr != zone)
				{
					if (currentInfo.mGameMode == Game::Mode::TIME_ATTACK)
					{
						String initials = zone->mInitials;
						initials.upperCase();
						state += *initials;
						const bool multipleActs = (zone->mActsNormal >= 2);
						if (multipleActs)
						{
							state = state + " " + ((currentInfo.mZoneAct & 1) ? "2" : "1");
						}
					}
					else
					{
						state += zone->mDisplayName;
						const bool multipleActs = (zone->mActsNormal >= 2);
						if (multipleActs)
						{
							state = state + " Act " + ((currentInfo.mZoneAct & 1) ? "2" : "1");
						}
					}

					largeImage = zone->mShortName;
				}
			}

			if (currentInfo.mChaosEmeralds != 0xff)
			{
				if (currentInfo.mGameMode != Game::Mode::TIME_ATTACK && currentInfo.mChaosEmeralds > 0)
				{
					const bool showSuperEmeralds = (currentInfo.mSuperEmeralds > 0 && currentInfo.mSuperEmeralds != 0xff);
					const int count = showSuperEmeralds ? currentInfo.mSuperEmeralds  : currentInfo.mChaosEmeralds;
					const char* type = showSuperEmeralds ? "Super" : "Chaos";

					if (count == 7)
					{
						details = details + " (All " + type + " Emeralds)";
					}
					else if (count == 1)
					{
						details = details + " (1 " + type + " Emerald)";
					}
					else
					{
						details = details + " (" + (std::to_string(count)) + " " + type + " Emeralds)";
					}
				}
			}

			if (currentInfo.mRecordTime != 0xffffffff)
			{
				state = state + ", Best Time: " + TimeAttackData::getTimeString(currentInfo.mRecordTime);
			}
			break;
		}

		case Game::Mode::COMPETITION:
		{
			switch (newInfo.mSubMode)
			{
				case 0:  details = "Grand Prix";    break;	// Intentionally overwriting "Competition Mode" here
				case 1:  details = "Match Race";    break;
				case 2:  details = "Time Attack";   break;
			}

			switch (newInfo.mZoneAct >> 8)
			{
				case 0x0e:  state += "Azure Lake";    break;
				case 0x0f:  state += "Balloon Park";  break;
				case 0x11:  state += "Chrome Gadget"; break;
				case 0x10:  state += "Desert Palace"; break;
				case 0x12:  state += "Endless Mine";  break;
			}
			break;
		}

		case Game::Mode::BLUE_SPHERE:
		{
			if (newInfo.mLevelNumber != 0xffffffff)
			{
				state += "Level " + std::to_string(currentInfo.mLevelNumber);
			}
			break;
		}

		default:
			break;
	}

	// Overwrite by modding?
	if (allowModdedContent)
	{
		if (!newInfo.mModdedDetails.empty())
		{
			details = newInfo.mModdedDetails;
		}
		if (!newInfo.mModdedState.empty())
		{
			state = newInfo.mModdedState;
		}
		if (!newInfo.mModdedLargeImage.empty())
		{
			largeImage = newInfo.mModdedLargeImage;
		}
		if (!newInfo.mModdedSmallImage.empty())
		{
			smallImage = newInfo.mModdedSmallImage;
		}
	}
	else
	{
		// Reset modded data
		currentInfo.clearModdedData();
		newInfo.clearModdedData();
	}

	activity.SetDetails(details.c_str());
	activity.SetState(state.c_str());
	activity.GetAssets().SetLargeImage(largeImage.c_str());
	activity.GetAssets().SetSmallImage(smallImage.c_str());
	activity.SetType(discord::ActivityType::Playing);

	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {} );
#endif
}

void DiscordIntegration::setModdedDetails(const std::string& text)
{
#ifdef SUPPORT_DISCORD
	newInfo.mModdedDetails = text;
#endif
}

void DiscordIntegration::setModdedState(const std::string& text)
{
#ifdef SUPPORT_DISCORD
	newInfo.mModdedState = text;
#endif
}

void DiscordIntegration::setModdedLargeImage(const std::string& imageName)
{
#ifdef SUPPORT_DISCORD
	newInfo.mModdedLargeImage = imageName;
#endif
}

void DiscordIntegration::setModdedSmallImage(const std::string& imageName)
{
#ifdef SUPPORT_DISCORD
	newInfo.mModdedSmallImage = imageName;
#endif
}
