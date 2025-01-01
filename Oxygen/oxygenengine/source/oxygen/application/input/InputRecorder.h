/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/ControlsIn.h"

class Emulator;


class InputRecorder
{
public:
	struct InputState
	{
		uint16 mInputFlags[2];    // For two gamepads
		inline InputState() : mInputFlags { 0, 0 } {}
	};

public:
	InputRecorder();
	~InputRecorder();

	uint32 getPosition() const;
	void setPosition(uint32 position);

	void initFromConfig();
	void initForRecording();
	void shutdown();

	inline bool isPlaying() const	{ return mIsPlaying; }
	inline bool isRecording() const { return mIsRecording; }

	const InputState& updatePlayback(uint32 position);
	void updateRecording(const InputState& inputState);

	bool loadRecording(const std::vector<uint8>& buffer);
	bool loadRecording(const std::wstring& filename);
	void saveRecording(std::vector<uint8>& buffer);
	void saveRecording(const std::wstring& filename);

private:
	struct Frame
	{
		InputState mInputState;
	};

private:
	std::vector<Frame> mFrames;
	uint32 mPosition;
	bool mIsPlaying;
	bool mIsRecording;
};
