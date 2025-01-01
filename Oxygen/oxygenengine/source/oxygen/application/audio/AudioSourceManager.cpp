/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/AudioSourceManager.h"
#include "oxygen/application/audio/EmulationAudioSource.h"
#include "oxygen/application/audio/OggAudioSource.h"


void AudioSourceManager::clear()
{
	// Before destroying the audio sources (incl. audio buffers), make sure no sound is playing any more
	FTX::Audio->removeAllSounds();

	// Now it's safe to destroy all the audio sources
	for (AudioSourceBase* audioSource : mAudioSources)
	{
		delete audioSource;
	}
	mAudioSources.clear();
	mMappedAudioSourcesByHash.clear();
}

AudioSourceBase* AudioSourceManager::getAudioSourceForPlayback(SourceRegistration& sourceRegistration)
{
	// Is the audio source pointer already cached, then just use it
	if (nullptr != sourceRegistration.mAudioSource)
	{
		return sourceRegistration.mAudioSource;
	}

	// Determine a hash
	uint64 hash = 0;
	if (sourceRegistration.mType == SourceRegistration::Type::FILE)
	{
		hash = rmx::getMurmur2_64((String("OggFile:") + WString(sourceRegistration.mSourceFile).toUTF8()));
	}
	else if (!sourceRegistration.mSourceFile.empty())
	{
		hash = rmx::getMurmur2_64((String("SMPSFile:") + WString(sourceRegistration.mSourceFile).toUTF8()));
	}
	else if (sourceRegistration.mSourceAddress != 0)
	{
		hash = rmx::getMurmur2_64(String(0, "EmulatedSource:%08x", sourceRegistration.mSourceAddress));
	}
	else
	{
		hash = rmx::getMurmur2_64(String(0, "EmulatedKey:%016llx", sourceRegistration.mAudioDefinition->mKeyId));
	}

	AudioSourceBase* audioSource = nullptr;

	// Is there an entry already with that hash?
	//  -> By checking this, we can easily re-use already loaded audio sources after changes to the audio collection, e.g. after a full mods reload
	const auto it = mMappedAudioSourcesByHash.find(hash);
	if (it != mMappedAudioSourcesByHash.end())
	{
		// Found it
		audioSource = it->second;
	}
	else
	{
		// Create a new audio source
		if (sourceRegistration.mType == SourceRegistration::Type::FILE)
		{
			const bool useCaching = !String(sourceRegistration.mAudioDefinition->mKeyString).endsWith("_fast");
			audioSource = addOggAudioSource(sourceRegistration.mSourceFile, useCaching, sourceRegistration.mIsLooping, sourceRegistration.mLoopStart);
		}
		else
		{
			const AudioSourceBase::CachingType cachingType = (sourceRegistration.mType == SourceRegistration::Type::EMULATION_BUFFERED) ? AudioSourceBase::CachingType::STREAMING_STATIC :
															 (sourceRegistration.mType == SourceRegistration::Type::EMULATION_DIRECT) ? AudioSourceBase::CachingType::STREAMING_DYNAMIC : AudioSourceBase::CachingType::FULL_DYNAMIC;
			audioSource = addEmulationAudioSource(sourceRegistration.mEmulationSfxId, cachingType, sourceRegistration.mSourceFile, sourceRegistration.mSourceAddress, sourceRegistration.mContentOffset);
		}

		mMappedAudioSourcesByHash[hash] = audioSource;
	}

	sourceRegistration.mAudioSource = audioSource;
	return audioSource;
}

void AudioSourceManager::updateStreaming(float currentTime)
{
	for (AudioSourceBase* audioSource : mAudioSources)
	{
		// Check if this audio source should get unloaded now
		if (!audioSource->checkForUnload(currentTime))
		{
			// Otherwise update streaming
			if (audioSource->isStreaming())
			{
				const float precacheTime = audioSource->needsMinimalLag() ? 0.1f : 0.25f;
				audioSource->progress(audioSource->getReadTime() + precacheTime);
			}
		}
	}
}

size_t AudioSourceManager::getMemoryUsage() const
{
	size_t memoryUsage = 0;
	for (const AudioSourceBase* audioSource : mAudioSources)
	{
		memoryUsage += audioSource->getMemoryUsage();
	}
	return memoryUsage;
}

AudioSourceBase* AudioSourceManager::addEmulationAudioSource(uint8 soundId, AudioSourceBase::CachingType cachingType, const std::wstring& filename, uint32 sourceAddress, uint32 contentOffset)
{
	// Register audio source
	EmulationAudioSource* audioSource = new EmulationAudioSource(cachingType);
	mAudioSources.push_back(audioSource);

	// Initialize and load content from file if needed
	if (!filename.empty())
	{
		audioSource->initWithCustomContent(soundId, filename, contentOffset);
	}
	else if (sourceAddress != 0)
	{
		audioSource->initWithCustomAddress(soundId, sourceAddress);
	}
	else
	{
		audioSource->initWithSfxId(soundId);
	}
	return audioSource;
}

AudioSourceBase* AudioSourceManager::addOggAudioSource(const std::wstring& filename, bool useCaching, bool isLooping, int loopStart)
{
	// Register audio source
	OggAudioSource* audioSource = new OggAudioSource(useCaching, isLooping, loopStart);
	mAudioSources.push_back(audioSource);

	// Load content
	audioSource->load(filename);
	return audioSource;
}
