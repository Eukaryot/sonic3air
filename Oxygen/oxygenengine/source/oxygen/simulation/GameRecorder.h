/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class CodeExec;


class GameRecorder
{
public:
	struct InputData
	{
		uint16 mInputs[2] = { 0, 0 };
	};

	struct PlaybackResult
	{
		const InputData* mInput = nullptr;
		const std::vector<uint8>* mData = nullptr;
	};

public:
	void clear();
	void addFrame(const InputData& input);
	void addKeyFrame(const InputData& input, const std::vector<uint8>& data);

	void discardOldFrames(uint32 minKeepNumber = 3600);

	inline uint32 getCurrentNumberOfFrames() const  { return mRangeEnd - mRangeStart; }
	inline uint32 getRangeStart() const	 { return mRangeStart; }
	inline uint32 getRangeEnd() const	 { return mRangeEnd; }

	inline bool hasFrameNumber(uint32 frameNumber) const  { return frameNumber >= mRangeStart && frameNumber < mRangeEnd; }
	bool updatePlayback(uint32 frameNumber, PlaybackResult& outResult);

	bool getFrameData(uint32 frameNumber, PlaybackResult& outResult);
	void jumpToPosition(uint32 nextFrameNumber, bool discardNewerFrames);

	bool loadRecording(const std::wstring& filename);
	bool saveRecording(const std::wstring& filename) const;

	inline void setIgnoreKeys(bool ignoreKeys)  { mIgnoreKeys = ignoreKeys; }

private:
	struct Frame
	{
		enum class Type : uint8
		{
			INPUT_ONLY,
			KEYFRAME,
			//DIFFERENTIAL	// Difference to last keyframe; not implemented yet
		};

		Type mType = Type::INPUT_ONLY;
		uint32 mNumber = 0;
		InputData mInput;
		bool mCompressedData = false;
		std::vector<uint8> mData;
	};

private:
	Frame& createFrameInternal(Frame::Type frameType, uint32 number);
	Frame& addFrameInternal(const InputData& input, Frame::Type frameType);

private:
	std::vector<Frame*> mFrames;
	RentableObjectPool<Frame> mFrameNoDataPool;
	RentableObjectPool<Frame> mFrameWithDataPool;

	uint32 mRangeStart = 0;			// Frame number of first frame stored in mFrames
	uint32 mRangeEnd = 0;			// Frame number of last frame stored in mFrames plus one (!)
	bool mIgnoreKeys = false;
};
