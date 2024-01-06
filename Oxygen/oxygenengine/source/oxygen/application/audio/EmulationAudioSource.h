/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/audio/AudioSourceBase.h"
#include "oxygen/simulation/sound/SoundEmulation.h"
#include "oxygen/simulation/sound/SoundDriver.h"


class EmulationAudioSource : public AudioSourceBase, public rmx::JobBase
{
public:
	EmulationAudioSource(CachingType cachingType);
	~EmulationAudioSource();

	virtual bool isEmulationAudioSource() const override { return true; }

	bool initWithSfxId(uint8 soundId);
	bool initWithCustomAddress(uint8 soundId, uint32 sourceAddress);
	bool initWithCustomContent(uint8 soundId, const std::wstring& filename, uint32 contentOffset);

	void resetContent();
	void injectPlaySound(uint8 soundId);
	void injectTempoSpeedup(uint8 tempoSpeedup);

	virtual bool checkForUnload(float timestamp) override;

protected:
	virtual State startupInternal() override;
	virtual void progressInternal(float targetTime) override;

protected:
	virtual bool jobFunc() override;

private:
	uint8 mSoundId = 0;
	uint32 mSourceAddress = 0;				// Usually not used (i.e. stays zero), except if a different address should be used than the one associated with the sound ID
	std::wstring mFilename;					// Empty if using original ROM data
	std::vector<uint8> mCompressedContent;	// Empty if using original ROM data

	SoundEmulation mSoundEmulation;
	SoundDriver mSoundDriver;

	SDL_mutex* mMutex = nullptr;
	float mPrecacheTime = 0.0f;
};
