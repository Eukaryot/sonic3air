/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"

#include "sonic3air/data/GameSettings.h"

class GameProfile;
class JsonSerializer;


class ConfigurationImpl : public ::Configuration
{
public:
	struct DevModeImplSettings
	{
		bool mEnforceDebugMode = false;
		bool SkipExitConfirmation = false;
	};

public:
	inline static ConfigurationImpl& instance() { return static_cast<ConfigurationImpl&>(::Configuration::instance()); }

	static void fillDefaultGameProfile(GameProfile& gameProfile);

public:
	ConfigurationImpl();

protected:
	void preLoadInitialization() override;
	bool loadConfigurationInternal(JsonSerializer& serializer) override;
	bool loadSettingsInternal(JsonSerializer& serializer, SettingsType settingsType) override;
	void saveSettingsInternal(JsonSerializer& serializer, SettingsType settingsType) override;

private:
	void serializeSettingsInternal(JsonSerializer& serializer);
	void serializeSharedSettingsConfig(JsonSerializer& serializer);

public:
	// Audio
	float mMusicVolume = 0.8f;
	float mSoundVolume = 0.8f;
	int mActiveSoundtrack = 1;		// 0 = emulated, 1 = remastered

	// Input
#if !defined(PLATFORM_VITA)
	int mGamepadVisualStyle = 0;
#else
	int mGamepadVisualStyle = 1;
#endif

	// Time Attack
	int mInstantTimeAttackRestart = 0;

	// Settings game version
	std::string mGameVersionInSettings;

	// Menus
	bool mShowControlsDisplay = true;

	// Dev mode (in addition to "mDevMode" from base class)
	DevModeImplSettings mDevModeImpl;

	// Game server
	struct UpdateCheck
	{
		int mReleaseChannel = 0;	// 0 = stable, 1 = preview, 2 = test builds
	};
	struct GhostSync
	{
		bool mEnabled = false;
		std::string mChannelName = "world";
		bool mShowOffscreenGhosts = true;
		int mGhostRendering = 3;
	};
	struct GameServerImpl
	{
		UpdateCheck mUpdateCheck;
		GhostSync mGhostSync;
	};
	GameServerImpl mGameServerImpl;

	// Game settings
	GameSettings mLocalGameSettings;			// Loaded from local settings
	GameSettings mAlternativeGameSettings;		// Used in netplay and game recording playback
	GameSettings* mActiveGameSettings = nullptr;
};
