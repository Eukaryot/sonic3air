/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"

class GameProfile;


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
	bool loadConfigurationInternal(JsonHelper& jsonHelper) override;
	bool loadSettingsInternal(JsonHelper& rootHelper, SettingsType settingsType) override;
	void saveSettingsInternal(Json::Value& root, SettingsType settingsType) override;

private:
	void loadSharedSettingsConfig(JsonHelper& rootHelper);

public:
	// Audio
	float mMusicVolume = 0.8f;
	float mSoundVolume = 0.8f;
	int mActiveSoundtrack = 1;		// 0 = emulated, 1 = remastered

	// Time Attack
	int mInstantTimeAttackRestart = 0;

	// Settings game version
	std::string mGameVersionInSettings;

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
	struct GameServer
	{
		std::string mServerHostName = "sonic3air.org";
		int mServerPortUDP = 21094;		// Used by most platforms
		int mServerPortTCP = 21095;		// Used only as a fallback for UDP
		int mServerPortWSS = 21096;		// Used by the web version
		UpdateCheck mUpdateCheck;
		GhostSync mGhostSync;
	};
	GameServer mGameServer;
};
