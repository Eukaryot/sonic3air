/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
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

	// Version
	uint32 version;
	serializer & version;

	const uint32 numberOfFrames = serializer.read<uint32>();
	mFrames.resize(numberOfFrames);

	for (uint32 i = 0; i < numberOfFrames; ++i)
	{
		Frame& frame = mFrames[i];

		// Controller 1
		serializer & frame.mInputState.mInputFlags[0];

		if (version >= 3)
		{
			// Controller 2
			serializer & frame.mInputState.mInputFlags[1];
		}
	}

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

	// Version
	uint32 version = 3;
	serializer & version;

	serializer.writeAs<uint32>(mFrames.size());

	for (Frame& frame : mFrames)
	{
		serializer & frame.mInputState.mInputFlags[0];
		serializer & frame.mInputState.mInputFlags[1];
	}
}

void InputRecorder::saveRecording(const std::wstring& filename)
{
	std::vector<uint8> dump;
	dump.reserve(8 + mFrames.size());

	saveRecording(dump);

	const std::wstring completeFilename = Configuration::instance().mSaveStatesDir + L"/" + filename + L".inputrec";
	FTX::FileSystem->saveFile(completeFilename, dump);
}
