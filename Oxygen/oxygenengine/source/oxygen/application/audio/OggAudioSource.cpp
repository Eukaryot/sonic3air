/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/OggAudioSource.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"


OggAudioSource::OggAudioSource(bool useCaching, bool isLooping, int loopStart) :
	AudioSourceBase(useCaching ? CachingType::STREAMING_STATIC : CachingType::STREAMING_DYNAMIC),
	mIsLooping(isLooping),
	mLoopStart(loopStart)
{
	// Without caching, audio buffer content can be deleted as soon as it was played
	mAudioBuffer.setPersistent(useCaching);

	mMutex = SDL_CreateMutex();
}

OggAudioSource::~OggAudioSource()
{
	if (isJobRegistered())
	{
		FTX::JobManager->removeJob(*this);
	}
	SDL_DestroyMutex(mMutex);

	SAFE_DELETE(mOggLoader);
	SAFE_DELETE(mInputStream);
}

bool OggAudioSource::load(const std::wstring& filename)
{
	mFilename = filename;
	mInputStream = FTX::FileSystem->createInputStream(filename);
	if (nullptr == mInputStream)
	{
		RMX_ERROR("Failed to load audio file '" << *WString(filename).toString() << "'", );
		return false;
	}
	return true;
}

void OggAudioSource::onPlaybackStart(AudioReference& audioRef, float time)
{
	SDL_LockMutex(mMutex);

	// Configure looping behavior of audio playback
	if (!isDynamic())
	{
		// Using caching, but with or without looping?
		if (mIsLooping)
		{
			audioRef.setLoop(true);
			audioRef.setLoopStartInSamples(mLoopStart);

			// Perform seeking if needed
			//  -> This requires the audio buffer to be filled up to the requested time already
			if (time > 0.0f)
			{
				audioRef.setPosition(time);
			}
		}
		else
		{
			audioRef.setLoop(false);
		}
	}
	else
	{
		// Looping has to be done by seeking in the stream
		audioRef.setLoop(false);

		// Always start with an empty audio buffer, it will get filled now
		mAudioBuffer.lock();
		mAudioBuffer.clear(mAudioBuffer.getFrequency());
		mAudioBuffer.unlock();
		mReadTime = 0.0f;

		// Perform seeking if needed
		if (time >= 0.0f)
		{
			mOggLoader->seek(time);
			mInitialSeekPos = roundToInt(time * mAudioBuffer.getFrequency());
		}
	}

	SDL_UnlockMutex(mMutex);
}

bool OggAudioSource::checkForUnload(float timestamp)
{
	bool mayUnload = false;

	if (isDynamic())
	{
		// Ignore tracks not loaded
		if (mAudioBuffer.getLengthInSec() > 0.2f)
		{
			// Unload after a few seconds already
			mayUnload = (timestamp - mLastUsedTimestamp > 10.0f);
		}
	}
	else
	{
		// Ignore short sounds, and tracks not loaded
		if (mAudioBuffer.getLengthInSec() > 5.0f)
		{
			// Unload after 3 minutes
			mayUnload = (timestamp - mLastUsedTimestamp > 180.0f);
		}
	}

	if (mayUnload)
	{
		if (isJobRegistered())
		{
			FTX::JobManager->removeJob(*this);
		}

		SDL_LockMutex(mMutex);
		mAudioBuffer.lock();
		mAudioBuffer.clear();
		mAudioBuffer.unlock();
		mState = State::INACTIVE;
		mReadTime = 0.0f;

		SAFE_DELETE(mOggLoader);
		if (nullptr != mInputStream)
		{
			mInputStream->rewind();
		}

		SDL_UnlockMutex(mMutex);
		return true;
	}
	return false;
}

float OggAudioSource::mapAudioRefPositionToTrackPosition(float audioRefPosition) const
{
	if (!isDynamic())
	{
		// For static caching, i.e. fully cached tracks, there's no difference between audio ref position and absolute position inside the track
		return audioRefPosition;
	}
	
	// Account for the difference in audio ref position (i.e. position inside the audio buffer) and absolute position inside the audio track
	const float frequency = (float)mAudioBuffer.getFrequency();
	int trackPosition = mInitialSeekPos + roundToInt(audioRefPosition * frequency);

	// If track length is not known, we certainly did not loop yet, and it's very unlikely to get an input value after the track length
	const bool afterFirstLoop = (mTrackLength > 0 && trackPosition >= mTrackLength);
	if (afterFirstLoop)
	{
		// Otherwise normalize track position into the looping range
		const int loopingPartLength = mTrackLength - mLoopStart;
		if (loopingPartLength > 0)
		{
			trackPosition = mLoopStart + (trackPosition - mLoopStart) % loopingPartLength;
		}
	}
	return (float)trackPosition / frequency;
}

AudioSourceBase::State OggAudioSource::startupInternal()
{
	if (nullptr == mInputStream)
	{
		// This can only mean that creating the input stream in "load" had failed
		return State::COMPLETED;
	}

	if (isJobRegistered())
	{
		FTX::JobManager->removeJob(*this);
	}

	// Setup the Ogg loader
	SDL_LockMutex(mMutex);
	mInputStream->rewind();
	if (nullptr == mOggLoader)
	{
		mOggLoader = new OggLoader();
	}
	mAudioBuffer.lock();
	const bool success = mOggLoader->startVorbisStreaming(&mAudioBuffer, mInputStream);
	mAudioBuffer.unlock();
	SDL_UnlockMutex(mMutex);

	return success ? State::STREAMING : State::COMPLETED;
}

void OggAudioSource::progressInternal(float precacheTime)
{
	mPrecacheTime = precacheTime;

	// Update job priority
	setJobPriority(mPrecacheTime - mAudioBuffer.getLengthInSec());

	if (Configuration::instance().mUseAudioThreading)
	{
		// Add to job manager if not done yet
		if (!isJobRegistered())
		{
			FTX::JobManager->insertJob(*this);
		}
	}
	else
	{
		while (getJobPriority() > 0.001f)
		{
			if (callJobFuncOnCallingThread())
				break;
		}
	}
}

bool OggAudioSource::jobFunc()
{
	// This method is executed by a worker thread
	SDL_LockMutex(mMutex);
	RMX_CHECK(nullptr != mOggLoader, "No ogg loader instance found", return true);

	// Update in increments of around 2 ms per "jobFunc" call, but at least 25 ms for the first update
	//  -> The worker threads should update all audio sources in parallel (using relatively small increments), instead of updating one completely, then the next, etc.
	//  -> On the other hand, the very first update should at least cover one complete sample buffer size (usually 1024 samples, which is around 23 ms, at 44.1 kHz)
	const float targetTime = clamp(mPrecacheTime, 0.025f, mAudioBuffer.getLengthInSec() + 0.002f);
	updateStreaming(targetTime);

	// Reached the end of the input?
	if (!mOggLoader->isStreaming() && shouldJobBeRunning())
	{
		if (!isDynamic() || mLoopStart == -1)
		{
			// All audio data is now cached, we're done here
			mState = State::COMPLETED;
			SAFE_DELETE(mOggLoader);
			SDL_UnlockMutex(mMutex);

			// Job completed
			return true;
		}
		else
		{
			// We now know where the end of the track actually is
			if (mTrackLength < 0)
			{
				mTrackLength = mInitialSeekPos + mAudioBuffer.getLength();
			}

			// Seek back
			mAudioBuffer.setCompleted(false);
			const float loopStartSeconds = (float)mLoopStart / (float)mAudioBuffer.getFrequency();
			mOggLoader->seek(loopStartSeconds);
			updateStreaming(targetTime);
		}
	}

	// Update job priority
	setJobPriority(mPrecacheTime - mAudioBuffer.getLengthInSec());
	SDL_UnlockMutex(mMutex);

	// Keep going with this job, i.e. this method will get called again
	return false;
}

void OggAudioSource::updateStreaming(float targetTime)
{
	mAudioBuffer.lock();
	while (mAudioBuffer.getLengthInSec() < targetTime)
	{
		if (!mOggLoader->updateStreaming())
			break;
	}
	mAudioBuffer.unlock();
}
