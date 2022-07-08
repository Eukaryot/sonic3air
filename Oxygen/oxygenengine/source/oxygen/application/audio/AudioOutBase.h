/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/audio/AudioPlayer.h"


class AudioOutBase
{
public:
	enum class AudioMixerId
	{
		ROOT		  = 0,		// This must not be changed
		INGAME_MASTER = 0x10,
		INGAME_MUSIC  = 0x11,
		INGAME_SOUND  = 0x12,
		MENU_MASTER   = 0x20,
		MENU_MUSIC    = 0x21,
		MENU_SOUND    = 0x22
	};

	enum Context
	{
		// Add together combinations
		CONTEXT_MUSIC	= 0x00,
		CONTEXT_SOUND	= 0x01,
		CONTEXT_INGAME	= 0x00,
		CONTEXT_MENU	= 0x10
	};

	enum class AudioKeyType
	{
		INVALID  = 0,
		UNMODDED = 1,
		MODDED   = 2
	};

public:
	AudioOutBase();
	virtual ~AudioOutBase();

	virtual void startup();
	virtual void shutdown();
	virtual void reset() = 0;
	virtual void resetGame() = 0;

	virtual void update(float secondsPassed) = 0;
	virtual void realtimeUpdate(float secondsPassed);

	AudioCollection& getAudioCollection()  { return mAudioCollection; }
	AudioPlayer& getAudioPlayer()		   { return mAudioPlayer; }

	inline float getGlobalVolume() const   { return mGlobalVolume; }
	void setGlobalVolume(float volume);

	AudioKeyType getAudioKeyType(uint64 sfxId) const;
	bool isPlayingSfxId(uint64 sfxId) const;

	bool playAudioBase(uint64 sfxId, uint8 contextId);
	void playOverride(uint64 sfxId, uint8 contextId, uint8 channelId, uint8 overriddenChannelId);
	void stopChannel(uint8 channelId);
	void fadeInChannel(uint8 channelId, float length);
	void fadeOutChannel(uint8 channelId, float length);

	void enableAudioModifier(uint8 channelId, uint8 contextId, std::string_view postfix, float relativeSpeed);
	void disableAudioModifier(uint8 channelId, uint8 contextId);

	void handleGameLoaded();
	void handleActiveModsChanged();

protected:
	virtual void determineActiveSourceRegistrations();

protected:
	AudioCollection mAudioCollection;
	AudioPlayer mAudioPlayer;
	float mGlobalVolume = 1.0f;
};
