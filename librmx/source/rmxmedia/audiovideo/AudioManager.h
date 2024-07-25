/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Audio
*		Music and sounds output.
*/

#pragma once


class AudioReference;

namespace rmx
{
	class AudioMixer;


	// Audio manager
	class API_EXPORT AudioManager
	{
	public:
		struct AudioInstance
		{
			int mID = 0;							// Unique audio instance ID, invalid if 0
			AudioBuffer* mAudioBuffer = nullptr;	// The audio buffer used as a source, must not be a nullptr
			AudioMixer* mAudioMixer = nullptr;		// Audio mixer this is played in
			int mPosition = 0;						// Position in the audio buffer, in samples
			int mTimeout = 0;						// Time until playback gets stopped in samples, or 0 if not used
			int mLoopStart = 0;						// If looping is enabled, jump back to this sample position
			float mVolume = 1.0f;					// Volume in range [0.0f, 1.0f]
			float mVolumeChange = 0.0f;				// Volume change rate per second, usually 0.0f
			float mSpeed = 1.0f;					// Playback speed, usually 1.0f
			float mPanning = 0.0f;					// Left/right panning value in range [-1.0f, +1.0f], usually 0.0f
			bool mLoop = false;						// Set if sound playback should be looped
			bool mPaused = false;					// Set when sound playback is paused
			bool mUsePan = false;					// Set if panning should be used
			bool mStreaming = false;				// Set if reaching the end of the audio buffer should not stop the playback, just temporily pause it until more data comes in
			bool mPlaybackDone = false;				// Gets set by audio mixer when playback should stop now
		};

		struct PlaybackOptions
		{
			AudioBuffer* mAudioBuffer = nullptr;
			float mVolume = 1.0f;
			float mVolumeChange = 0.0f;
			int mAudioMixerId = 0;
			float mSpeed = 1.0f;
			float mPosition = 0.0f;
			bool mLoop = false;
			bool mStreaming = false;
		};

	public:
		AudioManager();
		~AudioManager();

		void initialize(int sample_freq = 44100, int channels = 2, int audioBufferSamples = 1024);
		void exit();

		void clear();

		void playAudio(bool onoff);
		bool getAudioState();

		void lockAudio();
		void unlockAudio();

		void regularUpdate(float deltaSeconds);	// Should best be called once every frame

		void setGlobalVolume(float volume);

		template<typename T>
		T& createAudioMixer(int mixerId, int parentMixerId = 0)
		{
			RMX_ASSERT(mixerId != 0, "Root audio mixer (with ID 0) can't be replaced");
			T* audioMixer = new T(mixerId);
			registerAudioMixer(*audioMixer, parentMixerId);
			return *audioMixer;
		}

		AudioMixer* getAudioMixerByID(int mixerId) const;
		void deleteAudioMixerByID(int mixerId);

		float getAudioMixerVolumeByID(int mixerId) const;
		void setAudioMixerVolumeByID(int mixerId, float relativeVolume);

		bool addSound(const PlaybackOptions& playbackOptions, AudioReference& ref);
		int addSound(AudioBuffer* audiobuffer, bool streaming = false);							// Deprecated
		bool addSound(AudioBuffer* audiobuffer, AudioReference& ref, bool streaming = false);	// Deprecated

		void removeSound(AudioReference& ref);
		void removeAllSounds();

		AudioInstance* findInstance(int ID);

		inline int getChangeCounter() const  { return mChangeCounter; }

		inline int getOutputBufferSize() const		  { return mFormat.samples; }
		inline int getOutputFrequency() const		  { return mFormat.freq; }
		inline uint32 getGlobalPlayedSamples() const  { return mPlayedSamples; }
		inline double getGlobalPlaybackTime() const   { return (double)mPlayedSamples / (double)mFormat.freq; }

	private:
		void registerAudioMixer(AudioMixer& audioMixer, int parentMixerId);

		void removeInstance(int ID);
		void processRemoveIDs();

		static void mixAudioStatic(void* _userdata, uint8* outputStream, int outputBytes);
		void mixAudio(uint8* outputStream, int outputBytes);

	private:
		SDL_AudioDeviceID mAudioDeviceID = 0;		// Audio device opened by SDL
		SDL_AudioSpec mFormat;						// Audio format
		uint32 mAudioLocks = 0;						// Set if audio device is locked right now (needed to allow for nested audio locking)
		std::map<int, AudioInstance> mInstances;	// Map of all active audio instances by their ID
		std::vector<int> mRemoveIDs;				// Audio instance IDs that got invalid during audio mixing
		int mNextFreeID = 1;						// ID to use for next audio instance created
		int mChangeCounter = 0;						// Changed whenever an audio instance gets created or invalidated
		uint32 mPlayedSamples = 0;					// Number of samples played (this takes about one day to overflow at 48 kHz)

		float mTimeSinceLastUpdate = 0.0f;

		// Mixers
		std::map<int, AudioMixer*> mAudioMixers;
		AudioMixer& mRootMixer;				// Root audio mixer
	};


	// WAV loader
	class API_EXPORT WavLoader
	{
	public:
		static bool load(AudioBuffer* buffer, const String& source, const String& params);
	};

}
