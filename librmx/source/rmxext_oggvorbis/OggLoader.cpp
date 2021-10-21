/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxext_oggvorbis.h"


// Static load function
bool OggLoader::staticLoadVorbis(AudioBuffer* buffer, const String& source, const String& params)
{
	if (nullptr == buffer)
		return false;
	if (!source.endsWith(".ogg") && !source.endsWith(".oga") && !source.endsWith(".ogv"))
		return false;
	OggLoader loader;
	return loader.loadVorbis(buffer, source);
}


OggLoader::OggLoader()
{
	mIsStreaming = false;
	mInputStream = nullptr;
	mAudioBuffer = nullptr;
	mError = OggLoaderError::OK;
	ogg_sync_init(&mSyncState);
}

OggLoader::~OggLoader()
{
	ogg_sync_clear(&mSyncState);
	reset();
}

void OggLoader::reset()
{
	if (mAudioState != OggLoaderState::INACTIVE)
	{
		mAudioState = OggLoaderState::INACTIVE;

		ogg_stream_clear(&mVorbisStreamState);
		vorbis_block_clear(&mVorbisBlock);
		vorbis_dsp_clear(&mVorbisDspState);
		vorbis_comment_clear(&mVorbisComment);
		vorbis_info_clear(&mVorbisInfo);
	}

	mIsStreaming = false;
	mInputStream = nullptr;
	mAudioBuffer = nullptr;
	mError = OggLoaderError::OK;

	ogg_sync_reset(&mSyncState);
}

int OggLoader::bufferData()
{
	if (nullptr == mInputStream)
		return 0;

	const size_t bufferSize = 0x4000;
	char* buffer = ogg_sync_buffer(&mSyncState, (long)bufferSize);
	const size_t bytes = mInputStream->read(buffer, bufferSize);
	if (bytes == 0)
		return 0;

	ogg_sync_wrote(&mSyncState, (long)bytes);
	return (int)bytes;
}

bool OggLoader::openStreams(InputStream* istream)
{
	mInputStream = istream;
	mAudioState = OggLoaderState::INACTIVE;

	// Initialisation Ogg/Vorbis
	vorbis_info_init(&mVorbisInfo);
	vorbis_comment_init(&mVorbisComment);

	// Read headers
	int numVorbisHeaders = 0;
	ogg_page oggPage;
	ogg_packet oggPacket;
	bool finished = false;
	while (!finished)
	{
		const int bytes_read = bufferData();
		if (bytes_read <= 0)
			break;

		while (ogg_sync_pageout(&mSyncState, &oggPage) > 0)
		{
			if (!ogg_page_bos(&oggPage))
			{
				if (numVorbisHeaders > 0)
					ogg_stream_pagein(&mVorbisStreamState, &oggPage);
				finished = true;
				break;
			}

			ogg_stream_state testStreamState;
			ogg_stream_init(&testStreamState, ogg_page_serialno(&oggPage));
			ogg_stream_pagein(&testStreamState, &oggPage);
			ogg_stream_packetout(&testStreamState, &oggPacket);

			// Identify header packet, is it Vorbis?
			if (!numVorbisHeaders && vorbis_synthesis_headerin(&mVorbisInfo, &mVorbisComment, &oggPacket) >= 0)
			{
				// Copy over the data from the temporary stream to the actual Vorbis stream
				memcpy(&mVorbisStreamState, &testStreamState, sizeof(testStreamState));
				numVorbisHeaders = 1;
			}
			else
			{
				// Ignore (and skip) all the rest
				ogg_stream_clear(&testStreamState);
			}
		}
	}

	// More header packets (we need up to 3)
	while (numVorbisHeaders && numVorbisHeaders < 3)
	{
		// Vorbis
		int result;
		while (numVorbisHeaders && (numVorbisHeaders < 3) && (result = ogg_stream_packetout(&mVorbisStreamState, &oggPacket)))
		{
			bool success = (result >= 0);
			if (success)
				success = (vorbis_synthesis_headerin(&mVorbisInfo, &mVorbisComment, &oggPacket) == 0);
			if (!success)
			{
				mError = OggLoaderError::INVALID_VORBIS_HEADER;
				return false;
			}
			numVorbisHeaders++;
		}

		// In a usual stream, there should be first all headers, then the content
		if (ogg_sync_pageout(&mSyncState, &oggPage) > 0)
		{
			if (numVorbisHeaders)
				ogg_stream_pagein(&mVorbisStreamState, &oggPage);
		}
		else
		{
			// Need more data from the input stream
			if (bufferData() <= 0)
			{
				mError = OggLoaderError::UNEXPECTED_EOF;
				return false;
			}
		}
	}

	// Initialisation Vorbis decoder
	if (numVorbisHeaders > 0 && nullptr != mAudioBuffer)
	{
		vorbis_synthesis_init(&mVorbisDspState, &mVorbisInfo);
		vorbis_block_init(&mVorbisDspState, &mVorbisBlock);
		mVorbisGranulePos = 0;
		mAudioBuffer->clear(mVorbisInfo.rate, 2);
		mAudioState = OggLoaderState::STREAMING;
	}
	else
	{
		// Empty data structures
		vorbis_info_clear(&mVorbisInfo);
		vorbis_comment_clear(&mVorbisComment);
		mAudioBuffer = nullptr;
	}

	// Did we get usable headers now?
	if (nullptr == mAudioBuffer)
	{
		mError = OggLoaderError::HEADERS_NOT_FOUND;
		return false;
	}

	mError = OggLoaderError::OK;
	return true;
}

bool OggLoader::startVorbisStreaming(AudioBuffer* audiobuffer, InputStream* istream, float precachingTime)
{
	// Read from Ogg Vorbis input stream
	reset();
	if (nullptr == audiobuffer)
		return false;
	mAudioBuffer = audiobuffer;
	mAudioBuffer->clear();
	mIsStreaming = openStreams(istream);
	if (!mIsStreaming)
		return false;
	if (precachingTime > 0.0f)
		precache(precachingTime);
	return true;
}

bool OggLoader::updateStreaming()
{
	// Streaming update
	if (!mIsStreaming)
		return false;

	if (nullptr != mAudioBuffer)
	{
		// Read fully decoded data if possible
		float** pcm;
		const int memcount = vorbis_synthesis_pcmout(&mVorbisDspState, &pcm);
		if (memcount > 0)
		{
			// Fill output audio buffer
			int samples = memcount;
			float* source[2];
			source[0] = pcm[0];
			source[1] = (mVorbisInfo.channels >= 2) ? pcm[1] : pcm[0];

			if (mSkipAudioSampleOutput >= samples)
			{
				mSkipAudioSampleOutput -= samples;
			}
			else
			{
				if (mSkipAudioSampleOutput > 0)
				{
					source[0] += mSkipAudioSampleOutput;
					source[1] += mSkipAudioSampleOutput;
					samples -= mSkipAudioSampleOutput;
					mSkipAudioSampleOutput = 0;
				}

				mAudioBuffer->lock();
				mAudioBuffer->addData(source, samples);
				mAudioBuffer->unlock();
			}

			vorbis_synthesis_read(&mVorbisDspState, memcount);
			return true;
		}

		// Decode next Vorbis packet
		ogg_packet oggPacket;
		if (ogg_stream_packetout(&mVorbisStreamState, &oggPacket) > 0)
		{
			if (vorbis_synthesis(&mVorbisBlock, &oggPacket) == 0)
			{
				vorbis_synthesis_blockin(&mVorbisDspState, &mVorbisBlock);
			}
			return true;
		}
	}

	// Get more ogg pages
	ogg_page oggPage;
	if (ogg_sync_pageout(&mSyncState, &oggPage) == 1)
	{
		const ogg_int64_t gpos = ogg_page_granulepos(&oggPage);
		if (nullptr != mAudioBuffer && ogg_stream_pagein(&mVorbisStreamState, &oggPage) == 0)
		{
			if (gpos >= 0)
				mVorbisGranulePos = gpos;
			return true;
		}
		return true;
	}

	// If all above failed, more data from the input stream is needed
	if (bufferData() > 0)
	{
		return true;
	}

	// Input stream read complete and decoded everything, word here is done
	mAudioState = OggLoaderState::COMPLETE;
	mIsStreaming = false;

	mAudioBuffer->setCompleted();
	return false;
}

void OggLoader::precache(float time)
{
	// Load data until enough was cached
	if (!mIsStreaming)
		return;

	while (true)
	{
		const bool audioReady = (nullptr == mAudioBuffer || mAudioBuffer->getLengthInSec() >= time);
		if (!audioReady)
		{
			if (!updateStreaming())
				break;
		}
	}
}

bool OggLoader::loadVorbis(AudioBuffer* buffer, const String& source)
{
	// Load Vorbis stream from a file
	FileInputStream istream;
	if (!istream.open(source))
		return false;
	if (!startVorbisStreaming(buffer, &istream))
		return false;
	while (updateStreaming());
	return true;
}

void OggLoader::seek(float targetTime)
{
	// No stream, no fun
	if (nullptr == mInputStream)
		return;

	mIsStreaming = true;
	if (mAudioState == OggLoaderState::COMPLETE)
		mAudioState = OggLoaderState::STREAMING;

	// Audio seeking
	std::streamsize rangeMin = 0;
	std::streamsize rangeMax = (targetTime <= 0.0f) ? 1 : mInputStream->getSize();

	while (rangeMin < rangeMax)
	{
		const int result = seekInternal(targetTime, rangeMin, rangeMax);
		if (result < 0)
		{
			// Failed
			return;
		}

		if (result > 0)
		{
			// Done successfully

			// For a sample-perfect seeking:
			//  - Calculate the difference between current granule position and target position
			//  - Then skip this number of samples during output to the audio buffer
			mSkipAudioSampleOutput = clamp(roundToInt(targetTime * mVorbisInfo.rate) - (int)mVorbisGranulePos, 0, 100000);
			return;
		}
	}
}

int OggLoader::seekInternal(float targetTime, std::streamsize& rangeMin, std::streamsize& rangeMax)
{
	const std::streamsize streamPosition = (rangeMin + rangeMax) / 2;
	if (streamPosition == 0)
	{
		// Special handling for seeking to the start
		// TODO: This should also be used if seeking before the first granule pos, otherwise some parts before that can get skipped
		mInputStream->rewind();
		ogg_stream_reset(&mVorbisStreamState);
		ogg_sync_reset(&mSyncState);
	}
	else
	{
		ogg_int64_t granulePosFormer = -1;

		// Phase 1: Check if we're roughly at the right position, and find out what's the granule positions around the target time
		// Phase 2: Go through this part of the data stream again to find the granule position just before the target time
		for (int phase = 1; phase <= 2; ++phase)
		{
			mInputStream->setPosition((int)streamPosition);
			ogg_stream_reset(&mVorbisStreamState);
			ogg_sync_reset(&mSyncState);

			ogg_page oggPage;
			int result = 0;
			while (result <= 0)
			{
				if (result == 0)
				{
					if (bufferData() <= 0)
						return -1;
				}

				result = ogg_sync_pageseek(&mSyncState, &oggPage);
			}

			bool nextPhase = false;
			while (!nextPhase)
			{
				// Keep feeding in pages
				while (ogg_sync_pageout(&mSyncState, &oggPage) != 1)
				{
					if (bufferData() <= 0)
						return -1;
				}

				ogg_packet oggPacket;
				if (ogg_stream_pagein(&mVorbisStreamState, &oggPage) != 0)
					continue;

				while (ogg_stream_packetout(&mVorbisStreamState, &oggPacket) > 0)
				{
					// Find a packet with a granule pos
					if (oggPacket.granulepos < 0)
						continue;

					if (phase == 1)
					{
						// Phase 1: Check if we're roughly at the right position, and find out what's the granule position just before the target time
						const float foundTime = (float)vorbis_granule_time(&mVorbisDspState, oggPacket.granulepos);
						if (granulePosFormer == -1)
						{
							// Too early?
							if (foundTime < targetTime - 1.0f)
							{
								rangeMin = streamPosition + 1;
								return 0;
							}

							// Too late?
							if (foundTime > targetTime)
							{
								rangeMax = streamPosition - 1;
								return 0;
							}

							granulePosFormer = oggPacket.granulepos;
						}
						else
						{
							if (foundTime <= targetTime)
							{
								granulePosFormer = oggPacket.granulepos;
							}
							else
							{
								// Granule positions from here are all after the target time, so go to phase 2 now
								nextPhase = true;
								break;
							}
						}
					}
					else
					{
						// Phase 2: Go through this part of the data stream again to find the granule position just before the target time
						if (oggPacket.granulepos >= granulePosFormer)
						{
							// Synthesize the ogg packet we just read
							mVorbisGranulePos = oggPacket.granulepos;
							vorbis_synthesis_restart(&mVorbisDspState);
							if (vorbis_synthesis(&mVorbisBlock, &oggPacket) == 0)
							{
								vorbis_synthesis_blockin(&mVorbisDspState, &mVorbisBlock);
								return 1;
							}
						}
					}
				}
			}
		}
	}

	// Should never be reached
	return -1;
}

float OggLoader::getVorbisPosition()
{
	if (!mIsStreaming)
		return 0.0f;
	return (float)vorbis_granule_time(&mVorbisDspState, mVorbisGranulePos);
}

float OggLoader::getFilePosition()
{
	if (nullptr == mInputStream)
		return 1.0f;
	return (float)mInputStream->getPosition() / (float)mInputStream->getSize();
}
