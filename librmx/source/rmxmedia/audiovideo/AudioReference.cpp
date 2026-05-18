/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
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

bool AudioReference::isValid()
{
	updateInstance();
	return (nullptr != mInstance);
}

float AudioReference::getPosition()
{
	if (isValid())
		return (float)mInstance->mPosition / (float)mInstance->mAudioBuffer->getFrequency();
	return 0.0f;
}

float AudioReference::getVolume()
{
	if (isValid())
		return mInstance->mVolume;
	return 0.0f;
}

float AudioReference::getSpeed()
{
	if (isValid())
		return mInstance->mSpeed;
	return 0.0f;
}

bool AudioReference::isLooped()
{
	if (isValid())
		return mInstance->mLoop;
	return false;
}

bool AudioReference::isPaused()
{
	if (isValid())
		return mInstance->mPaused;
	return false;
}

bool AudioReference::isStreaming()
{
	if (isValid())
		return mInstance->mStreaming;
	return false;
}

void AudioReference::stop()
{
	if (isValid())
		FTX::Audio->removeSound(*this);
}

void AudioReference::setPosition(float position)
{
	if (isValid())
		mInstance->mPosition = (int)(position * mInstance->mAudioBuffer->getFrequency());
}

void AudioReference::setLoopStartInSamples(int loopStart)
{
	if (isValid())
		mInstance->mLoopStart = loopStart;
}

void AudioReference::setVolume(float volume)
{
	if (isValid())
		mInstance->mVolume = volume;
}

void AudioReference::setVolumeChange(float volumeChange)
{
	if (isValid())
		mInstance->mVolumeChange = volumeChange;
}

void AudioReference::setSpeed(float speed)
{
	if (isValid())
		mInstance->mSpeed = speed;
}

void AudioReference::setLoop(bool loop)
{
	if (isValid())
		mInstance->mLoop = loop;
}

void AudioReference::setPause(bool pause)
{
	if (isValid())
		mInstance->mPaused = pause;
}

void AudioReference::setStreaming(bool strm)
{
	if (isValid())
		mInstance->mStreaming = strm;
}

void AudioReference::setTimeout(float timeout)
{
	if (isValid())
		mInstance->mTimeout = (int)(timeout * mInstance->mAudioBuffer->getFrequency());
}

void AudioReference::setPanning(bool enable, float value)
{
	if (isValid())
	{
		mInstance->mUsePan = enable;
		mInstance->mPanning = value;
	}
}

void AudioReference::updateInstance()
{
	if (mChangeCounter != FTX::Audio->getChangeCounter())
	{
		mInstance = FTX::Audio->findInstance(mInstanceID);
		mChangeCounter = FTX::Audio->getChangeCounter();
	}
}
