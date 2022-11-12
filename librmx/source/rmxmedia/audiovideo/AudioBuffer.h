/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	AudioBuffer
*		Stores audio data.
*/

#pragma once


class API_EXPORT AudioBuffer
{
public:
	typedef bool(*LoadCallbackType)(AudioBuffer*, const String&, const String&);
	typedef std::list<LoadCallbackType> LoadCallbackList;
	static LoadCallbackList mStaticLoadCallbacks;

	static const constexpr int MAX_FRAME_LENGTH = 4096;		// Maximum length of an audio frame in samples -- this is the length of all audio frames, except the last

public:
	AudioBuffer();
	~AudioBuffer();

	void clear(int frequency = 44100, int channels = 2);

	void addData(short** data, int length, int frequency = 0, int channels = 0);
	void addData(float** data, int length, int frequency = 0, int channels = 0);

	void markPurgeableSamples(int purgePosition);

	bool load(const String& source, const String& params = String());

	inline int getFrequency() const { return mFrequency; }
	inline int getChannels() const  { return mChannels; }

	inline int getLength() const	{ return mLength; }
	float getLengthInSec() const;

	size_t getMemoryUsage() const;

	inline bool isPersistent()  { return mPersistent; }
	void setPersistent(bool persistent);

	inline bool isCompleted() const  { return mCompleted; }
	void setCompleted(bool completed = true);

	int getData(short** output, int position) const;

	void lock();
	void unlock();

private:
	struct AudioFrame
	{
		short* mBuffer = nullptr;				// Holds all audio data
		short* mData[2] = { nullptr, nullptr };	// Pointers into the buffer, one for each channel
		int mLength = 0;						// Length in samples
	};

private:
	void clearInternal();
	AudioFrame& getWorkingFrame();

private:
	std::vector<AudioFrame*> mFrames;	// Array of audio frames
	int mPurgedFrames = 0;
	int mLength = 0;					// In samples

	int mChannels = 2;					// 1 for Mono, 2 for Stereo
	int mFrequency = 44100;				// Sampling frequency, e.g. 44100 Hz
	bool mPersistent = true;			// If false, played audio frames get deleted (e.g. for music streams)
	bool mCompleted = false;			// Set to true when loading / streaming is completed

	rmx::Mutex mMutex;
	int mMutexLockCounter = 0;
};
