/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/ChipWritesAudioSource.h"
#include "oxygen/application/Configuration.h"

#if defined(PLATFORM_VITA) // For the emergency unloads
	#include "oxygen/application/audio/AudioOutBase.h"
	#include "oxygen/application/audio/AudioPlayer.h"
	#include "oxygen/application/EngineMain.h"
#endif


ChipWritesAudioSource::ChipWritesAudioSource(bool useCaching) :
	AudioSourceBase(AudioSourceType::CHIP_WRITES, useCaching ? CachingType::STREAMING_DYNAMIC : CachingType::STREAMING_STATIC)
{
	// Without caching, audio buffer content can be deleted as soon as it was played
	mAudioBuffer.setPersistent(!isDynamic());
}

ChipWritesAudioSource::~ChipWritesAudioSource()
{
}

bool ChipWritesAudioSource::load(std::wstring_view filename)
{
	if (filename.empty())
		return false;

	std::vector<uint8> content;
	if (!FTX::FileSystem->readFile(filename, content))
	{
		RMX_ERROR("Failed to load audio file '" << *WString(filename).toString() << "': File not found", );
		return false;
	}

	VectorBinarySerializer serializer(true, content);
	mSoundChipWritesByFrame.emplace_back();

	while (serializer.getRemaining() >= 4)
	{
		const uint32 input = serializer.read<uint32>();
		if (input == 0)
		{
			// Go to next frame
			mSoundChipWritesByFrame.emplace_back();
			continue;
		}

		const uint32 cycles = (input & 0x7fffffff) >> 17;
		const uint8 data = input & 0xff;
		if (input & 0x80000000)
		{
			SoundChipWrite& scw = vectorAdd(mSoundChipWritesByFrame.back());
			scw.mTarget = SoundChipWrite::Target::SN76489;
			scw.mData = data;
			scw.mCycles = cycles;
			scw.mFrameNumber = (uint16)(mSoundChipWritesByFrame.size() - 1);
		}
		else
		{
			const uint16 address = (input >> 8) & 0x1ff;

			SoundChipWrite& scw = vectorAdd(mSoundChipWritesByFrame.back());
			scw.mTarget = (address & 0x100) ? SoundChipWrite::Target::YAMAHA_FMII : SoundChipWrite::Target::YAMAHA_FMI;
			scw.mAddress = (uint8)address;
			scw.mData = data;
			scw.mCycles = cycles;
			scw.mFrameNumber = (uint16)(mSoundChipWritesByFrame.size() - 1);
		}
	}

	// Remove silence at the end
	while (!mSoundChipWritesByFrame.empty() && mSoundChipWritesByFrame.back().empty())
	{
		mSoundChipWritesByFrame.pop_back();
	}

	return true;
}

void ChipWritesAudioSource::resetInternal()
{
	mCurrentFrame = 0;
}

AudioSourceBase::State ChipWritesAudioSource::startupInternal()
{
	if (isJobRegistered())
	{
		FTX::JobManager->removeJob(*this);
	}

	SDL_LockMutex(mMutex);
	mAudioBuffer.lock();
	mAudioBuffer.clear(Configuration::instance().mAudio.mSampleRate, 2);
	mAudioBuffer.unlock();

	mSoundEmulation.init(Configuration::instance().mAudio.mSampleRate, 60.0);
	SDL_UnlockMutex(mMutex);

	return State::STREAMING;
}

void ChipWritesAudioSource::progressInternal(float precacheTime)
{
	mPrecacheTime = precacheTime;

	// Update job priority
	setJobPriority(mPrecacheTime - mAudioBuffer.getLengthInSec());

	if (Configuration::instance().mAudio.mUseAudioThreading)
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

bool ChipWritesAudioSource::jobFunc()
{
	// This method is executed by a worker thread
	SDL_LockMutex(mMutex);

	// Update in increments of around 2 ms per "jobFunc" call, but at least 25 ms for the first update
	//  -> The worker threads should update all audio sources in parallel (using relatively small increments), instead of updating one completely, then the next, etc.
	//  -> On the other hand, the very first update should at least cover one complete sample buffer size (usually 1024 samples, which is around 23 ms, at 44.1 kHz)
	const float targetTime = clamp(mPrecacheTime, 0.025f, mAudioBuffer.getLengthInSec() + 0.002f);
	while (mAudioBuffer.getLengthInSec() < targetTime && shouldJobBeRunning())
	{
		bool isPlaying = (mCurrentFrame < mSoundChipWritesByFrame.size());

		static std::vector<SoundChipWrite> EMPTY_LIST;
		const std::vector<SoundChipWrite>& writes = isPlaying ? mSoundChipWritesByFrame[mCurrentFrame] : EMPTY_LIST;
		++mCurrentFrame;

		static int16 soundBuffer[0x10000];
		const uint32 length = mSoundEmulation.update(soundBuffer, writes);	// Returns length in samples

		if (!isPlaying)
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
