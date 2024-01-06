/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


void AudioReference::setInstanceID(int ID)
{
	mInstanceID = ID;
	mInstance = nullptr;
	mChangeCounter = -1;
}

void AudioReference::updateInstance()
{
	if (mChangeCounter != FTX::Audio->getChangeCounter())
	{
		mInstance = FTX::Audio->findInstance(mInstanceID);
		mChangeCounter = FTX::Audio->getChangeCounter();
	}
}

bool AudioReference::valid()
{
	updateInstance();
	return (nullptr != mInstance);
}

float AudioReference::getPosition()
{
	if (valid())
		return (float)mInstance->mPosition / (float)mInstance->mAudioBuffer->getFrequency();
	return 0.0f;
}

float AudioReference::getVolume()
{
	if (valid())
		return mInstance->mVolume;
	return 0.0f;
}

float AudioReference::getSpeed()
{
	if (valid())
		return mInstance->mSpeed;
	return 0.0f;
}

bool AudioReference::isLooped()
{
	if (valid())
		return mInstance->mLoop;
	return false;
}

bool AudioReference::isPaused()
{
	if (valid())
		return mInstance->mPaused;
	return false;
}

bool AudioReference::isStreaming()
{
	if (valid())
		return mInstance->mStreaming;
	return false;
}

void AudioReference::stop()
{
	if (valid())
		FTX::Audio->removeSound(*this);
}

void AudioReference::setPosition(float position)
{
	if (valid())
		mInstance->mPosition = (int)(position * mInstance->mAudioBuffer->getFrequency());
}

void AudioReference::setLoopStartInSamples(int loopStart)
{
	if (valid())
		mInstance->mLoopStart = loopStart;
}

void AudioReference::setVolume(float volume)
{
	if (valid())
		mInstance->mVolume = volume;
}

void AudioReference::setVolumeChange(float volumeChange)
{
	if (valid())
		mInstance->mVolumeChange = volumeChange;
}

void AudioReference::setSpeed(float speed)
{
	if (valid())
		mInstance->mSpeed = speed;
}

void AudioReference::setLoop(bool loop)
{
	if (valid())
		mInstance->mLoop = loop;
}

void AudioReference::setPause(bool pause)
{
	if (valid())
		mInstance->mPaused = pause;
}

void AudioReference::setStreaming(bool strm)
{
	if (valid())
		mInstance->mStreaming = strm;
}

void AudioReference::setTimeout(float timeout)
{
	if (valid())
		mInstance->mTimeout = (int)(timeout * mInstance->mAudioBuffer->getFrequency());
}

void AudioReference::setPanning(bool enable, float value)
{
	if (valid())
	{
		mInstance->mUsePan = enable;
		mInstance->mPanning = value;
	}
}
