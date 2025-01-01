/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	AudioReference
*		Helper class for accessing a playing music / sound.
*/

#pragma once

#include "rmxmedia/audiovideo/AudioManager.h"


class API_EXPORT AudioReference
{
public:
	inline int getInstanceID() const  { return mInstanceID; }
	void setInstanceID(int ID);

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
	int mInstanceID = 0;
	rmx::AudioManager::AudioInstance* mInstance = nullptr;
	int mChangeCounter = -1;
};
