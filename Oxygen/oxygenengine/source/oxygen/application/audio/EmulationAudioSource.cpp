/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/EmulationAudioSource.h"
#include "oxygen/application/Configuration.h"


EmulationAudioSource::EmulationAudioSource(CachingType cachingType) :
	AudioSourceBase(cachingType)
{
	// Without caching, audio buffer content can be deleted as soon as it was played
	mAudioBuffer.setPersistent(!isDynamic());

	mMutex = SDL_CreateMutex();
}

EmulationAudioSource::~EmulationAudioSource()
{
	if (isJobRegistered())
	{
		FTX::JobManager->removeJob(*this);
	}
	SDL_DestroyMutex(mMutex);
}

bool EmulationAudioSource::initWithSfxId(uint8 soundId)
{
	mSoundId = soundId;
	return true;
}

bool EmulationAudioSource::initWithCustomAddress(uint8 soundId, uint32 sourceAddress)
{
	mSoundId = soundId;
	mSourceAddress = sourceAddress;
	mSoundDriver.setSourceAddress(sourceAddress);
	return true;
}

bool EmulationAudioSource::initWithCustomContent(uint8 soundId, const std::wstring& filename, uint32 contentOffset)
{
	mSoundId = soundId;
	mFilename = filename;

	if (!mFilename.empty())
	{
		if (!FTX::FileSystem->readFile(filename, mCompressedContent))
		{
			RMX_ERROR("Failed to load audio file '" << *WString(filename).toString() << "'", );
			return false;
		}
		mSoundDriver.setFixedContent(&mCompressedContent[0], (uint32)mCompressedContent.size(), contentOffset);
	}
	return true;
}

void EmulationAudioSource::resetContent()
{
	if (isJobRegistered())
	{
		FTX::JobManager->removeJob(*this);
	}

	mState = State::INACTIVE;	// This will also lead to audio buffer getting cleared
}

void EmulationAudioSource::injectPlaySound(uint8 soundId)
{
	// Note: This should only be called for dynamic sounds
	SDL_LockMutex(mMutex);
	mSoundDriver.playSound(soundId);
	mState = State::STREAMING;
	SDL_UnlockMutex(mMutex);
}

void EmulationAudioSource::injectTempoSpeedup(uint8 tempoSpeedup)
{
	SDL_LockMutex(mMutex);
	mSoundDriver.setTempoSpeedup(tempoSpeedup);
	SDL_UnlockMutex(mMutex);
}

bool EmulationAudioSource::checkForUnload(float timestamp)
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
		mSoundDriver.reset();

		SDL_UnlockMutex(mMutex);
		return true;
	}
	return false;
}

AudioSourceBase::State EmulationAudioSource::startupInternal()
{
	if (isJobRegistered())
	{
		FTX::JobManager->removeJob(*this);
	}

	SDL_LockMutex(mMutex);
	mAudioBuffer.lock();
	mAudioBuffer.clear(Configuration::instance().mAudioSampleRate, 2);
	mAudioBuffer.unlock();

	mSoundEmulation.init(Configuration::instance().mAudioSampleRate, 60.0);
	mSoundDriver.reset();
	mSoundDriver.playSound(mSoundId);
	SDL_UnlockMutex(mMutex);

	return State::STREAMING;
}

void EmulationAudioSource::progressInternal(float precacheTime)
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

bool EmulationAudioSource::jobFunc()
{
	// This method is executed by a worker thread
	SDL_LockMutex(mMutex);

	// Update in increments of around 2 ms per "jobFunc" call, but at least 25 ms for the first update
	//  -> The worker threads should update all audio sources in parallel (using relatively small increments), instead of updating one completely, then the next, etc.
	//  -> On the other hand, the very first update should at least cover one complete sample buffer size (usually 1024 samples, which is around 23 ms, at 44.1 kHz)
	const float targetTime = clamp(mPrecacheTime, 0.025f, mAudioBuffer.getLengthInSec() + 0.002f);
	while (mAudioBuffer.getLengthInSec() < targetTime && shouldJobBeRunning())
	{
		const SoundDriver::UpdateResult updateResult = mSoundDriver.update();
		const std::vector<SoundChipWrite>& writes = mSoundDriver.getSoundChipWrites();
		bool isPlaying = (updateResult == SoundDriver::UpdateResult::CONTINUE);

		static int16 soundBuffer[0x10000];
		const uint32 length = mSoundEmulation.update(soundBuffer, writes);	// Returns length in samples

		if (updateResult == SoundDriver::UpdateResult::FINISHED)
		{
			// Check if sound chips still produce output
			for (uint32 i = 0; i < length * 2; ++i)
			{
				if (soundBuffer[i] < -2 || soundBuffer[i] > 0)	// Sometimes we get -2 indefinitely (e.g. sound ID "CC" does this)
				{
					isPlaying = true;
					break;
				}
			}
		}

		if (isPlaying)
		{
			static int16 pcm[2][0x10000];
			int16* pcmPtr[2] = { pcm[0], pcm[1] };

			for (uint32 i = 0; i < length; ++i)
			{
				pcm[0][i] = soundBuffer[i*2];
				pcm[1][i] = soundBuffer[i*2+1];
			}
			mAudioBuffer.lock();
			mAudioBuffer.addData(pcmPtr, length);
			mAudioBuffer.unlock();
		}
		else
		{
			mAudioBuffer.setCompleted();
			mState = State::COMPLETED;
			SDL_UnlockMutex(mMutex);

			// Job completed
			return true;
		}
	}

	// Update job priority
	setJobPriority(mPrecacheTime - mAudioBuffer.getLengthInSec());
	SDL_UnlockMutex(mMutex);

	// Keep going with this job, i.e. this method will get called again
	return false;
}
