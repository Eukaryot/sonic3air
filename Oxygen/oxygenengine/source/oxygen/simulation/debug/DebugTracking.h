/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/DebuggingInterfaces.h"

class LemonScriptRuntime;
namespace lemon
{
	class ScriptFunction;
}


class DebugTracking final : public DebugNotificationInterface
{
friend class CodeExec;

public:
	struct Location
	{
		const lemon::ScriptFunction* mFunction = nullptr;
		size_t mProgramCounter = 0;
		mutable std::string mResolvedString;

		const std::string& toString(CodeExec& codeExec) const;
		bool operator==(const Location& other) const;
	};

	struct ScriptLogSingleEntry
	{
		std::string mValue;
		int mCallFrameIndex = -1;
	};
	struct ScriptLogEntry
	{
		std::vector<ScriptLogSingleEntry> mEntries;
		uint32 mLastUpdate = 0;
	};
	typedef std::map<std::string, ScriptLogEntry> ScriptLogEntryMap;

	struct ColorLogEntry
	{
		std::string mName;
		std::vector<Color> mColors;
	};
	typedef std::vector<ColorLogEntry> ColorLogEntryArray;

	struct Watch
	{
		struct Hit
		{
			uint32 mWrittenValue = 0;
			uint32 mAddress = 0;
			uint16 mBytes = 0;
			Location mLocation;
			int mCallFrameIndex = -1;
		};

		std::vector<Hit*> mHits;
		uint32 mAddress = 0;
		uint16 mBytes = 0;
		bool mPersistent = false;
		uint32 mInitialValue = 0;
		Location mLastHitLocation;
	};

	struct VRAMWrite
	{
		uint16 mAddress = 0;
		uint16 mSize = 0;
		Location mLocation;
		int mCallFrameIndex = -1;
	};

public:
	DebugTracking(CodeExec& codeExec, EmulatorInterface& emulatorInterface, LemonScriptRuntime& lemonScriptRuntime);

	void setupForDevMode();
	void clear();
	void onBeginFrame();

	// Debug logging
	inline const ScriptLogEntryMap& getScriptLogEntries() const  { return mScriptLogEntries; }
	void clearScriptLogValues();
	void clearScriptLogValue(const std::string& key);
	ScriptLogSingleEntry& updateScriptLogValue(std::string_view key, std::string_view value);

	inline const ColorLogEntryArray& getColorLogEntries() const { return mColorLogEntries; }
	void clearColorLogEntries();
	void addColorLogEntry(const ColorLogEntry& entry);
	void addColorLogEntry(std::string_view name, uint32 startAddress, uint8 numColors);

	// Debug watches
	inline const std::vector<Watch*>& getWatches() const  { return mWatches; }
	void updateWatches();
	void clearWatches(bool clearPersistent = false);
	void addWatch(uint32 address, uint16 bytes, bool persistent);
	void removeWatch(uint32 address, uint16 bytes);

	// VRAM writes
	inline const std::vector<VRAMWrite*>& getVRAMWrites() const  { return mVRAMWrites; }

private:
	void deleteWatch(Watch& watch);
	uint32 getCurrentWatchValue(uint32 address, uint16 bytes) const;
	int getCurrentCallFrameIndex() const;

private:
	// Interface implementations
	void onScriptLog(std::string_view key, std::string_view value) override;
	void onWatchTriggered(size_t watchIndex, uint32 address, uint16 bytes) override;
	void onVRAMWrite(uint16 address, uint16 bytes) override;

private:
	CodeExec&			mCodeExec;
	EmulatorInterface&	mEmulatorInterface;
	LemonScriptRuntime&	mLemonScriptRuntime;

	// Debug logging
	ScriptLogEntryMap mScriptLogEntries;
	ColorLogEntryArray mColorLogEntries;

	// Debug watches
	std::vector<Watch*> mWatches;
	std::vector<std::pair<Watch*, Watch::Hit*>> mWatchHitsThisUpdate;
	RentableObjectPool<Watch, 32> mWatchPool;
	RentableObjectPool<Watch::Hit, 32> mWatchHitPool;

	// VRAM writes
	std::vector<VRAMWrite*> mVRAMWrites;
	RentableObjectPool<VRAMWrite, 32> mVRAMWritePool;
};
