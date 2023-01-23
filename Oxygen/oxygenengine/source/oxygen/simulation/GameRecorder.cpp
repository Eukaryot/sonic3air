/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/GameRecorder.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/helper/FileHelper.h"


void GameRecorder::clear()
{
	mFrames.clear();
	mFrameNoDataPool.clear();
	mFrameWithDataPool.clear();
	mPlaybackPosition = -1;
	mRangeStart = 0;
	mRangeEnd = 0;
}

void GameRecorder::addFrame(const uint16* inputs)
{
	RMX_CHECK(!mFrames.empty(), "First frame must be a keyframe", );
	addFrameInternal(inputs, Frame::Type::INPUT_ONLY);
}

void GameRecorder::addKeyFrame(const uint16* inputs, const std::vector<uint8>& data)
{
	Frame& frame = addFrameInternal(inputs, Frame::Type::KEYFRAME);
	frame.mType = Frame::Type::KEYFRAME;
	frame.mData = data;
}

void GameRecorder::discardOldFrames(uint32 minKeepNumber)
{
	size_t firstIndexToKeep = 0;
	for (size_t index = 0; index < mFrames.size(); ++index)
	{
		if (mFrames[index]->mNumber >= mRangeEnd - minKeepNumber)
		{
			firstIndexToKeep = index;
			break;
		}
	}

	// Go back to the last keyframe, as we can't keep dependent frames whose keyframes get discarded
	for (; firstIndexToKeep > 0; --firstIndexToKeep)
	{
		if (mFrames[firstIndexToKeep]->mType == Frame::Type::KEYFRAME)
			break;
	}

	if (firstIndexToKeep > 0)
	{
		for (size_t index = 0; index < firstIndexToKeep; ++index)
		{
			Frame& frame = *mFrames[index];
			if (frame.mType == Frame::Type::INPUT_ONLY)
				mFrameNoDataPool.returnObject(frame);
			else
				mFrameWithDataPool.returnObject(frame);
		}
		mFrames.erase(mFrames.begin(), mFrames.begin() + firstIndexToKeep);

		mRangeStart = mFrames[0]->mNumber;
		RMX_CHECK(mRangeEnd == mRangeStart + (uint32)mFrames.size(), "Inconsistency in game recorder frames", );
	}
}

bool GameRecorder::updatePlayback(PlaybackResult& outResult)
{
	if (mPlaybackPosition == -1)
	{
		return false;
	}
	else if (mPlaybackPosition < (int32)mRangeStart || mPlaybackPosition >= (int32)mRangeEnd)
	{
		mPlaybackPosition = -1;
		return false;
	}

	Frame& frame = *mFrames[mPlaybackPosition];
	outResult.mInputs[0] = frame.mInputs[0];
	outResult.mInputs[1] = frame.mInputs[1];

	if (frame.mType == Frame::Type::KEYFRAME)
	{
		if (!mIgnoreKeys || mPlaybackPosition == 0)
		{
			// TODO: Handle "frame.mCompressedData == true" (not needed for pure playback after loading from file)
			outResult.mData = &frame.mData;
		}
	}

	LogDisplay::instance().setModeDisplay("Game recorder playback at position: " + std::to_string(mPlaybackPosition));

	++mPlaybackPosition;
	return true;
}

bool GameRecorder::loadRecording(const std::wstring& filename)
{
	clear();

	std::vector<uint8> dump;
	if (!FTX::FileSystem->readFile(filename, dump))
		return false;

	VectorBinarySerializer serializer(true, dump);

	// Signature
	char signature[4];
	serializer.read(signature, 4);
	int formatVersion = 0;
	if (memcmp(signature, "GRC1", 4) == 0)
	{
		formatVersion = 1;
	}
	else if (memcmp(signature, "GRC0", 4) == 0)
	{
		formatVersion = 0;
	}
	else
	{
		return false;
	}

	char buildString[11] = "..........";
	serializer.read(buildString, 10);

	// Game-specific
	if (std::string(buildString) >= "19.08.11.0")
	{
		const uint32 bufferSize = serializer.read<uint32>();
		if (bufferSize > 0)
		{
			std::vector<uint8> buffer;
			buffer.resize((size_t)bufferSize);
			serializer.read((char*)&buffer[0], bufferSize);
			EngineMain::getDelegate().onGameRecordingHeaderLoaded(buildString, buffer);
		}
	}

	// Load all frames
	const uint32 frameCount = serializer.read<uint32>();
	for (uint32 index = 0; index < frameCount; ++index)
	{
		const Frame::Type frameType = (Frame::Type)serializer.read<uint8>();
		Frame& frame = createFrameInternal(frameType, index);
		mFrames.push_back(&frame);

		serializer.serialize(frame.mInputs[0]);
		serializer.serialize(frame.mInputs[1]);

		if (frameType == Frame::Type::KEYFRAME)
		{
			frame.mCompressedData = false;
			const uint32 dataSize = serializer.read<uint32>();
			if (dataSize > 0)
			{
				if (formatVersion >= 1)
				{
					// Newer versions use zlib deflate, including the two zlib header bytes
					ZlibDeflate::decode(frame.mData, serializer.peek(), dataSize);
					serializer.skip(dataSize);
				}
				else
				{
					// Older version using own (slower) deflate implementation, lacking the two zlib header bytes
					frame.mData.resize(dataSize);
					serializer.read((char*)&frame.mData[0], dataSize);

					// Uncompress data
					int size = 0;
					uint8* decoded = Deflate::decode(size, (void*)&frame.mData[0], (uint32)frame.mData.size());
					frame.mData.resize(size);
					frame.mData.shrink_to_fit();
					memcpy(frame.mData.data(), decoded, size);
					delete[] decoded;
				}
			}
		}
	}

	mPlaybackPosition = 0;
	mRangeEnd = (uint32)mFrames.size();
	return true;
}

bool GameRecorder::saveRecording(const std::wstring& filename) const
{
	// Create directory if needed
	const size_t slashPosition = filename.find_last_of(L"/\\");
	if (slashPosition != std::string::npos)
	{
		FTX::FileSystem->createDirectory(filename.substr(0, slashPosition));
	}

	std::vector<uint8> dump;
	VectorBinarySerializer serializer(false, dump);

	// Signature
	const EngineDelegateInterface::AppMetaData& appMetaData = EngineMain::getDelegate().getAppMetaData();
	const char SIGNATURE[] = "GRC1";
	serializer.write(SIGNATURE, 4);
	serializer.write(appMetaData.mBuildVersionString.c_str(), 10);

	// Game-specific
	std::vector<uint8> buffer;
	{
		EngineMain::getDelegate().onGameRecordingHeaderSave(buffer);
		const uint32 bufferSize = (uint32)buffer.size();
		serializer.write(bufferSize);
		if (bufferSize > 0)
		{
			serializer.write(&buffer[0], bufferSize);
		}
	}

	// Save all frames
	const uint32 frameCount = (uint32)mFrames.size();
	serializer.write(frameCount);

	for (Frame* frame : mFrames)
	{
		serializer.writeAs<uint8>(frame->mType);
		serializer.write(frame->mInputs[0]);
		serializer.write(frame->mInputs[1]);

		if (frame->mType == Frame::Type::KEYFRAME)
		{
			if (!frame->mCompressedData)
			{
				// Compress data
				buffer.clear();
				ZlibDeflate::encode(buffer, &frame->mData[0], frame->mData.size());
				frame->mData.swap(buffer);
				frame->mData.shrink_to_fit();
				frame->mCompressedData = true;
			}

			const uint32 dataSize = (uint32)frame->mData.size();
			serializer.write(dataSize);
			serializer.write(&frame->mData[0], dataSize);
		}
	}

	return FTX::FileSystem->saveFile(filename, dump);
}

GameRecorder::Frame& GameRecorder::createFrameInternal(Frame::Type frameType, uint32 number)
{
	Frame& frame = (frameType == Frame::Type::INPUT_ONLY) ? mFrameNoDataPool.rentObject() : mFrameWithDataPool.rentObject();
	frame.mType = frameType;
	frame.mNumber = number;
	frame.mInputs[0] = 0;
	frame.mInputs[1] = 0;
	frame.mCompressedData = false;
	frame.mData.clear();
	return frame;
}

GameRecorder::Frame& GameRecorder::addFrameInternal(const uint16* inputs, Frame::Type frameType)
{
	Frame& frame = createFrameInternal(frameType, mRangeEnd);
	frame.mInputs[0] = inputs[0];
	frame.mInputs[1] = inputs[1];

	mFrames.push_back(&frame);
	++mRangeEnd;
	return frame;
}
