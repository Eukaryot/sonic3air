/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/audio/AudioSourceBase.h"


class OggAudioSource : public AudioSourceBase, public rmx::JobBase
{
public:
	OggAudioSource(bool useCaching, bool isLooping, int loopStart);
	~OggAudioSource();

	bool load(const std::wstring& filename);

	virtual void onPlaybackStart(AudioReference& audioRef, float time) override;
	virtual bool checkForUnload(float timestamp) override;

	virtual float mapAudioRefPositionToTrackPosition(float audioRefPosition) const override;

protected:
	virtual State startupInternal() override;
	virtual void progressInternal(float targetTime) override;

protected:
	virtual bool jobFunc() override;

private:
	void updateStreaming(float targetTime);

private:
	std::wstring mFilename;
	InputStream* mInputStream = nullptr;
	OggLoader* mOggLoader = nullptr;

	bool mIsLooping = false;
	int mLoopStart = -1;		// In samples

	int mTrackLength = -1;		// In samples - but it's only set if looped back at least once
	int mInitialSeekPos = 0;	// In samples, start position of initial seek (zero if playback started at the beginning)

	SDL_mutex* mMutex = nullptr;
	float mPrecacheTime = 0.0f;
};
