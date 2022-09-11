/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class PlayerRecorder
{
public:
	static std::wstring getUnusedRecordingFilename(const std::wstring& path, const std::wstring& basename);

public:
	PlayerRecorder();

	void reset();
	void setCurrentDirectory(const std::wstring& directory);

	inline bool isPlaying() const  { return (mState != State::INACTIVE && !mRecordingActive); }

	void initForDirectory(const std::wstring& directory, const std::wstring& recBaseFilename, uint16 zoneAndAct, uint8 character);
	void initRecording(const std::wstring& filename, uint16 zoneAndAct, uint8 character);
	void initPlayback(const std::wstring& filename);

	void onPostUpdateFrame();
	bool onTimeAttackFinish(int& hundreds, std::vector<int>& otherTimes);

	inline void setMaxGhosts(int maxGhosts)  { mMaxGhosts = maxGhosts; }

private:
	struct Frame
	{
		enum Flag : uint8
		{
			FLAG_FLIP_X = 0x01,			// From "sonic.render_flags"
			FLAG_FLIP_Y = 0x02,			// From "sonic.render_flags"
			FLAG_PRIORITY = 0x04,		// From "sonic.sprite_attributes", flag 0x8000
			FLAG_LAYER = 0x08,			// From "u8[A0 + 0x46]", which is either 0x0c or 0x0e
			FLAGMASK_SECTION = 0x30		// Used only in rare occasions, like ICZ1's secret transition to ICZ2
		};
		uint16 mInput = 0;
		Vec2i  mPosition;
		uint16 mSprite = 0;
		uint8  mRotation = 0;
		uint8  mFlags = 0;
		// TODO: Adding velocity direction (as angle) would make sense here for Tails, to get smoother tails movement while rolling & jumping
	};
	struct Recording
	{
		std::wstring mFilename;
		uint16 mFormatVersion = 0;
		uint32 mGameVersion = 0;
		std::vector<std::pair<uint32, uint8>> mSettings;
		uint16 mZoneAndAct;
		uint8  mCategory;
		int    mRank = 0;
		uint32 mTime = 0;
		bool   mVisible = true;
		size_t mIndex = 0;			// Only valid during playback
		std::vector<Frame> mFrames;
	};

private:
	void updateRecording(Recording& recording, uint16 frameNumber);
	void updatePlayback(const Recording& recording, uint16 frameNumber);

	bool serializeRecording(VectorBinarySerializer& serializer, Recording& recording);

private:
	std::wstring mDirectory;
	int mMaxGhosts = 5;

	enum class State
	{
		INACTIVE = 0,	// No recording or playback active
		WAITING,		// Waiting for gameplay start
		ACTIVE			// Active recording and/or playback
	};
	State mState = State::INACTIVE;

	bool mRecordingActive = false;

	Recording mCurrentRecording;
	std::vector<Recording> mCurrentPlaybacks;
};
