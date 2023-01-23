/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/version.inc"

#include "oxygen/application/GameProfile.h"
#include "oxygen/helper/JsonHelper.h"


namespace
{
	int getReleaseChannelValueForBuild()
	{
		const constexpr uint32 buildVariantHash = rmx::compileTimeFNV_32(BUILD_VARIANT);
		if (buildVariantHash == rmx::compileTimeFNV_32("TEST"))
			return 2;
		else if (buildVariantHash == rmx::compileTimeFNV_32("PREVIEW") || buildVariantHash == rmx::compileTimeFNV_32("BETA"))	// Treat betas as previews
			return 1;
		else
			return 0;
	}
}


void ConfigurationImpl::fillDefaultGameProfile(GameProfile& gameProfile)
{
	gameProfile.mShortName = "Sonic 3 A.I.R.";
	gameProfile.mFullName = "Sonic 3 - Angel Island Revisited";

	gameProfile.mRomCheck.mSize = 0x400000;
	gameProfile.mRomCheck.mChecksum = 0x344983ffcfeff8cb;

	gameProfile.mRomInfos.resize(1);
	gameProfile.mRomInfos[0].mSteamGameName = "Sonic 3 & Knuckles";
	gameProfile.mRomInfos[0].mSteamRomName = L"Sonic_Knuckles_wSonic3.bin";
	gameProfile.mRomInfos[0].mOverwrites.clear();
	gameProfile.mRomInfos[0].mOverwrites.emplace_back(0x2001f0, 0x4a);
}

ConfigurationImpl::ConfigurationImpl()
{
	// Setup defaults
	mGameServer.mUpdateCheck.mReleaseChannel = getReleaseChannelValueForBuild();
}

void ConfigurationImpl::preLoadInitialization()
{
	SharedDatabase::initialize();

#ifndef ENDUSER
	mCompiledScriptSavePath = L"saves/scripts.bin";
#endif
}

bool ConfigurationImpl::loadConfigurationInternal(JsonHelper& jsonHelper)
{
	loadSharedSettingsConfig(jsonHelper);

	// Add special preprocessor define
	//  -> Used to query whether it's the game build (i.e. not the Oxygen App), and to get its build number
	mPreprocessorDefinitions.setDefinition("GAMEAPP", BUILD_NUMBER);

	// Setup the default game profile data accordingly
	fillDefaultGameProfile(GameProfile::instance());

#ifndef ENDUSER
	// Explicitly set a (non-empty) project path, so that "oxygenproject.json" gets loaded
	mProjectPath = L"./";
#endif

	return true;
}

bool ConfigurationImpl::loadSettingsInternal(JsonHelper& rootHelper, SettingsType settingsType)
{
	if (settingsType == SettingsType::STANDARD)
	{
		rootHelper.tryReadString("GameVersion", mGameVersionInSettings);
		loadSharedSettingsConfig(rootHelper);
	}

	// Audio
	rootHelper.tryReadFloat("Audio_MusicVolume", mMusicVolume);
	rootHelper.tryReadFloat("Audio_SoundVolume", mSoundVolume);
	rootHelper.tryReadInt("ActiveSoundtrack", mActiveSoundtrack);

	// Game simulation
	if (rootHelper.tryReadInt("SimulationFrequency", mSimulationFrequency))
	{
		mSimulationFrequency = clamp(mSimulationFrequency, 30, 240);
	}

	// Time Attack
	rootHelper.tryReadInt("InstantTimeAttackRestart", mInstantTimeAttackRestart);

	// Game settings
	if (!mGameVersionInSettings.empty())
	{
		const auto& settingsMap = SharedDatabase::getSettings();

		JsonHelper gameSettingsHelper(rootHelper.mJson["GameSettings"]);

		// Load settings as uint32 values
		for (auto& pair : settingsMap)
		{
			if (pair.second.mSerializationType != SharedDatabase::Setting::SerializationType::NONE)
			{
				int value = 0;
				if (gameSettingsHelper.tryReadInt(pair.second.mIdentifier, value))
				{
					pair.second.mCurrentValue = (uint32)value;
				}
			}
		}

		if (mGameVersionInSettings < "20.05.01.0")
		{
			// Enforce auto-detect once if user had an older version before
			mAutoDetectRenderMethod = true;
		}

		if (mGameVersionInSettings < "22.08.27.0")
		{
			// Reset script optimization - its default was 3 before introducing -1 for auto
			mScriptOptimizationLevel = -1;
		}

		if (mGameVersionInSettings < "22.12.17.0")
		{
			// Reset the SETTING_FIX_GLITCHES, after the default value changed
			settingsMap.at(SharedDatabase::Setting::SETTING_FIX_GLITCHES).mCurrentValue = 2;
		}

		// Make corrections where needed
		if (!settingsMap.empty())	// This is going to be empty when the macOS UI calls loadConfiguration externally, causing crash
		{
			const SharedDatabase::Setting& ghosts = settingsMap.at(SharedDatabase::Setting::SETTING_TIME_ATTACK_GHOSTS);
			ghosts.mCurrentValue = (ghosts.mCurrentValue >= 5) ? 5 : (ghosts.mCurrentValue >= 3) ? 3 : (ghosts.mCurrentValue >= 1) ? 1 : 0;
		}
	}
	return true;
}

void ConfigurationImpl::saveSettingsInternal(Json::Value& root, SettingsType settingsType)
{
	if (settingsType != SettingsType::STANDARD)
		return;

	// Format info & metadata
	root["GameVersion"] = BUILD_STRING;
	root["GameExePath"] = WString(mExePath).toStdString();

	// Audio
	root["Audio_MusicVolume"] = mMusicVolume;
	root["Audio_SoundVolume"] = mSoundVolume;
	root["ActiveSoundtrack"] = mActiveSoundtrack;

	// Game simulation
	if (mSimulationFrequency != 60)
		root["SimulationFrequency"] = mSimulationFrequency;
	else
		root.removeMember("SimulationFrequency");

	// Time Attack
	root["InstantTimeAttackRestart"] = mInstantTimeAttackRestart;

	// Game settings
	{
		Json::Value gameSettingsJson;
		const auto& settingsMap = SharedDatabase::getSettings();
		for (auto& pair : settingsMap)
		{
			const SharedDatabase::Setting& setting = pair.second;
			if (setting.mSerializationType == SharedDatabase::Setting::SerializationType::NONE)
				continue;
			if (setting.mSerializationType == SharedDatabase::Setting::SerializationType::HIDDEN && setting.mCurrentValue == setting.mDefaultValue)
				continue;
			gameSettingsJson[setting.mIdentifier] = setting.mCurrentValue;
		}
		root["GameSettings"] = gameSettingsJson;
	}

	// Game server
	{
		Json::Value gameServerJson;
		{
			gameServerJson["ServerAddress"] = mGameServer.mServerHostName;
			gameServerJson["ServerPortUDP"] = mGameServer.mServerPortUDP;
			gameServerJson["ServerPortTCP"] = mGameServer.mServerPortTCP;
			gameServerJson["ServerPortWSS"] = mGameServer.mServerPortWSS;

			Json::Value ghostSyncJson;
			ghostSyncJson["Enabled"] = mGameServer.mGhostSync.mEnabled ? 1 : 0;
			ghostSyncJson["ChannelName"] = mGameServer.mGhostSync.mChannelName;
			ghostSyncJson["ShowOffscreenGhosts"] = mGameServer.mGhostSync.mShowOffscreenGhosts ? 1 : 0;
			gameServerJson["GhostSync"] = ghostSyncJson;

			Json::Value updateCheckJson;
			updateCheckJson["ReleaseChannel"] = mGameServer.mUpdateCheck.mReleaseChannel;
			gameServerJson["UpdateCheck"] = updateCheckJson;
		}
		root["GameServer"] = gameServerJson;
	}
}

void ConfigurationImpl::loadSharedSettingsConfig(JsonHelper& rootHelper)
{
	// Dev mode
	Json::Value devModeJson = rootHelper.mJson["DevMode"];
	if (devModeJson.isObject())
	{
		JsonHelper devModeHelper(devModeJson);
		devModeHelper.tryReadBool("EnforceDebugMode", mDevModeImpl.mEnforceDebugMode);
		devModeHelper.tryReadBool("SkipExitConfirmation", mDevModeImpl.SkipExitConfirmation);
	}

	// Game server
	{
		const Json::Value& gameServerJson = rootHelper.mJson["GameServer"];
		if (!gameServerJson.isNull())
		{
			// General game server settings
			JsonHelper gameServerHelper(gameServerJson);
			gameServerHelper.tryReadString("ServerAddress", mGameServer.mServerHostName);
			gameServerHelper.tryReadInt("ServerPortUDP", mGameServer.mServerPortUDP);
			gameServerHelper.tryReadInt("ServerPortTCP", mGameServer.mServerPortTCP);
			gameServerHelper.tryReadInt("ServerPortWSS", mGameServer.mServerPortWSS);

			// Update Check settings
			const Json::Value& updateCheckJson = gameServerHelper.mJson["UpdateCheck"];
			if (!updateCheckJson.isNull())
			{
				JsonHelper jsonHelper(updateCheckJson);
				jsonHelper.tryReadInt("UpdateChannel", mGameServer.mUpdateCheck.mReleaseChannel);
			}

			// Ghost Sync settings
			const Json::Value& ghostSyncJson = gameServerHelper.mJson["GhostSync"];
			if (!ghostSyncJson.isNull())
			{
				JsonHelper jsonHelper(ghostSyncJson);
				jsonHelper.tryReadBool("Enabled", mGameServer.mGhostSync.mEnabled);
				jsonHelper.tryReadString("ChannelName", mGameServer.mGhostSync.mChannelName);
				jsonHelper.tryReadBool("ShowOffscreenGhosts", mGameServer.mGhostSync.mShowOffscreenGhosts);
			}
		}
	}
}
