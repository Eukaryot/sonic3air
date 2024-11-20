/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/debug/DebugTracking.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/LemonScriptRuntime.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/application/Application.h"
#include "oxygen/rendering/parts/palette/PaletteManager.h"

#include <lemon/program/Function.h>
#include <lemon/runtime/RuntimeFunction.h>


const std::string& DebugTracking::Location::toString(CodeExec& codeExec) const
{
	if (mResolvedString.empty())
	{
		if (nullptr == mFunction)
		{
			mResolvedString = "<unable to resolve location>";
		}
		else
		{
			LemonScriptProgram::ResolvedLocation location;
			codeExec.getLemonScriptProgram().resolveLocation(location, *mFunction, mProgramCounter.has_value() ? (uint32)*mProgramCounter : 0);
			mLineNumber = location.mLineNumber;
			mResolvedString = mFunction->getName().getString();
		}
	}
	return mResolvedString;
}

bool DebugTracking::Location::operator==(const Location& other) const
{
	return (mFunction == other.mFunction && mProgramCounter == other.mProgramCounter);
}


DebugTracking::DebugTracking(CodeExec& codeExec, EmulatorInterface& emulatorInterface, LemonScriptRuntime& lemonScriptRuntime) :
	mCodeExec(codeExec),
	mEmulatorInterface(emulatorInterface),
	mLemonScriptRuntime(lemonScriptRuntime)
{
}

void DebugTracking::setupForDevMode()
{
	mVRAMWrites.reserve(0x800);
}

void DebugTracking::clear()
{
	clearScriptLogValues();
	clearColorLogEntries();
	clearWatches();
	mVRAMWritePool.clear();
	mVRAMWrites.clear();
}

void DebugTracking::onBeginFrame()
{
	// Reset script logging
	for (auto& [key, entry] : mScriptLogEntries)
	{
		for (ScriptLogSingleEntry& singleEntry : entry.mEntries)
		{
			singleEntry.mCallFrameIndex = -1;
		}
	}

	clearColorLogEntries();

	// Sanity check: Make sure no one else changed the emulator interface's watches
	RMX_ASSERT(mWatches.size() == mEmulatorInterface.getWatches().size(), "Watches got changed by someone");

	// Reset watch hits
	for (Watch* watch : mWatches)
	{
		watch->mInitialValue = getCurrentWatchValue(watch->mAddress, watch->mBytes);
		for (Watch::Hit* hit : watch->mHits)
			mWatchHitPool.returnObject(*hit);
		watch->mHits.clear();
		watch->mLastHitLocation = Location();
	}

	// Reset VRAM writes
	for (VRAMWrite* write : mVRAMWrites)
		mVRAMWritePool.returnObject(*write);
	mVRAMWrites.clear();
}

void DebugTracking::clearScriptLogValues()
{
	mScriptLogEntries.clear();
}

void DebugTracking::clearScriptLogValue(const std::string& key)
{
	mScriptLogEntries.erase(key);
}

DebugTracking::ScriptLogSingleEntry& DebugTracking::updateScriptLogValue(std::string_view key, std::string_view value)
{
	const uint32 frameNumber = Application::instance().getSimulation().getFrameNumber();
	ScriptLogEntry& entry = mScriptLogEntries[std::string(key)];
	if (frameNumber != entry.mLastUpdate)
	{
		entry.mEntries.clear();
		entry.mLastUpdate = frameNumber;
	}
	ScriptLogSingleEntry& singleEntry = vectorAdd(entry.mEntries);
	singleEntry.mValue = value;
	return singleEntry;
}

void DebugTracking::clearColorLogEntries()
{
	mColorLogEntries.clear();
}

void DebugTracking::addColorLogEntry(const ColorLogEntry& entry)
{
	mColorLogEntries.emplace_back(entry);
}

void DebugTracking::addColorLogEntry(std::string_view name, uint32 startAddress, uint8 numColors)
{
	EmulatorInterface& emulatorInterface = mCodeExec.getEmulatorInterface();

	DebugTracking::ColorLogEntry entry;
	entry.mName = name;
	entry.mColors.reserve(numColors);
	for (uint8 i = 0; i < numColors; ++i)
	{
		const uint16 packedColor = emulatorInterface.readMemory16(startAddress + i * 2);
		entry.mColors.push_back(PaletteManager::unpackColor(packedColor));
	}
	addColorLogEntry(entry);

	Application::instance().getSimulation().stopSingleStepContinue();
}

bool DebugTracking::hasWatch(uint32 address, uint16 bytes) const
{
	return (getExistingWatchIndex(address, bytes) >= 0);
}

int DebugTracking::getExistingWatchIndex(uint32 address, uint16 bytes) const
{
	address &= 0x00ffffff;

	int index = -1;
	for (int i = 0; i < (int)mWatches.size(); ++i)
	{
		if (mWatches[i]->mAddress == address && mWatches[i]->mBytes == bytes)
		{
			return i;
		}
	}
	return -1;
}

void DebugTracking::updateWatches()
{
	if (!mWatchHitsThisUpdate.empty())
	{
		for (auto& pair : mWatchHitsThisUpdate)
		{
			Watch& watch = *pair.first;
			Watch::Hit& hit = *pair.second;
			hit.mWrittenValue = (watch.mBytes <= 4) ? getCurrentWatchValue(watch.mAddress, watch.mBytes) : getCurrentWatchValue(hit.mAddress, hit.mBytes);
		}
		mWatchHitsThisUpdate.clear();
	}
}

void DebugTracking::clearWatches(bool clearPersistent)
{
	std::vector<std::pair<uint32, uint16>> reAddWatches;
	if (!clearPersistent)
	{
		// Save persistent watches
		reAddWatches.reserve(mWatches.size());
		for (const Watch* watch : mWatches)
		{
			if (watch->mPersistent)
			{
				reAddWatches.emplace_back(watch->mAddress, watch->mBytes);
			}
		}
	}

	for (Watch* watch : mWatches)
		deleteWatch(*watch);
	mWatches.clear();
	mEmulatorInterface.getWatches().clear();

	for (const auto& pair : reAddWatches)
	{
		addWatch(pair.first, pair.second, true);
	}
}

void DebugTracking::addWatch(uint32 address, uint16 bytes, bool persistent)
{
	address &= 0x00ffffff;

	// Check if already exists
	if (hasWatch(address, bytes))
		return;

	// Add a new watch in EmulatorInterface
	EmulatorInterface::Watch& internalWatch = vectorAdd(mEmulatorInterface.getWatches());
	internalWatch.mAddress = address;
	internalWatch.mBytes = bytes;

	// Add a new watch here
	Watch& watch = mWatchPool.rentObject();
	watch.mAddress = address;
	watch.mBytes = bytes;
	watch.mPersistent = persistent;
	watch.mInitialValue = getCurrentWatchValue(watch.mAddress, watch.mBytes);
	watch.mHits.clear();
	watch.mLastHitLocation = Location();
	mWatches.push_back(&watch);
}

void DebugTracking::removeWatch(uint32 address, uint16 bytes)
{
	address &= 0x00ffffff;

	// Try to find the watch
	const int index = getExistingWatchIndex(address, bytes);
	if (index == -1)
		return;

	// Remove it here
	deleteWatch(*mWatches[index]);
	mWatches.erase(mWatches.begin() + index);

	// Remove it in EmulatorInterface
	mEmulatorInterface.getWatches().erase(mEmulatorInterface.getWatches().begin() + index);
}

void DebugTracking::getCallStackFromCallFrameIndex(std::vector<Location>& outCallStack, int callFrameIndex, std::optional<size_t> firstProgramCounter)
{
	const std::vector<CodeExec::CallFrame>& callFrames = mCodeExec.getCallFrames();
	const uint8* lastCallingPC = nullptr;
	bool isFirst = true;
	while (callFrameIndex >= 0 && callFrameIndex < (int)callFrames.size())
	{
		const CodeExec::CallFrame& callFrame = callFrames[callFrameIndex];
		if (nullptr != callFrame.mFunction && callFrame.mFunction->getType() == lemon::Function::Type::SCRIPT)
		{
			Location& location = vectorAdd(outCallStack);
			location.mFunction = static_cast<const lemon::ScriptFunction*>(callFrame.mFunction);

			// TODO: Move this inside of a helper function, maybe inside LemonScriptRuntime?
			lemon::RuntimeFunction* runtimeFunction = mCodeExec.getLemonScriptRuntime().getInternalLemonRuntime().getRuntimeFunction(*location.mFunction);
			if (nullptr != runtimeFunction)
			{
				if (isFirst && firstProgramCounter.has_value())
				{
					location.mProgramCounter = *firstProgramCounter;
				}
				else
				{
					if (nullptr != lastCallingPC)
					{
						const int pc = runtimeFunction->translateFromRuntimeProgramCounterOptional(lastCallingPC);
						if (pc >= 0)
						{
							// The program counter refers to the opcode after the call, so we need to reduce it by 1
							location.mProgramCounter = (size_t)std::max(pc - 1, 0);
						}
					}
				}
			}
		}

		// Continue with parent
		callFrameIndex = callFrame.mParentIndex;
		lastCallingPC = callFrame.mCallingPC;
		isFirst = false;
	}
}

void DebugTracking::deleteWatch(Watch& watch)
{
	for (Watch::Hit* hit : watch.mHits)
		mWatchHitPool.returnObject(*hit);
	watch.mHits.clear();
	mWatchPool.returnObject(watch);
}

uint32 DebugTracking::getCurrentWatchValue(uint32 address, uint16 bytes) const
{
	switch (bytes)
	{
		case 1:  return mEmulatorInterface.readMemory8 (address);
		case 2:  return mEmulatorInterface.readMemory16(address);
		case 4:  return mEmulatorInterface.readMemory32(address);
		default: return 0;
	}
}

int DebugTracking::getCurrentCallFrameIndex() const
{
	const CodeExec::CallFrameTracking* tracking = mCodeExec.getActiveCallFrameTracking();
	if (nullptr == tracking || tracking->mCallStack.empty())
		return 0;
	return (int)tracking->mCallStack.back();
}

void DebugTracking::onScriptLog(std::string_view key, std::string_view value)
{
	ScriptLogSingleEntry& scriptLogSingleEntry = updateScriptLogValue(key, value);
	scriptLogSingleEntry.mCallFrameIndex = getCurrentCallFrameIndex();

	size_t pc;
	mLemonScriptRuntime.getLastStepLocation(scriptLogSingleEntry.mLocation.mFunction, pc);
	scriptLogSingleEntry.mLocation.mProgramCounter = pc;

	Application::instance().getSimulation().stopSingleStepContinue();
}

void DebugTracking::onWatchTriggered(size_t watchIndex, uint32 address, uint16 bytes)
{
	if (watchIndex >= mWatches.size())	// This may happen if a watch gets added and changed in the same frame
		return;

	Location location;
	size_t pc;
	mLemonScriptRuntime.getLastStepLocation(location.mFunction, pc);
	location.mProgramCounter = pc;

	Watch& watch = *mWatches[watchIndex];
	{
		// Add hit
		Watch::Hit& hit = mWatchHitPool.rentObject();
		hit.mWrittenValue = (watch.mBytes <= 4) ? getCurrentWatchValue(watch.mAddress, watch.mBytes) : getCurrentWatchValue(hit.mAddress, hit.mBytes);
		hit.mAddress = address;
		hit.mBytes = bytes;
		hit.mLocation = location;
		hit.mCallFrameIndex = getCurrentCallFrameIndex();
		watch.mHits.push_back(&hit);

		mWatchHitsThisUpdate.emplace_back(&watch, &hit);
		mLemonScriptRuntime.getInternalLemonRuntime().triggerStopSignal();
	}
	watch.mLastHitLocation = location;

	Application::instance().getSimulation().stopSingleStepContinue();
}

void DebugTracking::onVRAMWrite(uint16 address, uint16 bytes)
{
	// Not more than the limit
	if (mVRAMWrites.size() >= mVRAMWrites.capacity())
		return;

	Location location;
	size_t pc;
	mLemonScriptRuntime.getLastStepLocation(location.mFunction, pc);
	location.mProgramCounter = pc;

	// Check if this can be merged with the VRAM write just before
	if (!mVRAMWrites.empty())
	{
		VRAMWrite& other = *mVRAMWrites.back();
		if (other.mAddress + other.mSize == address && other.mLocation == location)
		{
			other.mSize += bytes;
			return;
		}
	}

	// Add a new VRAM write
	VRAMWrite& write = mVRAMWritePool.rentObject();
	write.mAddress = address;
	write.mSize = bytes;
	write.mLocation = location;
	write.mCallFrameIndex = getCurrentCallFrameIndex();
	mVRAMWrites.push_back(&write);
}
