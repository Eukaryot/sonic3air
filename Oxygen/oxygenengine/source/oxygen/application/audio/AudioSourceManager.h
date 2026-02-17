/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/audio/AudioCollection.h"
#include "oxygen/application/audio/AudioSourceBase.h"


class AudioSourceManager
{
public:
	using SourceRegistration = AudioCollection::SourceRegistration;

public:
	void clear();

	AudioSourceBase* getAudioSourceForPlayback(SourceRegistration& sourceRegistration);

	void updateStreaming(float currentTime);

	size_t getMemoryUsage() const;

private:
	AudioSourceBase* addEmulationAudioSource(uint8 soundId, AudioSourceBase::CachingType cachingType, const std::wstring& filename = L"", uint32 sourceAddress = 0, uint32 contentOffset = 0);
	AudioSourceBase* addOggAudioSource(const std::wstring& filename, bool useCaching = true, bool isLooping = false, int loopStart = -1);

private:
	std::vector<AudioSourceBase*> mAudioSources;
	std::map<uint64, AudioSourceBase*> mMappedAudioSourcesByHash;
};
