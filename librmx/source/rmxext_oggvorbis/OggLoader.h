/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	OggLoader
*		Plays Ogg Vorbis audio files.
*/

#pragma once


// OggLoaderError
enum class OggLoaderError
{
	OK = 0,
	COULD_NOT_OPEN_FILE,
	UNEXPECTED_EOF,
	INVALID_VORBIS_HEADER,
	HEADERS_NOT_FOUND
};

enum class OggLoaderState
{
	INACTIVE = 0,
	STREAMING,
	COMPLETE
};


// OggLoader
class OggLoader
{
public:
	static bool staticLoadVorbis(AudioBuffer* buffer, const String& source, const String& params);

public:
	OggLoader();
	~OggLoader();

	void reset();

	bool startVorbisStreaming(AudioBuffer* audiobuffer, InputStream* istream, float precachingTime = 0.0f);

	bool updateStreaming();
	void precache(float time);

	bool loadVorbis(AudioBuffer* buffer, const String& source);

	void seek(float targetTime);

	float getVorbisPosition();
	float getFilePosition();

	inline bool isStreaming() const        { return mIsStreaming; }
	inline bool isStreamingVorbis() const  { return (mAudioBuffer != nullptr); }
	inline OggLoaderState getAudioState() const  { return mAudioState; }

	inline OggLoaderError getError() const  { return mError; }

private:
	int  bufferData();
	bool openStreams(InputStream* istream);
	int  seekInternal(float targetTime, std::streamsize& rangeMin, std::streamsize& rangeMax);

private:
	bool mIsStreaming = false;
	AudioBuffer* mAudioBuffer = nullptr;
	InputStream* mInputStream = nullptr;
	OggLoaderError mError = OggLoaderError::OK;
	ogg_int64_t mVorbisGranulePos = 0;
	OggLoaderState mAudioState = OggLoaderState::INACTIVE;
	int mSkipAudioSampleOutput = 0;

	// Ogg/Vorbis data structures
	ogg_sync_state   mSyncState;
	ogg_stream_state mVorbisStreamState;
	vorbis_info      mVorbisInfo;
	vorbis_comment   mVorbisComment;
	vorbis_dsp_state mVorbisDspState;
	vorbis_block     mVorbisBlock;
};
