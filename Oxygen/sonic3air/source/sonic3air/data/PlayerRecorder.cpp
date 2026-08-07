/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/data/PlayerRecorder.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/data/TimeAttackData.h"
#include "sonic3air/helper/GameUtils.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/rendering/parts/SpriteManager.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/Simulation.h"


namespace
{
	// General recording file format version
	//  - 0x0103 = First released build
	//  - 0x0104 = Change from characters to categories (introduction of "Sonic - Max Control")
	//  - 0x0105 = Support for extended animation sprites
	//  - 0x0106 = Serialization of settings
	const uint16 EARLIEST_FORMAT_VERSION = 0x0103;
	const uint16 CURRENT_FORMAT_VERSION	 = 0x0106;

	const wchar_t* getLeadingZeroString(uint32 value)
	{
		static wchar_t str[9] = { 0 };
		str[8] = 0;
		for (int i = 0; i <= 7; ++i)
		{
			str[7-i] = L'0' + (value % 10);
			value /= 10;
		}
		return str;
	}

	void getSplittedTime(uint32 frames, uint32& min, uint32& sec, uint32& hsec)
	{
		min = frames / 3600;
		sec = (frames / 60) % 60;
		hsec = ((frames % 60) * 99 + 30) / 59;		// Same calculation as used by script for HUD display
	}

	int getHundreds(uint32 frames)
	{
		uint32 min, sec, hsec;
		getSplittedTime(frames, min, sec, hsec);
		return min * 6000 + sec * 100 + hsec;
	}

	uint8 getLevelSectionFlags(EmulatorInterface& emulatorInterface, uint16 zoneAndAct)
	{
		if (zoneAndAct == 0x0000)
		{
			// AIZ act 1 internal transition to act 2
			if (emulatorInterface.readMemory16(0xfffffe10) == 0x0001)
				return 0x10;
		}
		else if (zoneAndAct == 0x0500)
		{
			// ICZ act 1 internal transition to act 2
			if (emulatorInterface.readMemory16(0xfffffe10) == 0x0501)
				return 0x10;
		}
		return 0;
	}
}


std::wstring PlayerRecorder::getUnusedRecordingFilename(const std::wstring& path, const std::wstring& basename)
{
	// TODO: Initialize startup value from last session
	static uint32 nextRecordingIndex = 1;
	while (true)
	{
		std::wstring filename = basename + L"_" + getLeadingZeroString(nextRecordingIndex) + L".s3rec";
		if (!FTX::FileSystem->exists(path + L"/" + filename))
			return filename;
		++nextRecordingIndex;
	}
	return basename;
}


PlayerRecorder::PlayerRecorder()
{
	mCurrentPlaybacks.reserve(5);
}

void PlayerRecorder::reset()
{
	mState = State::INACTIVE;
	mRecordingActive = false;

	mCurrentRecording.mFrames.clear();
	mCurrentPlaybacks.clear();
}

void PlayerRecorder::setEmulatorInterface(EmulatorInterface& emulatorInterface)
{
	mEmulatorInterface = &emulatorInterface;
}

void PlayerRecorder::setCurrentDirectory(const std::wstring& directory)
{
	mDirectory = directory;
}

void PlayerRecorder::initForDirectory(const std::wstring& directory, const std::wstring& recBaseFilename, uint16 zoneAndAct, uint8 category)
{
	const std::wstring filename = getUnusedRecordingFilename(directory, recBaseFilename);
	setCurrentDirectory(directory);
	initRecording(filename, zoneAndAct, category);

	// Load entries from the time attack table
	{
		TimeAttackData::Table* timeAttackTable = TimeAttackData::getTable(zoneAndAct, category);
		if (nullptr == timeAttackTable)
		{
			timeAttackTable = &TimeAttackData::loadTable(zoneAndAct, category, directory + L"/records.json");
		}
		for (TimeAttackData::Entry& entry : timeAttackTable->mEntries)
		{
			initPlayback(entry.mFilename);
		}
	}
}

void PlayerRecorder::initRecording(const std::wstring& filename, uint16 zoneAndAct, uint8 category)
{
	mState = State::WAITING;
	mRecordingActive = !filename.empty();

	mCurrentRecording.mFilename = filename;
	mCurrentRecording.mFormatVersion = CURRENT_FORMAT_VERSION;
	mCurrentRecording.mGameVersion = EngineMain::getDelegate().getAppMetaData().mBuildVersionNumber;
	mCurrentRecording.mZoneAndAct = zoneAndAct;
	mCurrentRecording.mCategory = category;
	mCurrentRecording.mFrames.clear();

	// Save settings
	const auto& settingsMap = SharedDatabase::getSettings();
	for (const auto& pair : settingsMap)
	{
		const SharedDatabase::Setting& setting = pair.second;
		const uint32 value = ConfigurationImpl::instance().mActiveGameSettings->getValue(pair.first);
		if (setting.mSerializationType != SharedDatabase::Setting::SerializationType::NONE && value != setting.mDefaultValue)
		{
			mCurrentRecording.mSettings.emplace_back(pair.first, value);
		}
	}
}

void PlayerRecorder::initPlayback(const std::wstring& filename)
{
	mState = State::WAITING;
	if (!filename.empty())
	{
		std::vector<uint8> buffer;
		if (FTX::FileSystem->readFile(mDirectory + L"/" + filename, buffer))
		{
			Recording& recording = vectorAdd(mCurrentPlaybacks);
			recording.mFilename = filename;
			recording.mVisible = ((int)mCurrentPlaybacks.size() <= mMaxGhosts);
			recording.mIndex = mCurrentPlaybacks.size() - 1;

			bool success = false;
			VectorBinarySerializer serializer(true, buffer);
			if (serializeRecording(serializer, recording))
			{
				success = !recording.mFrames.empty();
			}

			if (!success)
			{
				mCurrentPlaybacks.pop_back();
			}
		}
	}
}

void PlayerRecorder::onPostUpdateFrame()
{
	if (mState == State::INACTIVE)
		return;

	// Nothing to do outside of main game
	EmulatorInterface& emulatorInterface = *mEmulatorInterface;
	const bool isMainGame = (emulatorInterface.readMemory8(0xfffff600) == 0x0c);
	const bool hasStarted = ((int8)emulatorInterface.readMemory8(0xfffff711) > 0);

	if (isMainGame && hasStarted)
	{
		mState = State::ACTIVE;

		const uint16 frameNumber = emulatorInterface.readMemory16(0xfffffe04);

		// Recording
		if (mRecordingActive)
		{
			updateRecording(mCurrentRecording, frameNumber);
		}

		// Playback
		for (const Recording& recording : mCurrentPlaybacks)
		{
			updatePlayback(recording, frameNumber);
		}
	}
	else
	{
		// Switch to inactive if not still waiting for gameplay start
		if (mState == State::ACTIVE)
			mState = State::INACTIVE;
	}
}

bool PlayerRecorder::onTimeAttackFinish(int& hundreds, std::vector<int>& otherTimes)
{
	if (mState == State::INACTIVE)
		return false;

	if (!mRecordingActive)
		return false;

	mRecordingActive = false;
	mCurrentRecording.mTime = (uint32)mCurrentRecording.mFrames.size();

	hundreds = getHundreds(mCurrentRecording.mTime);

	// Sort recordings
	std::vector<Recording*> sortedRecordings;
	{
		sortedRecordings.push_back(&mCurrentRecording);
		for (Recording& recording : mCurrentPlaybacks)
		{
			sortedRecordings.push_back(&recording);
		}

		std::sort(sortedRecordings.begin(), sortedRecordings.end(),
				  [](Recording* a, Recording* b) { return (a->mTime < b->mTime); });
	}

	for (size_t i = 0; i < sortedRecordings.size(); ++i)
	{
		if (sortedRecordings[i] != &mCurrentRecording)
		{
			otherTimes.push_back(getHundreds(sortedRecordings[i]->mTime));
		}
	}

	// Apply ranks
	for (size_t i = 0; i < sortedRecordings.size(); ++i)
	{
		sortedRecordings[i]->mRank = (int)(i + 1);
	}

	// Save recording to file
	{
		std::vector<uint8> buffer;
		buffer.reserve(mCurrentRecording.mFrames.size() * sizeof(Frame) + 0x20);

		VectorBinarySerializer serializer(false, buffer);
		if (serializeRecording(serializer, mCurrentRecording))
		{
			FTX::FileSystem->saveFile(mDirectory + L"/" + mCurrentRecording.mFilename, buffer);
		}
	}

	// Update best times
	{
		TimeAttackData::Table& timeAttackTable = TimeAttackData::createTable(mCurrentRecording.mZoneAndAct, mCurrentRecording.mCategory);
		timeAttackTable.mEntries.clear();

		const size_t ranks = std::min<size_t>(sortedRecordings.size(), 5);
		for (size_t i = 0; i < ranks; ++i)
		{
			TimeAttackData::Entry& entry = vectorAdd(timeAttackTable.mEntries);
			Recording& recording = *sortedRecordings[i];
			entry.mFilename = recording.mFilename;
			entry.mTime = recording.mTime;
		}

		// Save updated JSON
		TimeAttackData::saveTable(mCurrentRecording.mZoneAndAct, mCurrentRecording.mCategory, mDirectory + L"/records.json");
	}

	return true;
}


void PlayerRecorder::updateRecording(Recording& recording, uint16 frameNumber)
{
	if (frameNumber == recording.mFrames.size())
	{
		EmulatorInterface& emulatorInterface = *mEmulatorInterface;

		recording.mFrames.emplace_back();
		Frame& frame = recording.mFrames.back();

		// Collect data
		frame.mInput = ControlsIn::instance().getGamepad(0).mCurrentInput;
		frame.mPosition.x = emulatorInterface.readMemory16(0xffffb010);
		frame.mPosition.y = emulatorInterface.readMemory16(0xffffb014);
		frame.mSprite = emulatorInterface.readMemory16(0x801002);
		frame.mRotation = emulatorInterface.readMemory8(0xffffb026);

		// Set flags accordingly
		frame.mFlags = (emulatorInterface.readMemory8(0xffffb004) & 0x03);
		if (emulatorInterface.readMemory16(0xffffb00a) & 0x8000)	{ frame.mFlags |= Frame::FLAG_PRIORITY; }
		if (emulatorInterface.readMemory8(0xffffb046) == 0x0e)		{ frame.mFlags |= Frame::FLAG_LAYER; }
		frame.mFlags |= getLevelSectionFlags(emulatorInterface, recording.mZoneAndAct);
	}
	else
	{
		RMX_CHECK(frameNumber == recording.mFrames.size() - 1, "Jump in frame number", );
	}
}

void PlayerRecorder::updatePlayback(const Recording& recording, uint16 frameNumber)
{
	if (!recording.mVisible || frameNumber >= recording.mFrames.size())
		return;

	EmulatorInterface& emulatorInterface = *mEmulatorInterface;

	const Frame& frame = recording.mFrames[frameNumber];
	int px = frame.mPosition.x - emulatorInterface.readMemory16(0xffffee80);
	int py = frame.mPosition.y - emulatorInterface.readMemory16(0xffffee84);

	// Level section fixes for AIZ 1 and ICZ 1
	if (recording.mZoneAndAct == 0x0000)
	{
		const uint8 currentSectionFlags = getLevelSectionFlags(emulatorInterface, recording.mZoneAndAct);
		if (frame.mFlags & 0x10)
		{
			px += 0x2f00;
			py += 0x80;
		}
		if (currentSectionFlags & 0x10)
		{
			px -= 0x2f00;
			py -= 0x80;
		}
	}
	else if (recording.mZoneAndAct == 0x0500)
	{
		const uint8 currentSectionFlags = getLevelSectionFlags(emulatorInterface, recording.mZoneAndAct);
		if (frame.mFlags & 0x10)
		{
			px += 0x6880;
			py -= 0x100;
		}
		if (currentSectionFlags & 0x10)
		{
			px -= 0x6880;
			py += 0x100;
		}
	}

	// Consider vertical level wrap
	if (emulatorInterface.readMemory16(0xffffee18) != 0)
	{
		const int levelHeightBitmask = emulatorInterface.readMemory16(0xffffeeaa);
		py &= levelHeightBitmask;
		if (py >= levelHeightBitmask / 2)
			py -= (levelHeightBitmask + 1);
	}

	const uint8 character = (recording.mCategory >> 4) - 1;
	const Vec2i velocity = (frameNumber > 0) ? (frame.mPosition - recording.mFrames[frameNumber-1].mPosition) : Vec2i(0, 1);
	s3air::drawPlayerSprite(emulatorInterface, character, Vec2i(px, py), velocity, frame.mSprite, frame.mFlags & 0x43, frame.mRotation, Color(1.5f, 1.5f, 1.5f, 0.65f), &frameNumber, Color(1.5f, 1.5f, 1.5f, 0.65f), 0x99990000 + recording.mIndex * 0x10);
}

bool PlayerRecorder::serializeRecording(VectorBinarySerializer& serializer, Recording& recording)
{
	// Identifier
	char identifier[9] = "S3AIRREC";
	serializer.serialize(identifier, 8);
	if (serializer.isReading())
	{
		if (memcmp(identifier, "S3AIRREC", 8) != 0)
			return false;
	}

	// Format version
	serializer & recording.mFormatVersion;
	const uint16 formatVersion = recording.mFormatVersion;
	if (serializer.isReading())
	{
		if (formatVersion < EARLIEST_FORMAT_VERSION || formatVersion > CURRENT_FORMAT_VERSION)
			return false;
	}

	// Game version
	serializer & recording.mGameVersion;

	// Settings
	if (formatVersion >= 0x0106)
	{
		serializer.serializeArraySize(recording.mSettings);
		for (auto& pair : recording.mSettings)
		{
			serializer.serialize(pair.first);
			serializer.serialize(pair.second);
		}
	}
	else
	{
		uint32 dummy = 0;
		serializer & dummy;
	}

	// Meta data
	serializer & recording.mZoneAndAct;
	serializer & recording.mCategory;

	if (formatVersion < 0x0104)
	{
		// Translate characters to category
		recording.mCategory = (recording.mCategory + 1) << 4;
	}

	// Number of frames
	if (serializer.isReading())
	{
		recording.mFrames.resize(serializer.read<uint32>());
		recording.mTime = (uint32)recording.mFrames.size();
	}
	else
	{
		serializer.writeAs<uint32>(recording.mFrames.size());
	}

	// Frames
	for (size_t i = 0; i < recording.mFrames.size(); ++i)
	{
		Frame& frame = recording.mFrames[i];
		serializer & frame.mInput;
		serializer.serializeAs<uint32>(frame.mPosition.x);
		serializer.serializeAs<uint32>(frame.mPosition.y);
		if (formatVersion < 0x0105)
		{
			serializer.serializeAs<uint8>(frame.mSprite);
		}
		else
		{
			serializer & frame.mSprite;
		}
		serializer & frame.mRotation;
		serializer & frame.mFlags;
	}

	return true;
}
