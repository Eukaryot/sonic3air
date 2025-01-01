/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class AudioSourceBase
{
public:
	enum class CachingType
	{
		STREAMING_STATIC,	// Static sound, it's the same on each playback, and hence can be cached (e.g. normal music and most sound effects)
		STREAMING_DYNAMIC,	// Dynamic sound, i.e. can be different on each playback, but no quick reactions to gameplay events are needed (e.g. fast music tracks)
		FULL_DYNAMIC		// Dynamic sound, i.e. can be different on each playback, quick reactions to gameplay are needed (e.g. continuous looping sounds, or spindash sound with reverb)
	};

public:
	AudioSourceBase(CachingType cachingType) : mCachingType(cachingType) {}
	virtual ~AudioSourceBase() {}

	virtual bool isEmulationAudioSource() const	 { return false; }

	inline bool isDynamic() const			{ return (mCachingType != CachingType::STREAMING_STATIC); }
	inline bool needsMinimalLag() const		{ return (mCachingType == CachingType::FULL_DYNAMIC); }
	inline bool isStreaming() const			{ return (mState == State::STREAMING); }
	inline bool isCompletelyLoaded() const	{ return (mState == State::COMPLETED); }

	virtual void onPlaybackStart(AudioReference& audioRef, float time)  {}
	virtual void onPlaybackStop()  {}

	inline float getReadTime() const			{ return mReadTime; }
	inline void updateReadTime(float readTime)	{ mReadTime = std::max(mReadTime, readTime); }

	AudioBuffer* startup(float precacheTime);
	void progress(float precacheTime);

	void setLastUsedTimestamp(float timestamp)	  { mLastUsedTimestamp = timestamp; }
	virtual bool checkForUnload(float timestamp)  { return false; }

	virtual float mapAudioRefPositionToTrackPosition(float audioRefPosition) const  { return audioRefPosition; }

	inline size_t getMemoryUsage() const  { return mAudioBuffer.getMemoryUsage(); }

protected:
	enum class State
	{
		INACTIVE,	// No playback has started yet
		STREAMING,	// Currently streaming (but it may be paused)
		COMPLETED,	// Streaming completely done
	};

protected:
	virtual State startupInternal() = 0;
	virtual void progressInternal(float targetTime) = 0;

protected:
	AudioBuffer mAudioBuffer;
	CachingType mCachingType = CachingType::STREAMING_STATIC;
	State mState = State::INACTIVE;
	float mReadTime = 0.0f;
	float mLastUsedTimestamp = 0.0f;
};
