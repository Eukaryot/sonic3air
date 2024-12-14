/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/version.inc"

#include "oxygen/application/GameProfile.h"
#include "oxygen/helper/JsonSerializer.h"


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
	mGameServerBase.mServerHostName = "sonic3air.org";
	mGameServerImpl.mUpdateCheck.mReleaseChannel = getReleaseChannelValueForBuild();
}

void ConfigurationImpl::preLoadInitialization()
{
	SharedDatabase::initialize();

	mCompiledScriptSavePath = L"saves/scripts.bin";
}

bool ConfigurationImpl::loadConfigurationInternal(JsonSerializer& serializer)
{
	serializeSharedSettingsConfig(serializer);

	// Add special preprocessor define
	//  -> Used to query whether it's the game build (i.e. not the Oxygen App), and to get its build number
	mPreprocessorDefinitions.setDefinition("GAMEAPP", BUILD_NUMBER);

	// Setup the default game profile data accordingly
	fillDefaultGameProfile(GameProfile::instance());

	return true;
}

bool ConfigurationImpl::loadSettingsInternal(JsonSerializer& serializer, SettingsType settingsType)
{
	serializeSettingsInternal(serializer);

	return true;
}

void ConfigurationImpl::saveSettingsInternal(JsonSerializer& serializer, SettingsType settingsType)
{
	if (settingsType != SettingsType::STANDARD)
		return;

	serializeSettingsInternal(serializer);
	serializeSharedSettingsConfig(serializer);
}

void ConfigurationImpl::serializeSettingsInternal(JsonSerializer& serializer)
{
	// Format info & metadata
	if (serializer.isReading())
	{
		serializer.serialize("GameVersion", mGameVersionInSettings);
	}
	else
	{
		std::string buildString = BUILD_STRING;
		serializer.serialize("GameVersion", buildString);
		serializer.serialize("GameExePath", mExePath);
	}

	// Audio
	serializer.serialize("Audio_MusicVolume", mMusicVolume);
	serializer.serialize("Audio_SoundVolume", mSoundVolume);
	serializer.serialize("ActiveSoundtrack", mActiveSoundtrack);

	// Input
	serializer.serialize("GamepadVisualStyle", mGamepadVisualStyle);

	// Game simulation
	if (serializer.isReading())
	{
		if (serializer.serialize("SimulationFrequency", mSimulationFrequency))
		{
			mSimulationFrequency = clamp(mSimulationFrequency, 30, 240);
		}
	}
	else
	{
		if (mSimulationFrequency != 60)
		{
			serializer.serialize("SimulationFrequency", mSimulationFrequency);
		}
	}

	// Time Attack
	serializer.serialize("InstantTimeAttackRestart", mInstantTimeAttackRestart);

	// Game settings
	if (serializer.isReading())
	{
		if (!mGameVersionInSettings.empty())
		{
			if (!SharedDatabase::getSettings().empty())		// This is going to be empty when the macOS UI calls loadConfiguration externally, causing crash
			{
				if (serializer.beginObject("GameSettings"))
				{
					const auto& settingsMap = SharedDatabase::getSettings();

					// Load settings as uint32 values
					for (auto& pair : settingsMap)
					{
						if (pair.second.mSerializationType != SharedDatabase::Setting::SerializationType::NONE)
						{
							int value = 0;
							if (serializer.serialize(pair.second.mIdentifier.c_str(), value))
							{
								pair.second.mCurrentValue = (uint32)value;
							}
						}
					}

					// Make corrections where needed
					if (mGameVersionInSettings < "22.12.17.0")
					{
						// Reset the SETTING_FIX_GLITCHES, after the default value changed
						const SharedDatabase::Setting* setting = mapFind(settingsMap, (uint32)SharedDatabase::Setting::SETTING_FIX_GLITCHES);
						if (nullptr != setting)
							setting->mCurrentValue = 2;
					}

					const SharedDatabase::Setting* ghostsSetting = mapFind(settingsMap, (uint32)SharedDatabase::Setting::SETTING_TIME_ATTACK_GHOSTS);
					if (nullptr != ghostsSetting)
						ghostsSetting->mCurrentValue = (ghostsSetting->mCurrentValue >= 5) ? 5 : (ghostsSetting->mCurrentValue >= 3) ? 3 : (ghostsSetting->mCurrentValue >= 1) ? 1 : 0;

					serializer.endObject();
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
		}
	}
	else
	{
		if (serializer.beginObject("GameSettings"))
		{
			const auto& settingsMap = SharedDatabase::getSettings();
			for (auto& pair : settingsMap)
			{
				const SharedDatabase::Setting& setting = pair.second;
				if (setting.mSerializationType == SharedDatabase::Setting::SerializationType::NONE)
					continue;
				if (setting.mSerializationType == SharedDatabase::Setting::SerializationType::HIDDEN && setting.mCurrentValue == setting.mDefaultValue)
					continue;

				int value = setting.mCurrentValue;
				serializer.serialize(setting.mIdentifier.c_str(), value);
			}
			serializer.endObject();
		}
	}

	// All that is shared with config
	serializeSharedSettingsConfig(serializer);
}

void ConfigurationImpl::serializeSharedSettingsConfig(JsonSerializer& serializer)
{
	// Dev mode
	if (serializer.isReading())		// These two properties are only supposed to be read, but not written if they don't exist already
	{
		if (serializer.beginObject("DevMode"))
		{
			serializer.serialize("EnforceDebugMode", mDevModeImpl.mEnforceDebugMode);
			serializer.serialize("SkipExitConfirmation", mDevModeImpl.SkipExitConfirmation);
			serializer.endObject();
		}
	}

	// Game server
	if (serializer.beginObject("GameServer"))
	{
		// Update Check settings
		if (serializer.beginObject("UpdateCheck"))
		{
			serializer.serialize("ReleaseChannel", mGameServerImpl.mUpdateCheck.mReleaseChannel);
			serializer.endObject();
		}

		// Ghost Sync settings
		if (serializer.beginObject("GhostSync"))
		{
			serializer.serialize("Enabled", mGameServerImpl.mGhostSync.mEnabled);
			serializer.serialize("ChannelName", mGameServerImpl.mGhostSync.mChannelName);
			serializer.serialize("ShowOffscreenGhosts", mGameServerImpl.mGhostSync.mShowOffscreenGhosts);
			serializer.serialize("GhostRendering", mGameServerImpl.mGhostSync.mGhostRendering);
			serializer.endObject();
		}

		serializer.endObject();
	}
}
