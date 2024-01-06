/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


// Static list of loading callbacks
AudioBuffer::LoadCallbackList AudioBuffer::mStaticLoadCallbacks;


AudioBuffer::AudioBuffer()
{
}

AudioBuffer::~AudioBuffer()
{
	clearInternal();
}

void AudioBuffer::clear(int frequency, int channels)
{
	RMX_ASSERT(mMutexLockCounter > 0, "Audio buffer mutex should be locked in 'AudioBuffer::clear' method");
	clearInternal();
	mFrequency = clamp(frequency, 22050, 48000);
	mChannels = clamp(channels, 1, 2);
	mCompleted = false;
}

void AudioBuffer::addData(short** data, int length, int frequency, int channels)
{
	RMX_ASSERT(mMutexLockCounter > 0, "Audio buffer mutex should be locked in 'AudioBuffer::addData' method");
	if (nullptr == data || length <= 0)
		return;

/*	// TODO: Convert data if needed
	if (channels <= 0)
		channels = mDefaultChannels;
	if (frequency <= 0)
		frequency = mDefaultFrequency;
*/
	int offset = 0;
	while (offset < length)
	{
		// Get or create working frame
		AudioFrame& workingFrame = getWorkingFrame();

		// Copy data
		int len = std::min(length - offset, MAX_FRAME_LENGTH - workingFrame.mLength);
		for (int i = 0; i < mChannels; ++i)
		{
			short* src = &data[i][offset];
			short* dst = &workingFrame.mData[i][workingFrame.mLength];
			memcpy(dst, src, len * sizeof(short));
		}
		offset += len;
		workingFrame.mLength += len;
		mLength += len;
	}
}

void AudioBuffer::addData(float** data, int length, int frequency, int channels)
{
	RMX_ASSERT(mMutexLockCounter > 0, "Audio buffer mutex should be locked in 'AudioBuffer::addData' method");
	if (nullptr == data || length <= 0)
		return;

/*	// TODO: Convert data if needed
	if (channels <= 0)
		channels = mDefaultChannels;
	if (frequency <= 0)
		frequency = mDefaultFrequency;
*/
	int offset = 0;
	while (offset < length)
	{
		// Get or create working frame
		AudioFrame& workingFrame = getWorkingFrame();

		// Copy data
		const int len = std::min(length - offset, MAX_FRAME_LENGTH - workingFrame.mLength);
		for (int i = 0; i < mChannels; ++i)
		{
			const float* src = &data[i][offset];
			short* dst = &workingFrame.mData[i][workingFrame.mLength];
			for (int j = 0; j < len; ++j)
			{
				const int value = (int)(src[j] * 0x8000 + 0.5f);
				dst[j] = clamp(value, -0x8000, +0x7fff);
			}
		}
		offset += len;
		workingFrame.mLength += len;
		mLength += len;
	}
}

void AudioBuffer::markPurgeableSamples(int purgePosition)
{
	RMX_ASSERT(!mPersistent, "'AudioBuffer::markPurgeableSamples' is meant only for non-persistent audio buffers");
	RMX_ASSERT(mMutexLockCounter > 0, "Audio buffer mutex should be locked in 'AudioBuffer::markPurgeableSamples' method");
	int numFramesToPurge = purgePosition / MAX_FRAME_LENGTH;
	numFramesToPurge = clamp(numFramesToPurge, 0, mPurgedFrames + (int)mFrames.size() - 1);
	const int difference = numFramesToPurge - mPurgedFrames;

	// To avoid the expensive copying all the time, don't purge any frames unless it's at least 32 frames to be moved
	if (difference >= 32)
	{
		const int framesRemaining = (int)mFrames.size() - difference;
		for (int i = 0; i < framesRemaining; ++i)
		{
			mFrames[i] = mFrames[i + difference];
		}
		mFrames.resize(framesRemaining);
		mPurgedFrames = numFramesToPurge;
	}
}

bool AudioBuffer::load(const String& source, const String& params)
{
	// Load using the static loading callbacks
	clear();
	for (LoadCallbackList::iterator it = mStaticLoadCallbacks.begin(); it != mStaticLoadCallbacks.end(); ++it)
	{
		LoadCallbackType func = *it;
		if (func(this, source, params))
			return true;
	}
	return false;
}

float AudioBuffer::getLengthInSec() const
{
	return (float)mLength / (float)mFrequency;
}

size_t AudioBuffer::getMemoryUsage() const
{
	return mFrames.size() * MAX_FRAME_LENGTH * sizeof(short) * mChannels;
}

void AudioBuffer::setPersistent(bool persistent)
{
	mPersistent = persistent;
}

void AudioBuffer::setCompleted(bool completed)
{
	mCompleted = completed;
}

int AudioBuffer::getData(short** output, int position) const
{
	// Access audio data
	RMX_ASSERT(mMutexLockCounter > 0, "Audio buffer mutex should be locked in 'AudioBuffer::getData' method");
	output[0] = nullptr;
	output[1] = nullptr;
	if (position < 0)
		return 0;

	const int frameIndex = (position / MAX_FRAME_LENGTH) - mPurgedFrames;
	RMX_ASSERT(frameIndex >= 0 && frameIndex <= (int)mFrames.size(), "Invalid frame index " << frameIndex);  // Equality with frame size is okay and does not trigger the assert, but caught by the if-check
	if (frameIndex >= 0 && frameIndex < (int)mFrames.size())
	{
		const AudioFrame* frame = mFrames[frameIndex];
		if (nullptr != frame)
		{
			const int localPosition = position % MAX_FRAME_LENGTH;
			if (localPosition < frame->mLength)
			{
				output[0] = &frame->mData[0][localPosition];
				output[1] = &frame->mData[1][localPosition];
				return frame->mLength - localPosition;
			}
		}
	}
	return 0;
}

void AudioBuffer::lock()
{
	// Note: An SDL mutex can be locked multiple times by the same thread with no problems
	mMutex.lock();
	++mMutexLockCounter;
}

void AudioBuffer::unlock()
{
	--mMutexLockCounter;
	mMutex.unlock();
}

void AudioBuffer::clearInternal()
{
	// Clear all frames
	for (int i = 0; i < (int)mFrames.size(); ++i)
	{
		delete[] mFrames[i]->mBuffer;
		delete mFrames[i];
	}
	mFrames.clear();
	mPurgedFrames = 0;
	mLength = 0;
}

AudioBuffer::AudioFrame& AudioBuffer::getWorkingFrame()
{
	AudioFrame* workingFrame = mFrames.empty() ? nullptr : mFrames[mFrames.size()-1];
	if (nullptr == workingFrame || workingFrame->mLength >= MAX_FRAME_LENGTH)
	{
		workingFrame = new AudioFrame();
		workingFrame->mBuffer = new short[mChannels * MAX_FRAME_LENGTH];
		for (int i = 0; i < mChannels; ++i)
			workingFrame->mData[i] = &workingFrame->mBuffer[i * MAX_FRAME_LENGTH];
		workingFrame->mLength = 0;
		mFrames.push_back(workingFrame);
	}
	return *workingFrame;
}
