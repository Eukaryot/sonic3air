/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class AudioSourceBase;


class AudioCollection : public SingleInstance<AudioCollection>
{
public:
	enum class Package
	{
		NONE		= 0,
		ORIGINAL	= 1,
		REMASTERED	= 2,
		MODDED		= 3,
		_NUM
	};

	struct AudioDefinition;

	struct SourceRegistration
	{
		enum class Type
		{
			FILE,					// Reading from an audio file
			EMULATION_BUFFERED,		// Using emulation -- playback once, then use buffered data (used for music and sounds that can't dynamically change)
			EMULATION_DIRECT,		// Using emulation -- direct playback, no caching (used e.g. for music that can be dynamically sped up)
			EMULATION_CONTINUOUS	// Using emulation -- direct playback, no caching, repeated starts are processed by sound driver (used for continuous sound effects)
		};

		AudioDefinition* mAudioDefinition = nullptr;
		Package mPackage = Package::ORIGINAL;
		Type mType = Type::FILE;
		std::wstring mSourceFile;
		uint8 mEmulationSfxId = 0;
		uint32 mSourceAddress = 0;
		uint32 mContentOffset = 0;
		bool mIsLooping = false;
		uint32 mLoopStart = 0;
		float mVolume = 1.0f;

		AudioSourceBase* mAudioSource = nullptr;	// Managed by AudioPlayer
	};

	struct AudioDefinition
	{
		enum class Type
		{
			MUSIC   = 0,	// Using channel 0, looping by default
			JINGLE  = 1,	// Using channel 0, not looping by default
			SOUND   = 2		// Using other or no channel at all, not looping by default
		};

		enum class Visibility
		{
			AUTO,
			ALWAYS_VISIBLE,
			ALWAYS_HIDDEN,
			DEV_MODE_ONLY
		};

		uint64 mPrimaryKeyId = 0;			// String hash of key string
		int16 mNumericKey = -1;				// uint8 numeric key if key string is a hex value like "2C", otherwise -1
		std::vector<uint64> mAllKeyIds;		// There may be multiple key IDs, namely the primary key, a lowercase hash, and the numeric key
		std::string mKeyString;
		std::string mDisplayName;

		Type mType = Type::SOUND;
		uint8 mChannel = 0xff;
		Visibility mSoundTestVisibility = Visibility::AUTO;

		SourceRegistration* mActiveSource = nullptr;
		std::vector<SourceRegistration> mSources;

		inline bool hasKeyId(uint64 audioKeyId) const  { return vectorContainsByPredicate(mAllKeyIds, [=](uint64 id) { return (id == audioKeyId); } ); }
	};

public:
	static int16 checkForNumericKey(std::string_view keyString);	// Returns -1 if not a uint8 key

public:
	AudioCollection();
	~AudioCollection();

	inline const std::unordered_map<uint64, AudioDefinition>& getAudioDefinitions() const  { return mUniqueAudioDefinitions; }
	inline int getNumSourcesByPackageType(Package package) const  { return (package < Package::_NUM) ? mNumSourcesByPackageType[(size_t)package] : 0; };

	void clear();
	void clearPackage(Package package);
	bool loadFromJson(const std::wstring& basepath, const std::wstring& filename, Package package);

	void registerAudioDefinitionKeyIds(AudioDefinition& audioDefinition);
	void unregisterAudioDefinitionKeyIds(const AudioDefinition& audioDefinition);

	void determineActiveSourceRegistrations(bool preferOriginalSoundtrack);

	const AudioDefinition* getAudioDefinition(uint64 keyId) const;
	SourceRegistration* getSourceRegistration(uint64 keyId) const;
	SourceRegistration* getSourceRegistration(uint64 keyId, Package preferredPackage) const;

	inline uint32 getChangeCounter() const  { return mChangeCounter; }

private:
	std::unordered_map<uint64, AudioDefinition> mUniqueAudioDefinitions;
	std::unordered_map<uint64, AudioDefinition*> mAudioDefinitionsByKeyId;
	int mNumSourcesByPackageType[(size_t)Package::_NUM] = { 0 };
	uint32 mChangeCounter = 0;
};
