/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/audio/AudioSourceBase.h"
#include "oxygen/application/audio/AudioSourceManager.h"
#include "oxygen/application/audio/AudioCollection.h"

#include <optional>


class AudioPlayer
{
public:
	using SourceRegistration = AudioCollection::SourceRegistration;

	struct SavedAudioState
	{
		uint64 mSfxId = 0;
		int mChannelId = -1;
		int mContextId = -1;
	};

	struct SavedPlaybackState
	{
		std::vector<SavedAudioState> mAudioStates;
	};

public:
	inline AudioPlayer(AudioCollection& audioCollection) : mAudioCollection(audioCollection) {}

	void startup();
	void shutdown();
	void clearPlayback();

	bool playAudio(uint64 sfxId, int contextId);
	bool playAudio(uint64 sfxId, int contextId, int channelId);
	void playOverride(uint64 sfxId, int contextId, int channelId, int overriddenChannel);

	void updatePlayback(float timeElapsed);

	bool isPlayingSfxId(uint64 sfxId, AudioReference* outAudioRef = nullptr) const;
	bool getAudioRefByChannel(int channelId, AudioReference& outAudioRef) const;
	bool getAudioRefByContext(int contextId, AudioReference& outAudioRef) const;
	void changeSoundContext(AudioReference& audioRef, int contextId);

	void pauseAllSoundsByChannel(int channelId);
	void resumeAllSoundsByChannel(int channelId);

	void pauseAllSoundsByContext(int contextId);
	void resumeAllSoundsByContext(int contextId);

	void stopAllSounds(bool immediately = false);
	void stopAllSoundsByChannel(int channelId);
	void stopAllSoundsByContext(int contextId);
	void stopAllSoundsByChannelAndContext(int channelId, int contextId);

	void fadeInChannel(int channelId, float length);
	void fadeOutChannel(int channelId, float length);

	AudioSourceBase* findAudioSourceByRef(AudioReference& audioRef) const;
	float getAudioPlaybackPosition(AudioReference& audioRef) const;

	void resetChannelOverrides();

	void resetAudioModifiers();
	void enableAudioModifier(int channelId, int contextId, std::string_view postfix, float relativeSpeed);
	void disableAudioModifier(int channelId, int contextId);

	inline size_t getNumPlayingSounds() const  { return mPlayingSounds.size(); }
	size_t getMemoryUsage() const;

	void loadPlaybackState(const SavedPlaybackState& playbackState);
	void savePlaybackState(SavedPlaybackState& outPlaybackState) const;

private:
	struct PlayingSound
	{
		enum class State
		{
			NONE,
			PLAYING,
			OVERRIDDEN
		};

		uint32 mUniqueId = 0;
		AudioReference mAudioRef;
		uint64 mSfxId = 0;
		SourceRegistration* mSourceReg = nullptr;
		SourceRegistration* mBaseSourceReg = nullptr;	// Original non-modified source registration
		AudioSourceBase* mAudioSource = nullptr;

		float mBaseVolume = 1.0f;
		float mRelativeVolume = 1.0f;
		float mRelativeVolumeChange = 0.0f;
		int mContextId = -1;
		int mChannelId = -1;
		State mState = State::PLAYING;
	};

	struct AutoStreamer
	{
		AudioSourceBase* mAudioSource = nullptr;
		float mTime = 0.0f;
		float mSpeed = 1.0f;
	};

	struct ChannelOverride
	{
		int mPlayingChannelId = -1;
		int mOverriddenChannelId = -1;
		uint8 mContextId = 0;
		uint32 mPlayingSoundUniqueId = 0;
		bool mActive = true;
	};

	struct AudioModifier
	{
		int mContextId = -1;
		int mChannelId = -1;
		std::string mPostfix;
		float mRelativeSpeed = 1.0f;
	};

	struct SoundIterator
	{
	public:
		SoundIterator(std::vector<PlayingSound>& sounds);
		void filterContext(int contextId);
		void filterChannel(int channelId);
		void filterState(PlayingSound::State state);
		PlayingSound* getCurrent();
		PlayingSound* getNext();
		void removeCurrent();

	private:
		std::vector<PlayingSound>& mSounds;
		int mCurrentIndex = -1;
		int mFilterContextId = -1;
		int mFilterChannelId = -1;
		PlayingSound::State mFilterState = PlayingSound::State::NONE;
	};

private:
	PlayingSound* playAudioInternal(SourceRegistration* sourceReg, int contextId);
	PlayingSound* playAudioInternal(SourceRegistration* sourceReg, int channelId, int contextId);
	PlayingSound* startPlayback(SourceRegistration& sourceReg, float time, float volume, int contextId = -1, int channelId = -1);
	PlayingSound* startOrContinuePlayback(SourceRegistration& sourceReg, float volume, uint64 soundId, int contextId = -1, int channelId = -1);

	PlayingSound* startPlaybackInternal(SourceRegistration& sourceReg, AudioSourceBase& audioSource, float volume, float time, int contextId, int channelId);
	PlayingSound* getPlayingSound(uint32 uniqueId);
	PlayingSound* getPlayingSound(AudioReference& audioRef);
	const PlayingSound* getPlayingSound(AudioReference& audioRef) const;

	void applyChannelOverride(int overriddenChannelId, uint8 contextId);
	void removeChannelOverride(int overriddenChannelId, uint8 contextId);
	bool isChannelOverridden(int channelId, uint8 contextId) const;

	AudioModifier* findAudioModifier(int channelId, int contextId, int* outIndex = nullptr);
	SourceRegistration* getModifiedSourceRegistration(SourceRegistration& baseSourceReg, std::string_view postfix) const;
	void applyAudioModifier(int channelId, int contextId, std::string_view postfix, float relativeSpeed, float speedChange);
	void applyAudioModifierSingle(SoundIterator& iterator, std::string_view postfix, float relativeSpeed, float speedChange);

	void startAutoStreamer(AudioSourceBase& audioSource, float currentTime, float speed = 1.0f);
	void stopAutoStreamer(AudioSourceBase& audioSource);

private:
	AudioCollection& mAudioCollection;
	AudioSourceManager mAudioSourceManager;

	uint32 mLastUniqueId = 0;
	std::vector<PlayingSound> mPlayingSounds;
	std::vector<AutoStreamer> mAutoStreamers;
	std::vector<ChannelOverride> mChannelOverrides;
	std::vector<AudioModifier> mActiveAudioModifiers;

	uint32 mLastAudioTime = 0;		// Our own accumulated time, measured in audio samples -- this is meant to be roughly synced to the FTX audio manager's playback time
};
