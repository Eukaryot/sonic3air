/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/input/InputRecorder.h"
#include "oxygen/application/Configuration.h"


InputRecorder::InputRecorder() :
	mPosition(0),
	mIsPlaying(false),
	mIsRecording(false)
{
}

InputRecorder::~InputRecorder()
{
}

uint32 InputRecorder::getPosition() const
{
	return mPosition;
}

void InputRecorder::setPosition(uint32 position)
{
	mPosition = position;
}

void InputRecorder::initFromConfig()
{
	mIsPlaying = false;
	mIsRecording = false;

	Configuration& config = Configuration::instance();
	if (config.mInputRecorderInput.empty())
	{
		if (!config.mInputRecorderOutput.empty())
		{
			initForRecording();
		}
	}
	else
	{
		loadRecording(config.mInputRecorderInput);
	}
}

void InputRecorder::initForRecording()
{
	mPosition = 0;
	mFrames.clear();
	mIsPlaying = false;
	mIsRecording = true;
}

void InputRecorder::shutdown()
{
	if (isRecording())
	{
		saveRecording(Configuration::instance().mInputRecorderOutput);
	}

	mIsPlaying = false;
	mIsRecording = false;
}

const InputRecorder::InputState& InputRecorder::updatePlayback(uint32 position)
{
	mPosition = position;
	if (mPosition < mFrames.size())
	{
		return mFrames[mPosition].mInputState;
	}
	else
	{
		static InputState defaultInputState;
		return defaultInputState;
	}
}

void InputRecorder::updateRecording(const InputState& inputState)
{
	mPosition = (uint32)mFrames.size();
	mFrames.push_back(Frame());
	mFrames.back().mInputState = inputState;
}

bool InputRecorder::loadRecording(const std::vector<uint8>& buffer)
{
	VectorBinarySerializer serializer(true, buffer);
	serializeRecording(serializer);

	mIsPlaying = true;
	return true;
}

bool InputRecorder::loadRecording(const std::wstring& filename)
{
	mIsPlaying = false;

	std::vector<uint8> dump;
	const std::wstring completeFilename = Configuration::instance().mSaveStatesDir + L"/" + filename + L".inputrec";
	if (!FTX::FileSystem->readFile(completeFilename, dump))
		return false;

	return loadRecording(dump);
}

void InputRecorder::saveRecording(std::vector<uint8>& buffer)
{
	VectorBinarySerializer serializer(false, buffer);
	serializeRecording(serializer);
}

void InputRecorder::saveRecording(const std::wstring& filename)
{
	std::vector<uint8> dump;
	dump.reserve(8 + mFrames.size());

	saveRecording(dump);

	const std::wstring completeFilename = Configuration::instance().mSaveStatesDir + L"/" + filename + L".inputrec";
	FTX::FileSystem->saveFile(completeFilename, dump);
}

void InputRecorder::serializeRecording(VectorBinarySerializer& serializer)
{
	// Version
	uint32 version = 4;
	serializer & version;

	// Number of players
	size_t numPlayers = InputManager::NUM_PLAYERS;
	if (version >= 4)
	{
		serializer.serializeAs<uint8>(numPlayers);

		if (serializer.isReading())
		{
			numPlayers = (size_t)clamp(serializer.read<uint8>(), 1, InputManager::NUM_PLAYERS);
		}
	}
	else
	{
		numPlayers = (version >= 3) ? 2 : 1;
	}

	// Frames
	serializer.serializeArraySize(mFrames);
	for (Frame& frame : mFrames)
	{
		// Read inputs for each frame
		for (size_t k = 0; k < numPlayers; ++k)
		{
			serializer & frame.mInputState.mInputFlags[k];
		}

		// Fill missing players with empty inputs
		if (serializer.isReading())
		{
			for (size_t k = numPlayers; k < InputManager::NUM_PLAYERS; ++k)
			{
				frame.mInputState.mInputFlags[1] = 0;
			}
		}
	}
}
