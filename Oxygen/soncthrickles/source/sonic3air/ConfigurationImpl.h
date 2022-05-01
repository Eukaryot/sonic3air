/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
	void loadSettingsInternal(JsonHelper& rootHelper, SettingsType settingsType, bool isDeprecatedJson);

public:
	// Audio
	float mMusicVolume = 0.8f;
	float mSoundVolume = 0.8f;
	int mActiveSoundtrack = 1;		// 0 = emulated, 1 = remastered

	// Time Attack
	int mInstantTimeAttackRestart = 0;

	// Settings game version
	std::string mGameVersionInSettings;

	// Game server
	struct UpdateCheck
	{
		int mReleaseChannel = 0;	// 0 = stable, 1 = preview, 2 = test builds
	};
	struct GhostSync
	{
		bool mEnabled = false;
		std::string mChannelName;
		bool mShowOffscreenGhosts = false;
	};
	struct GameServer
	{
		std::string mServerHostName;
		int mServerPortUDP = 0;
		int mServerPortTCP = 0;
		int mServerPortWSS = 0;
		UpdateCheck mUpdateCheck;
		GhostSync mGhostSync;
	};
	GameServer mGameServer;
};
