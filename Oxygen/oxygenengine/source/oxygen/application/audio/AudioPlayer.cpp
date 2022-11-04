/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/AudioPlayer.h"
#include "oxygen/application/audio/EmulationAudioSource.h"

AudioPlayer::SoundIterator::SoundIterator(std::vector<PlayingSound>& sounds) :
	mSounds(sounds)
{
}

void AudioPlayer::SoundIterator::filterContext(int contextId)
{
	mFilterContextId = contextId;
}

void AudioPlayer::SoundIterator::filterChannel(int channelId)
{
	mFilterChannelId = channelId;
}

void AudioPlayer::SoundIterator::filterState(PlayingSound::State state)
{
	mFilterState = state;
}

AudioPlayer::PlayingSound* AudioPlayer::SoundIterator::getCurrent()
{
	return (mCurrentIndex >= 0 && mCurrentIndex < (int)mSounds.size()) ? &mSounds[mCurrentIndex] : nullptr;
}

AudioPlayer::PlayingSound* AudioPlayer::SoundIterator::getNext()
{
	if (mCurrentIndex < -1)
		mCurrentIndex = -1;

	while (mCurrentIndex < (int)mSounds.size())
	{
		++mCurrentIndex;
		if (mCurrentIndex >= (int)mSounds.size())
			break;

		PlayingSound& sound = mSounds[mCurrentIndex];

		if (mFilterContextId != -1 && sound.mContextId != mFilterContextId)
			continue;
		if (mFilterChannelId != -1 && sound.mChannelId != mFilterChannelId)
			continue;
		if (mFilterState != PlayingSound::State::NONE && sound.mState != mFilterState)
			continue;

		return &sound;
	}
	return nullptr;
}

void AudioPlayer::SoundIterator::removeCurrent()
{
	if (mCurrentIndex < 0 || mCurrentIndex >= (int)mSounds.size())
		return;

	// Is this the last sound?
	if (mCurrentIndex + 1 < (int)mSounds.size())
	{
		mSounds[mCurrentIndex] = mSounds.back();
		--mCurrentIndex;
	}
	mSounds.pop_back();
}



void AudioPlayer::startup()
{
	mLastAudioTime = FTX::Audio->getGlobalPlayedSamples();
}

void AudioPlayer::shutdown()
{
	stopAllSounds(true);
        mAudioSourceManager.clear();
}

void AudioPlayer::clearPlayback()
{
	stopAllSounds(true);
	mAudioSourceManager.clear();
}

bool AudioPlayer::playAudio(uint64 sfxId, int contextId)
{
	SourceRegistration* sourceReg = mAudioCollection.getSourceRegistration(sfxId);
	if (nullptr == sourceReg)
		return false;

	PlayingSound* playingSound = playAudioInternal(sourceReg, sourceReg->mAudioDefinition->mChannel, contextId);
	return (nullptr != playingSound);
}

void AudioPlayer::playOverride(uint64 sfxId, int contextId, int channelId, int overriddenChannelId)
{
	// Deactivate duplicates first
	for (ChannelOverride& channelOverride : mChannelOverrides)
	{
		if (channelOverride.mPlayingChannelId == channelId)
		{
			channelOverride.mActive = false;
		}
	}

	PlayingSound* playingSound = nullptr;
	{
		SourceRegistration* sourceReg = mAudioCollection.getSourceRegistration(sfxId);
		if (nullptr != sourceReg)
		{
			// Start playback of new sound
			playingSound = playAudioInternal(sourceReg, channelId, contextId);
		}
	}

	if (nullptr != playingSound)
	{
		// Create new channel override
		ChannelOverride& channelOverride = vectorAdd(mChannelOverrides);
		channelOverride.mPlayingChannelId = channelId;
		channelOverride.mOverriddenChannelId = overriddenChannelId;
		channelOverride.mPlayingSoundUniqueId = playingSound->mUniqueId;

		// Apply its effects
		applyChannelOverride(overriddenChannelId);
	}
}

void AudioPlayer::updatePlayback(float timeElapsed)
{
	// Update delta time
	//  -> This is used to slightly delay sounds, so that their starting time gets considered correctly
	//  -> Otherwise we get irregularities as the audio update rate (44100 / 1024 = around 43 Hz) is not in sync with the simulation rate (60 Hz)
	//  -> So this is mainly here to smooth out playback starting positions, especially for small sounds starting in quick succession (e.g. the Sonic game's score tally counting)
	{
		mLastAudioTime += roundToInt(timeElapsed * (float)FTX::Audio->getOutputFrequency());
		const int bufferSize = FTX::Audio->getOutputBufferSize();
		const uint32 audioPlayedSamples = FTX::Audio->getGlobalPlayedSamples() + bufferSize;	// Look into the future! Because the actual playback start will still take a while, so it's not unlikely there will be another audio update
		int difference = mLastAudioTime - audioPlayedSamples;
		difference = clamp(difference, -bufferSize, bufferSize);
		mLastAudioTime = audioPlayedSamples + difference;
	}

	// Attention, we're using a different timer for the rest
	const float currentTime = FTX::getTime();

	// Update playing sounds list
	SoundIterator iterator(mPlayingSounds);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		PlayingSound& sound = *soundPtr;
		if (!sound.mAudioRef.valid())
		{
			iterator.removeCurrent();
			continue;
		}

		if (sound.mState == PlayingSound::State::PLAYING)
		{
			// Update read time for streaming
			if (nullptr != sound.mAudioSource)
			{
				sound.mAudioSource->updateReadTime(sound.mAudioRef.getPosition());

				// Note that this is done also for paused tracks, i.e. they won't get unloaded
				sound.mAudioSource->setLastUsedTimestamp(currentTime);
			}

			// Update fading
			if (sound.mRelativeVolumeChange != 0.0f)
			{
				sound.mRelativeVolume += sound.mRelativeVolumeChange * timeElapsed;
				if (sound.mRelativeVolumeChange < 0.0f)
				{
					// Fading out
					if (sound.mRelativeVolume <= 0.0f)
					{
						// Stop sound now - as usual, using a quick volume change
						sound.mAudioRef.setVolumeChange(-20.0f);
						iterator.removeCurrent();
						continue;
					}
				}
				else
				{
					// Fading in
					if (sound.mRelativeVolume >= 1.0f)
					{
						sound.mRelativeVolume = 1.0f;
						sound.mRelativeVolumeChange = 0.0f;
					}
				}

				sound.mAudioRef.setVolume(sound.mBaseVolume * sound.mRelativeVolume);
			}
		}
		else if (sound.mState == PlayingSound::State::OVERRIDDEN)
		{
			// Make sure an overridden track does not get unloaded
			if (nullptr != sound.mAudioSource)
			{
				sound.mAudioSource->setLastUsedTimestamp(currentTime);
			}
		}
	}

	// Update channel overrides
	for (size_t i = 0; i < mChannelOverrides.size(); ++i)
	{
		ChannelOverride& channelOverride = mChannelOverrides[i];
		if (channelOverride.mActive)
		{
			// Channel override gets inactive autonmatically when the overriding sound was played
			PlayingSound* soundPtr = getPlayingSound(channelOverride.mPlayingSoundUniqueId);
			if (nullptr == soundPtr || !soundPtr->mAudioRef.valid())
			{
				channelOverride.mActive = false;
			}
		}

		if (!channelOverride.mActive)
		{
			// Check if overridden channel is still affected by another override
			if (!isChannelOverridden(channelOverride.mOverriddenChannelId))
			{
				// Undo its effects
				removeChannelOverride(channelOverride.mOverriddenChannelId);
			}

			// Remove from list
			mChannelOverrides.erase(mChannelOverrides.begin() + i);
			--i;
		}
	}

	// Update auto-streamers
	for (size_t i = 0; i < mAutoStreamers.size(); ++i)
	{
		AutoStreamer& autoStreamer = mAutoStreamers[i];
		autoStreamer.mTime += timeElapsed * autoStreamer.mSpeed;
		autoStreamer.mAudioSource->updateReadTime(autoStreamer.mTime);

		if (autoStreamer.mAudioSource->isStreaming())
		{
			autoStreamer.mAudioSource->setLastUsedTimestamp(currentTime);
		}
		else
		{
			mAutoStreamers.erase(mAutoStreamers.begin() + i);
			--i;
		}
	}

	// Update streaming & unloading
	mAudioSourceManager.updateStreaming(currentTime);

	// Update rmx audio manager
	FTX::Audio->regularUpdate(timeElapsed);
}

bool AudioPlayer::isPlayingSfxId(uint64 sfxId, AudioReference* outAudioRef) const
{
	for (const PlayingSound& playingSound : mPlayingSounds)
	{
		if (playingSound.mBaseSourceReg->mAudioDefinition->mKeyId == sfxId)
		{
			if (nullptr != outAudioRef)
				*outAudioRef = playingSound.mAudioRef;
			return true;
		}
	}
	return false;
}

bool AudioPlayer::getAudioRefByChannel(int channelId, AudioReference& outAudioRef) const
{
	for (const PlayingSound& playingSound : mPlayingSounds)
	{
		if (playingSound.mContextId == channelId)
		{
			outAudioRef = playingSound.mAudioRef;
			return true;
		}
	}
	return false;
}

bool AudioPlayer::getAudioRefByContext(int contextId, AudioReference& outAudioRef) const
{
	for (const PlayingSound& playingSound : mPlayingSounds)
	{
		if (playingSound.mContextId == contextId)
		{
			outAudioRef = playingSound.mAudioRef;
			return true;
		}
	}
	return false;
}

void AudioPlayer::changeSoundContext(AudioReference& audioRef, int contextId)
{
	PlayingSound* soundPtr = getPlayingSound(audioRef);
	if (nullptr != soundPtr)
	{
		soundPtr->mContextId = contextId;
	}
}

void AudioPlayer::stopAllSounds(bool immediately)
{
	if (immediately)
	{
		for (PlayingSound& playingSound : mPlayingSounds)
		{
			playingSound.mAudioRef.stop();
		}
	}
	else
	{
		for (PlayingSound& playingSound : mPlayingSounds)
		{
			// We're not actually stopping the sound, but just fading it out very fast
			playingSound.mAudioRef.setVolumeChange(-20.0f);
		}
	}
	mPlayingSounds.clear();
}

void AudioPlayer::stopAllSoundsByChannel(int channelId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(channelId);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		// We're not actually stopping the sound, but just fading it out very fast
		soundPtr->mAudioRef.setVolumeChange(-20.0f);
		iterator.removeCurrent();
	}
}

void AudioPlayer::stopAllSoundsByChannelAndContext(int channelId, int contextId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(channelId);
	iterator.filterContext(contextId);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		// We're not actually stopping the sound, but just fading it out very fast
		soundPtr->mAudioRef.setVolumeChange(-20.0f);
		iterator.removeCurrent();
	}
}

void AudioPlayer::fadeInChannel(int channelId, float length)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(channelId);
	iterator.filterState(PlayingSound::State::PLAYING);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		soundPtr->mRelativeVolumeChange = (length > 0.0f) ? (1.0f / length) : 0.1f;
	}
}

void AudioPlayer::fadeOutChannel(int channelId, float length)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(channelId);
	iterator.filterState(PlayingSound::State::PLAYING);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		soundPtr->mRelativeVolumeChange = (length > 0.0f) ? (-1.0f / length) : 0.1f;
	}
}

void AudioPlayer::pauseAllSoundsByContext(int contextId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterContext(contextId);
	iterator.filterState(PlayingSound::State::PLAYING);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		soundPtr->mAudioRef.setPause(true);
	}
}

void AudioPlayer::resumeAllSoundsByContext(int contextId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterContext(contextId);
	iterator.filterState(PlayingSound::State::PLAYING);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		soundPtr->mAudioRef.setPause(false);
	}
}

void AudioPlayer::stopAllSoundsByContext(int contextId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterContext(contextId);
	iterator.filterState(PlayingSound::State::PLAYING);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		if (soundPtr->mAudioRef.isPaused())
		{
			// No need for quick fade-out if it's paused anyways
			soundPtr->mAudioRef.stop();
		}
		else
		{
			soundPtr->mAudioRef.setVolumeChange(-20.0f);
		}
	}
}

AudioSourceBase* AudioPlayer::findAudioSourceByRef(AudioReference& audioRef) const
{
	const PlayingSound* soundPtr = getPlayingSound(audioRef);
	return (nullptr == soundPtr) ? nullptr : soundPtr->mAudioSource;
}

float AudioPlayer::getAudioPlaybackPosition(AudioReference& audioRef) const
{
	for (const PlayingSound& playingSound : mPlayingSounds)
	{
		if (playingSound.mAudioRef.getInstanceID() == audioRef.getInstanceID())
		{
			return playingSound.mAudioSource->mapAudioRefPositionToTrackPosition(audioRef.getPosition());
		}
	}
	return 0.0f;
}

void AudioPlayer::resetChannelOverrides()
{
	// Stop all overridden sounds
	SoundIterator iterator(mPlayingSounds);
	iterator.filterState(PlayingSound::State::OVERRIDDEN);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		soundPtr->mAudioRef.stop();
		iterator.removeCurrent();
	}
	mChannelOverrides.clear();
}

void AudioPlayer::resetAudioModifiers()
{
	for (AudioModifier& modifier : mActiveAudioModifiers)
	{
		// Remove its effects
		applyAudioModifier(modifier.mChannelId, modifier.mContextId, "", 1.0f, 1.0f / modifier.mRelativeSpeed);
	}
	mActiveAudioModifiers.clear();
}

void AudioPlayer::enableAudioModifier(int channelId, int contextId, std::string_view postfix, float relativeSpeed)
{
	// Search for existing modifier to overwrite
	AudioModifier* existingModifier = findAudioModifier(channelId, contextId);
	if (nullptr == existingModifier)
	{
		// Add a new modifier with default properties; these will get overwritten afterwards
		AudioModifier& modifier = vectorAdd(mActiveAudioModifiers);
		modifier.mChannelId = channelId;
		modifier.mContextId = contextId;
		modifier.mPostfix.clear();
		modifier.mRelativeSpeed = 1.0f;
		existingModifier = &modifier;
	}

	// Use sane values
	relativeSpeed = clamp(relativeSpeed, 0.1f, 10.0f);

	applyAudioModifier(channelId, contextId, postfix, relativeSpeed, relativeSpeed / existingModifier->mRelativeSpeed);

	existingModifier->mPostfix = postfix;
	existingModifier->mRelativeSpeed = relativeSpeed;
}

void AudioPlayer::disableAudioModifier(int channelId, int contextId)
{
	// Search for the modifier, if it even exists
	int index = -1;
	AudioModifier* modifier = findAudioModifier(channelId, contextId, &index);
	if (nullptr != modifier)
	{
		// Remove its effects
		applyAudioModifier(channelId, contextId, "", 1.0f, 1.0f / modifier->mRelativeSpeed);
		mActiveAudioModifiers.erase(mActiveAudioModifiers.begin() + index);
	}
}

size_t AudioPlayer::getMemoryUsage() const
{
	return mAudioSourceManager.getMemoryUsage();
}

AudioPlayer::PlayingSound* AudioPlayer::playAudioInternal(SourceRegistration* sourceReg, int channelId, int contextId)
{
	RMX_ASSERT(nullptr != sourceReg, "Got a null pointer for sourceReg");

	// Stop all old sounds of this channel
	if (channelId != 0xff && sourceReg->mType != SourceRegistration::Type::EMULATION_CONTINUOUS)
	{
		stopAllSoundsByChannelAndContext(channelId, contextId);
	}

	// Check for channel overrides
	const bool isOverridden = isChannelOverridden(channelId);
	const float volume = isOverridden ? 0.0f : 1.0f;

	// Start playing that sound
	PlayingSound* playingSound = nullptr;
	if (sourceReg->mType == SourceRegistration::Type::EMULATION_CONTINUOUS)
	{
		playingSound = startOrContinuePlayback(*sourceReg, volume, sourceReg->mEmulationSfxId, contextId, channelId);
	}
	else
	{
		playingSound = startPlayback(*sourceReg, 0.0f, volume, contextId, channelId);
	}

	if (nullptr == playingSound)
	{
		// Failed
		return nullptr;
	}

	// Respect audio override
	if (isOverridden)
	{
		playingSound->mState = PlayingSound::State::OVERRIDDEN;
		playingSound->mAudioRef.setPause(true);
		playingSound->mRelativeVolume = 0.0f;
		playingSound->mBaseVolume = 1.0f;
	}

	// Apply audio modifier if needed
	AudioModifier* modifier = findAudioModifier(channelId, contextId);
	if (nullptr != modifier)
	{
		SoundIterator iterator(mPlayingSounds);
		while (PlayingSound* soundPtr = iterator.getNext())
		{
			if (soundPtr == playingSound)
			{
				applyAudioModifierSingle(iterator, modifier->mPostfix, modifier->mRelativeSpeed, modifier->mRelativeSpeed);
				break;
			}
		}
	}

	// Success
	playingSound->mBaseSourceReg = sourceReg;
	return playingSound;
}

AudioPlayer::PlayingSound* AudioPlayer::startPlayback(SourceRegistration& sourceReg, float time, float volume, int contextId, int channelId)
{
	AudioSourceBase* audioSource = mAudioSourceManager.getAudioSourceForPlayback(sourceReg);
	if (nullptr == audioSource)
	{
		// Unknown audio source
		return nullptr;
	}

	// Start a new playing sound instance
	return startPlaybackInternal(sourceReg, *audioSource, volume, time, contextId, channelId);
}

AudioPlayer::PlayingSound* AudioPlayer::startOrContinuePlayback(SourceRegistration& sourceReg, float volume, uint64 soundId, int contextId, int channelId)
{
	AudioSourceBase* audioSource = mAudioSourceManager.getAudioSourceForPlayback(sourceReg);
	if (nullptr == audioSource)
	{
		// Unknown audio source
		return nullptr;
	}

	if (audioSource->isEmulationAudioSource())
	{
		RMX_CHECK(soundId < 0x100, "Emulation audio is only supported for audio IDs between 0x00 and 0xff", return nullptr);

		EmulationAudioSource& emulationAudioSource = static_cast<EmulationAudioSource&>(*audioSource);
		if (emulationAudioSource.isDynamic())
		{
			// Search for a playing sound instance of this audio source
			for (PlayingSound& sound : mPlayingSounds)
			{
				if (sound.mAudioSource == audioSource)
				{
					// Just tell audio source to play the new ID as well, and continue playing this instance
					emulationAudioSource.injectPlaySound((uint8)soundId);
					return &sound;
				}
			}

			// Reset before starting a new playback
			emulationAudioSource.resetContent();
		}
	}

	// Start a new playing sound instance
	return startPlaybackInternal(sourceReg, *audioSource, volume, -1.0f, contextId, channelId);
}

AudioPlayer::PlayingSound* AudioPlayer::startPlaybackInternal(SourceRegistration& sourceReg, AudioSourceBase& audioSource, float volume, float time, int contextId, int channelId)
{
	// Startup audio source and get the audio buffer
	AudioBuffer* audioBuffer = audioSource.startup(0.1f);
	if (nullptr == audioBuffer)
		return nullptr;

	// Start playing audio
	rmx::AudioManager::PlaybackOptions options;
	options.mAudioBuffer = audioBuffer;
	options.mStreaming = true;
	options.mVolume = volume;
	options.mAudioMixerId = contextId + 0x11;	// Translate "AudioOutBase::Context" to "AudioOutBase::AudioMixerId"

	AudioReference audioRef;
	FTX::Audio->lockAudio();
	{
		// Calculate a small delay, which should at maximum be one audio output buffer size worth of time
		const int difference = (int)(mLastAudioTime - FTX::Audio->getGlobalPlayedSamples());
		if (difference > 0)		// No "negative delay" please
		{
			const float delay = (float)difference / (float)FTX::Audio->getOutputFrequency();
			options.mPosition = -delay;
		}

		// Add the sound
		FTX::Audio->addSound(options, audioRef);

		// Additional configuration depending on audio source type
		audioSource.onPlaybackStart(audioRef, time);
	}
	FTX::Audio->unlockAudio();

	// Register as playing sound here
#if DEBUG && defined(PLATFORM_WINDOWS)
	// Memory debugging
	if (mPlayingSounds.size() == mPlayingSounds.capacity() && !mPlayingSounds.empty())
	{
		PlayingSound* oldData = &mPlayingSounds[0];
		mPlayingSounds.reserve(mPlayingSounds.capacity() + 1);
		memset(oldData, 0xdd, sizeof(PlayingSound) * mPlayingSounds.size());
	}
#endif
	PlayingSound& sound = vectorAdd(mPlayingSounds);
	sound.mUniqueId = ++mLastUniqueId;
	sound.mAudioRef = audioRef;
	sound.mSourceReg = &sourceReg;
	sound.mBaseSourceReg = &sourceReg;
	sound.mAudioSource = &audioSource;
	sound.mBaseVolume = volume;
	sound.mRelativeVolume = 1.0f;
	sound.mContextId = contextId;
	sound.mChannelId = channelId;
	return &sound;
}

AudioPlayer::PlayingSound* AudioPlayer::getPlayingSound(uint32 uniqueId)
{
	for (PlayingSound& playingSound : mPlayingSounds)
	{
		if (playingSound.mUniqueId == uniqueId)
		{
			return &playingSound;
		}
	}
	return nullptr;
}

AudioPlayer::PlayingSound* AudioPlayer::getPlayingSound(AudioReference& audioRef)
{
	if (audioRef.valid())
	{
		// Just go through the playing sounds
		//  -> No need for a map or similar, as we usually have only a few of them active at the same time
		const int instanceId = audioRef.getInstanceID();
		for (PlayingSound& playingSound : mPlayingSounds)
		{
			if (playingSound.mAudioRef.getInstanceID() == instanceId)
			{
				return &playingSound;
			}
		}
	}
	return nullptr;
}

const AudioPlayer::PlayingSound* AudioPlayer::getPlayingSound(AudioReference& audioRef) const
{
	return const_cast<AudioPlayer*>(this)->getPlayingSound(audioRef);
}

void AudioPlayer::applyChannelOverride(int overriddenChannelId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(overriddenChannelId);
	iterator.filterState(PlayingSound::State::PLAYING);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		PlayingSound& sound = *soundPtr;
		if (sound.mAudioRef.valid())
		{
			sound.mState = PlayingSound::State::OVERRIDDEN;
			sound.mAudioRef.setPause(true);
		}
	}
}

void AudioPlayer::removeChannelOverride(int overriddenChannelId)
{
	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(overriddenChannelId);
	iterator.filterState(PlayingSound::State::OVERRIDDEN);
	while (PlayingSound* soundPtr = iterator.getNext())
	{
		PlayingSound& sound = *soundPtr;
		if (sound.mAudioRef.valid())
		{
			// Get active again, with a fade-in
			sound.mRelativeVolume = 0.0f;
			sound.mRelativeVolumeChange = 1.0f;
			sound.mAudioRef.setVolume(0.0f);
			sound.mAudioRef.setPause(false);
			sound.mState = PlayingSound::State::PLAYING;
		}
	}
}

bool AudioPlayer::isChannelOverridden(int channelId) const
{
	for (const ChannelOverride& channelOverride : mChannelOverrides)
	{
		if (channelOverride.mOverriddenChannelId == channelId && channelOverride.mActive)
			return true;
	}
	return false;
}

AudioPlayer::AudioModifier* AudioPlayer::findAudioModifier(int channelId, int contextId, int* outIndex)
{
	for (size_t i = 0; i < mActiveAudioModifiers.size(); ++i)
	{
		AudioModifier& modifier = mActiveAudioModifiers[i];
		if (modifier.mChannelId == channelId && modifier.mContextId == contextId)
		{
			if (nullptr != outIndex)
				*outIndex = (int)i;
			return &modifier;
		}
	}
	return nullptr;
}

AudioPlayer::SourceRegistration* AudioPlayer::getModifiedSourceRegistration(SourceRegistration& baseSourceReg, std::string_view postfix) const
{
	std::string newKeyString = baseSourceReg.mAudioDefinition->mKeyString;
	newKeyString.append(postfix);	// It would be nice if "std::string + std::string_view" would be supported by the STL
	return mAudioCollection.getSourceRegistration(rmx::getMurmur2_64(newKeyString), baseSourceReg.mPackage);
}

void AudioPlayer::applyAudioModifier(int channelId, int contextId, std::string_view postfix, float relativeSpeed, float speedChange)
{
	// Audio modifiers have an effect in three places:
	//  a) New sounds starting use it (gets handled in "playAudioInternal", not here)
	//  b) Currently playing sounds have to apply it
	//  c) Paused sounds have to apply it

	SoundIterator iterator(mPlayingSounds);
	iterator.filterChannel(channelId);
	iterator.filterContext(contextId);
	while (nullptr != iterator.getNext())
	{
		applyAudioModifierSingle(iterator, postfix, relativeSpeed, speedChange);
	}
}

void AudioPlayer::applyAudioModifierSingle(SoundIterator& iterator, std::string_view postfix, float relativeSpeed, float speedChange)
{
	PlayingSound& playingSound = *iterator.getCurrent();
	if (!playingSound.mAudioRef.valid())
		return;

	if (playingSound.mAudioSource->isEmulationAudioSource())
	{
		// Emulated audio source: Only music tempo speedup is supported
		uint8 tempoSpeedup = 0;
		if (relativeSpeed > 1.01f)
		{
			tempoSpeedup = 2 * roundToInt(1.0f / (relativeSpeed - 1.0f));
		}

		EmulationAudioSource& emulationAudioSource = static_cast<EmulationAudioSource&>(*playingSound.mAudioSource);
		emulationAudioSource.injectTempoSpeedup(tempoSpeedup);
	}
	else
	{
		// Non-emulated audio source: Switch to a different audio file to play
		PlayingSound& oldSound = playingSound;

		// Get new source
		SourceRegistration* newSourceReg = getModifiedSourceRegistration(*oldSound.mBaseSourceReg, postfix);
		if (nullptr == newSourceReg)
			return;

		// Check if the postfix version is already playing
		if (oldSound.mSourceReg == newSourceReg)
			return;

		// If original sound is modded, we require the modified sound to be modded as well
		if (oldSound.mSourceReg->mPackage == AudioCollection::Package::MODDED && newSourceReg->mPackage != AudioCollection::Package::MODDED)
			return;

		if (oldSound.mState == PlayingSound::State::NONE)
			return;

		const bool isOverridden = (oldSound.mState == PlayingSound::State::OVERRIDDEN);
		const float newPosition = oldSound.mAudioSource->mapAudioRefPositionToTrackPosition(oldSound.mAudioRef.getPosition()) / speedChange;
		const float newVolume = isOverridden ? 0.0f : newSourceReg->mVolume;	// If pushed back, start muted until we can pause it just below

		PlayingSound* newSound = startPlayback(*newSourceReg, newPosition, newVolume, oldSound.mContextId, oldSound.mChannelId);
		if (nullptr != newSound)
		{
			// Attention: Pointer to oldSound potentially got invalid after adding a new sound, so we better fetch it again
			PlayingSound& oldSound = *iterator.getCurrent();

			newSound->mBaseSourceReg = oldSound.mBaseSourceReg;
			newSound->mState = oldSound.mState;

			if (isOverridden)
			{
				newSound->mAudioRef.setPause(true);
				newSound->mBaseVolume = newSourceReg->mVolume;
			}
			else
			{
				// Quick cross fade
				oldSound.mAudioRef.setVolumeChange(-50.0f);
				newSound->mAudioRef.setVolumeChange(50.0f);
			}

			// Add or remove auto-streamer
			if (oldSound.mSourceReg == oldSound.mBaseSourceReg)
			{
				startAutoStreamer(*oldSound.mAudioSource, oldSound.mAudioRef.getPosition(), speedChange);
			}
			if (newSound->mSourceReg == newSound->mBaseSourceReg)
			{
				stopAutoStreamer(*newSound->mAudioSource);
			}

			// Remove old playing sound
			iterator.removeCurrent();
		}
	}
}

void AudioPlayer::startAutoStreamer(AudioSourceBase& audioSource, float currentTime, float speed)
{
	// Check if one is already active for that audio source ID
	for (size_t i = 0; i < mAutoStreamers.size(); ++i)
	{
		AutoStreamer& autoStreamer = mAutoStreamers[i];
		if (autoStreamer.mAudioSource == &audioSource)
		{
			autoStreamer.mTime = std::max(currentTime, autoStreamer.mTime);
			autoStreamer.mSpeed = speed;
			return;
		}
	}

	if (audioSource.isCompletelyLoaded())
	{
		// No need for an auto-streamer if everything is loaded already
		return;
	}
	else if (!audioSource.isStreaming())
	{
		// Startup the audio source if it's not playing yet
		audioSource.startup(0.0f);
	}

	AutoStreamer& autoStreamer = vectorAdd(mAutoStreamers);
	autoStreamer.mAudioSource = &audioSource;
	autoStreamer.mTime = currentTime;
	autoStreamer.mSpeed = speed;
}

void AudioPlayer::stopAutoStreamer(AudioSourceBase& audioSource)
{
	for (size_t i = 0; i < mAutoStreamers.size(); ++i)
	{
		AutoStreamer& autoStreamer = mAutoStreamers[i];
		if (autoStreamer.mAudioSource == &audioSource)
		{
			mAutoStreamers.erase(mAutoStreamers.begin() + i);
			return;
		}
	}
}
