/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/audio/AudioPlayer.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/Configuration.h"


AudioOutBase::AudioOutBase() :
	mAudioPlayer(mAudioCollection)
{
}

AudioOutBase::~AudioOutBase()
{
}

void AudioOutBase::startup()
{
	// Create audio mixers
	FTX::Audio->createAudioMixer<rmx::AudioMixer>((int)AudioMixerId::INGAME_MASTER, (int)AudioMixerId::ROOT);
	FTX::Audio->createAudioMixer<rmx::AudioMixer>((int)AudioMixerId::INGAME_MUSIC,  (int)AudioMixerId::INGAME_MASTER);
	FTX::Audio->createAudioMixer<rmx::AudioMixer>((int)AudioMixerId::INGAME_SOUND,  (int)AudioMixerId::INGAME_MASTER);
	FTX::Audio->createAudioMixer<rmx::AudioMixer>((int)AudioMixerId::MENU_MASTER,   (int)AudioMixerId::ROOT);
	FTX::Audio->createAudioMixer<rmx::AudioMixer>((int)AudioMixerId::MENU_MUSIC,    (int)AudioMixerId::MENU_MASTER);
	FTX::Audio->createAudioMixer<rmx::AudioMixer>((int)AudioMixerId::MENU_SOUND,    (int)AudioMixerId::MENU_MASTER);

	// Load audio definitions
	//  -> No mods yet here, that's coming later in "handleGameLoaded"
	mAudioCollection.loadFromJson(L"data/audio/original", L"audio_default.json", AudioCollection::Package::ORIGINAL);
	mAudioCollection.loadFromJson(L"data/audio/remastered", L"audio_replacements.json", AudioCollection::Package::REMASTERED);
	determineActiveSourceRegistrations();

	// Startup
	mAudioPlayer.startup();
	setGlobalVolume(Configuration::instance().mAudioVolume);
}

void AudioOutBase::shutdown()
{
	mAudioPlayer.shutdown();
}

void AudioOutBase::realtimeUpdate(float secondsPassed)
{
	// Sync volumes
	if (mGlobalVolume != Configuration::instance().mAudioVolume)
	{
		setGlobalVolume(Configuration::instance().mAudioVolume);
	}

	// Update playback and streaming
	mAudioPlayer.updatePlayback(secondsPassed);
}

void AudioOutBase::setGlobalVolume(float volume)
{
	mGlobalVolume = volume;
	FTX::Audio->setGlobalVolume(mGlobalVolume);
}

bool AudioOutBase::isPlayingSfxId(uint64 sfxId) const
{
	return mAudioPlayer.isPlayingSfxId(sfxId);
}

bool AudioOutBase::playAudioBase(uint64 sfxId, uint8 contextId)
{
	return mAudioPlayer.playAudio(sfxId, contextId);
}

void AudioOutBase::playOverride(uint64 sfxId, uint8 contextId, uint8 channelId, uint8 overriddenChannelId)
{
	mAudioPlayer.playOverride(sfxId, contextId, channelId, overriddenChannelId);
}

void AudioOutBase::stopChannel(uint8 channelId)
{
	mAudioPlayer.stopAllSoundsByChannel(channelId);
}

void AudioOutBase::fadeInChannel(uint8 channelId, float length)
{
	mAudioPlayer.fadeInChannel(channelId, length);
}

void AudioOutBase::fadeOutChannel(uint8 channelId, float length)
{
	mAudioPlayer.fadeOutChannel(channelId, length);
}

void AudioOutBase::enableAudioModifier(uint8 channelId, uint8 contextId, const std::string& postfix, float relativeSpeed)
{
	mAudioPlayer.enableAudioModifier(channelId, contextId, postfix, relativeSpeed);
}

void AudioOutBase::disableAudioModifier(uint8 channelId, uint8 contextId)
{
	mAudioPlayer.disableAudioModifier(channelId, contextId);
}

void AudioOutBase::handleGameLoaded()
{
	// Active mods are now set; this is handled just like mods changed
	handleActiveModsChanged();
}

void AudioOutBase::handleActiveModsChanged()
{
	// First of all, stop all playing sounds immediately
	mAudioPlayer.stopAllSounds();
	FTX::Audio->removeAllSounds();

	// Reload of modded audio definitions
	mAudioCollection.clearPackage(AudioCollection::Package::MODDED);
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		mAudioCollection.loadFromJson(mod->mFullPath + L"audio", L"audio_replacements.json", AudioCollection::Package::MODDED);
	}
	determineActiveSourceRegistrations();

	// We could now remove all audio sources that won't be used any more, but there's no actual need to do this
	//  -> They will receive an unload due to not being actively used sooner or later
	//  -> Their instances won't get deleted, but most of their memory will get freed then
}

void AudioOutBase::determineActiveSourceRegistrations()
{
	// Default implementation: Use remastered music if possible
	mAudioCollection.determineActiveSourceRegistrations(false);
}
