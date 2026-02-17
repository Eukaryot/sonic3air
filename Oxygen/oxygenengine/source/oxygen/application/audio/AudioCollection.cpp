/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/AudioCollection.h"
#include "oxygen/helper/JsonHelper.h"


namespace
{
	bool isHexDigit(char ch)
	{
		return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
	}

	int compareSourceRegistrationPackages(AudioCollection::Package a, AudioCollection::Package b, bool preferOriginalSoundtrack)
	{
		static_assert((int)AudioCollection::Package::_NUM == 4);
		const int prioritiesA[4] = { 0, 1, 2, 3 };		// Preferring remastered over original, but modded will always be first
		const int prioritiesB[4] = { 0, 2, 1, 3 };		// Preferring original over remastered, but modded will always be first
		const int* priorities = preferOriginalSoundtrack ? prioritiesB : prioritiesA;

		const int prioA = priorities[(int)a];
		const int prioB = priorities[(int)b];
		return (prioA == prioB) ? 0 : (prioA < prioB) ? 1 : -1;
	}

	bool shouldPreferSoundRegistration(AudioCollection::SourceRegistration& soundRegToCheck, AudioCollection::SourceRegistration* bestFoundSoFar, bool preferOriginalSoundtrack)
	{
		if (nullptr == bestFoundSoFar)
			return true;

		// Using <= instead of < here, so that with multiple modded sources, the last one will get used - that's the one with highest priority
		return (compareSourceRegistrationPackages(soundRegToCheck.mPackage, bestFoundSoFar->mPackage, preferOriginalSoundtrack) <= 0);
	}

	bool getHexCodeRetranslation(uint64& outKey, uint64 hexCodeString)
	{
		static std::map<uint64, uint64> retranslation;
		if (retranslation.empty())
		{
			for (uint64 i = 0; i < 0x100; ++i)
			{
				retranslation[rmx::getMurmur2_64(String(0, "%02x", (uint32)i))] = i;
				retranslation[rmx::getMurmur2_64(String(0, "%02X", (uint32)i))] = i;
			}
		}

		const auto it2 = retranslation.find(hexCodeString);
		if (it2 != retranslation.end())
		{
			outKey = it2->second;
			return true;
		}
		return false;
	}
}


AudioCollection::AudioCollection()
{
}

AudioCollection::~AudioCollection()
{
}

void AudioCollection::clear()
{
	mAudioDefinitions.clear();
	for (size_t i = 0; i < (size_t)Package::_NUM; ++i)
		mNumSourcesByPackageType[i] = 0;
	++mChangeCounter;
}

void AudioCollection::clearPackage(Package package)
{
	for (auto it = mAudioDefinitions.begin(); it != mAudioDefinitions.end(); )
	{
		auto& sources = it->second.mSources;
		for (size_t k = 0; k < sources.size(); ++k)
		{
			if (sources[k].mPackage == package)
			{
				sources.erase(sources.begin() + k);
				--k;
			}
		}

		if (sources.empty())
		{
			it = mAudioDefinitions.erase(it);
		}
		else
		{
			++it;
		}
	}
	mNumSourcesByPackageType[(size_t)package] = 0;
	++mChangeCounter;
}

bool AudioCollection::loadFromJson(const std::wstring& basepath, const std::wstring& filename, Package package)
{
	const Json::Value jsonRoot = JsonHelper::loadFile(basepath + L'/' + filename);
	if (jsonRoot.empty())
		return false;

	for (auto iterator = jsonRoot.begin(); iterator != jsonRoot.end(); ++iterator)
	{
		String keyString = iterator.key().asString();
		keyString.lowerCase();

		// Numeric key is either a string hash, or the value in case of keys like "2C"
		uint64 numericKey = 0;
		{
			if (keyString.length() == 2 && isHexDigit(keyString[0]) && isHexDigit(keyString[1]))
			{
				numericKey = rmx::parseInteger(String("0x") + keyString);
			}
			else
			{
				numericKey = rmx::getMurmur2_64(keyString);
			}
		}

		// Read definition from JSON
		AudioDefinition::Type type = AudioDefinition::Type::SOUND;
		std::string displayName;
		WString audioFilename;
		uint32 sourceAddress = 0;
		uint32 contentOffset = 0;
		uint8 emulationSfxId = (numericKey <= 0xff) ? (uint8)numericKey : 0;
		SourceRegistration::Type sourceType = SourceRegistration::Type::FILE;
		int loopStart = 0;
		float volume = 1.0f;
		uint8 channel = (numericKey < 0xff) ? (uint8)numericKey : 0xff;
		AudioDefinition::Visibility soundTestVisibility = AudioDefinition::Visibility::AUTO;

		for (auto it = iterator->begin(); it != iterator->end(); ++it)
		{
			const std::string key = it.key().asString();
			const std::string value = it->asString();

			if (key == "Name")
			{
				displayName = value;
			}
			else if (key == "Type")
			{
				if (value == "Music")
				{
					type = AudioDefinition::Type::MUSIC;
				}
				else if (value == "Jingle")
				{
					type = AudioDefinition::Type::JINGLE;
				}
				else if (value == "Sound")
				{
					type = AudioDefinition::Type::SOUND;
				}
				else
				{
					RMX_ERROR("Invalid audio definition type: " << value, continue);
				}
			}
			else if (key == "File" && !value.empty())
			{
			#if 0
				// TEST: Enforce emulation
				sourceType = SourceRegistration::Type::EMULATION_DIRECT;
			#else
				audioFilename = WString(basepath) + L'/' + String(value).toWString();
			#endif
			}
			else if (key == "Source" && !value.empty())
			{
				sourceType = (value == "EmulationContinuous") ? SourceRegistration::Type::EMULATION_CONTINUOUS :
							 (value == "EmulationDirect") ? SourceRegistration::Type::EMULATION_DIRECT : SourceRegistration::Type::EMULATION_BUFFERED;
			}
			else if (key == "Address" && !value.empty())
			{
				sourceAddress = (uint32)rmx::parseInteger(value);
			}
			else if (key == "ContentOffset" && !value.empty())
			{
				contentOffset = (uint32)rmx::parseInteger(value);
			}
			else if (key == "EmulatedID" && !value.empty())
			{
				emulationSfxId = (uint8)rmx::parseInteger(value);
			}
			else if (key == "Channel" && !value.empty())
			{
				if (value == "multiple")
					channel = 0xff;
				else
					channel = (uint8)rmx::parseInteger("0x" + value);
			}
			else if (key == "LoopStart" && !value.empty())
			{
				loopStart = (uint32)rmx::parseInteger(value);
			}
			else if (key == "Volume" && !value.empty())
			{
				volume = String(value).parseFloat();
			}
			else if (key == "SoundTestVisibility" && !value.empty())
			{
				soundTestVisibility = (value == "visible") ? AudioDefinition::Visibility::ALWAYS_VISIBLE
									: (value == "hidden")  ? AudioDefinition::Visibility::ALWAYS_HIDDEN
									: (value == "devmode") ? AudioDefinition::Visibility::DEV_MODE_ONLY : AudioDefinition::Visibility::AUTO;
			}
		}

		AudioDefinition* audioDefinition = mapFind(mAudioDefinitions, numericKey);
		if (nullptr == audioDefinition)
		{
			audioDefinition = &mAudioDefinitions[numericKey];
			audioDefinition->mKeyId = numericKey;
			audioDefinition->mKeyString = *keyString;
			audioDefinition->mType = type;

			// Music and jingles always use channel 0 -- no matter what is edited
			if (type == AudioDefinition::Type::MUSIC || type == AudioDefinition::Type::JINGLE)
			{
				audioDefinition->mChannel = 0;
			}
			else
			{
				audioDefinition->mChannel = channel;
			}
		}
		else
		{
			// Definition already exists, ignore the properties that are not specifying the source
		}

		// Set or overwrite values in audio definition
		if (!displayName.empty())
			audioDefinition->mDisplayName = displayName;
		if (soundTestVisibility != AudioDefinition::Visibility::AUTO)
			audioDefinition->mSoundTestVisibility = soundTestVisibility;

		// Add audio source
		SourceRegistration& sourceRegistration = vectorAdd(audioDefinition->mSources);
		sourceRegistration.mAudioDefinition = audioDefinition;
		sourceRegistration.mPackage = package;
		sourceRegistration.mType = sourceType;
		sourceRegistration.mIsLooping = (type == AudioDefinition::Type::MUSIC);
		sourceRegistration.mLoopStart = loopStart;
		sourceRegistration.mVolume = volume;
		++mNumSourcesByPackageType[(size_t)package];

		if (sourceType == SourceRegistration::Type::FILE)
		{
			RMX_CHECK(!audioFilename.empty(), "No audio file name set for audio key " << *keyString, );
			RMX_CHECK(sourceAddress == 0, "Source address can only be used with emulated sound, for audio key " << *keyString, );
			RMX_CHECK(contentOffset == 0, "Content offset can only be used with emulated sound, for audio key " << *keyString, );
			sourceRegistration.mSourceFile = *audioFilename;
		}
		else
		{
			sourceRegistration.mEmulationSfxId = emulationSfxId;
			sourceRegistration.mSourceFile = *audioFilename;	// Can be empty to use ROM's original SMPS data, or the name of a file containing that data
			sourceRegistration.mSourceAddress = sourceAddress;	// Can be zero to use original address in ROM, or the address where the SMPS data is located
			sourceRegistration.mContentOffset = contentOffset;
		}
	}

	++mChangeCounter;
	return true;
}

void AudioCollection::determineActiveSourceRegistrations(bool preferOriginalSoundtrack)
{
	for (auto& [key, audioDefinition] : mAudioDefinitions)
	{
		// Search for the right one considering settings
		SourceRegistration* bestSourceReg = nullptr;
		for (SourceRegistration& soundReg : audioDefinition.mSources)
		{
			if (shouldPreferSoundRegistration(soundReg, bestSourceReg, preferOriginalSoundtrack))
			{
				bestSourceReg = &soundReg;
			}
		}
		audioDefinition.mActiveSource = bestSourceReg;
	}
	++mChangeCounter;
}

const AudioCollection::AudioDefinition* AudioCollection::getAudioDefinition(uint64 keyId) const
{
	const AudioDefinition* audioDefinition = mapFind(mAudioDefinitions, keyId);
	if (nullptr != audioDefinition)
	{
		// Found directly
		return audioDefinition;
	}

	// It could be the hash of a hex code like "1C", check for that
	if (!getHexCodeRetranslation(keyId, keyId))
	{
		// It's not a hex code
		return nullptr;
	}

	// Try again, it might get found now
	return mapFind(mAudioDefinitions, keyId);
}

AudioCollection::SourceRegistration* AudioCollection::getSourceRegistration(uint64 keyId) const
{
	const AudioDefinition* audioDefinition = getAudioDefinition(keyId);
	return (nullptr == audioDefinition) ? nullptr : audioDefinition->mActiveSource;
}

AudioCollection::SourceRegistration* AudioCollection::getSourceRegistration(uint64 keyId, Package preferredPackage) const
{
	const AudioDefinition* audioDefinition = getAudioDefinition(keyId);
	if (nullptr == audioDefinition || nullptr == audioDefinition->mActiveSource)
	{
		// Not found
		return nullptr;
	}

	if (audioDefinition->mActiveSource->mPackage == preferredPackage)
	{
		// Active source is already the right choice
		return audioDefinition->mActiveSource;
	}

	// Search for a source registration of the preferred package
	for (const SourceRegistration& sourceReg : audioDefinition->mSources)
	{
		if (sourceReg.mPackage == preferredPackage)
		{
			// Found one
			return const_cast<AudioCollection::SourceRegistration*>(&sourceReg);
		}
	}

	// Fallback: Use active source
	return audioDefinition->mActiveSource;
}
