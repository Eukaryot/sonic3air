/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	AudioReference
*		Helper class for accessing a playing music / sound.
*/

#pragma once

#include "rmxmedia/AudioManager.h"


class API_EXPORT AudioReference
{
public:
	AudioReference();
	void initialize(int ID);

	int getInstanceID() const  { return mInstanceID; }

	bool valid();

	float getPosition();
	float getVolume();
	float getSpeed();
	bool isLooped();
	bool isPaused();
	bool isStreaming();

	void stop();
	void setPosition(float position);
	void setLoopStartInSamples(int loopStart);
	void setVolume(float volume);
	void setVolumeChange(float volumeChange);
	void setSpeed(float speed);
	void setLoop(bool loop);
	void setPause(bool pause);
	void setStreaming(bool strm);
	void setTimeout(float timeout);
	void setPanning(bool enable, float value = 0.0f);

private:
	void updateInstance();

private:
	int mInstanceID;
	rmx::AudioManager::AudioInstance* mInstance;
	int mChangeCounter;
};
