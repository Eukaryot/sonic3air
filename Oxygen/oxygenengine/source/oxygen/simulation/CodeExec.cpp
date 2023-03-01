/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/platform/PlatformFunctions.h"

#include <lemon/program/Function.h>
#include <lemon/runtime/Runtime.h>


namespace
{
	const std::vector<GameProfile::LemonStackEntry>* getLemonStackByAsmStack(const std::vector<uint32>& asmStack)
	{
		// Try to find the right stack
		for (const GameProfile::StackLookupEntry& lookup : GameProfile::instance().mStackLookups)
		{
			if (lookup.mAsmStack.size() == asmStack.size())
			{
				bool equal = true;
				for (size_t i = 0; i < asmStack.size(); ++i)
				{
					if (lookup.mAsmStack[i] != asmStack[i])
					{
						equal = false;
						break;
					}
				}

				if (equal)
				{
					return &lookup.mLemonStack;
				}
			}
		}

		return nullptr;
	}

	const std::vector<GameProfile::LemonStackEntry>* findCurrentLemonStack(const std::vector<uint32>& asmStack)
	{
		// Try to find the right stack
		for (const GameProfile::StackLookupEntry& lookup : GameProfile::instance().mStackLookups)
		{
			if (lookup.mAsmStack.size() == asmStack.size())
			{
				bool equal = true;
				for (size_t i = 0; i < asmStack.size(); ++i)
				{
					if (lookup.mAsmStack[i] != asmStack[i])
					{
						equal = false;
						break;
					}
				}

				if (equal)
				{
					return &lookup.mLemonStack;
				}
			}
		}

		return nullptr;
	}
}


struct RuntimeExecuteConnector : public lemon::Runtime::ExecuteConnector
{
	CodeExec& mCodeExec;

	inline explicit RuntimeExecuteConnector(CodeExec& codeExec) : mCodeExec(codeExec) {}

	bool handleCall(const lemon::Function* func, uint64 callTarget) override
	{
		if (nullptr == func)
		{
			mCodeExec.showErrorWithScriptLocation("Call failed, probably due to invalid function (target = " + rmx::hexString(callTarget, 16) + ").");
			return false;
		}
		return true;
	}

	bool handleReturn() override
	{
		if (mCodeExec.mHasCallFramesToAdd)
		{
			mCodeExec.applyCallFramesToAdd();
		}
		return true;
	}

	bool handleExternalCall(uint64 address) override
	{
		// Check for address hook at the target address
		//  -> If it fails, we will just continue after the call
		mCodeExec.tryCallAddressHook((uint32)address);
		return true;
	}

	bool handleExternalJump(uint64 address) override
	{
		handleReturn();
		return handleExternalCall(address);
	}
};


struct RuntimeExecuteConnectorDev : public RuntimeExecuteConnector
{
	inline explicit RuntimeExecuteConnectorDev(CodeExec& codeExec) : RuntimeExecuteConnector(codeExec) {}

	bool handleCall(const lemon::Function* func, uint64 callTarget) override
	{
		if (nullptr == func)
		{
			mCodeExec.showErrorWithScriptLocation("Call failed, probably due to invalid function (target = " + rmx::hexString(callTarget, 16) + ").");
			return false;
		}
		if (func->getType() == lemon::Function::Type::SCRIPT)
		{
			CodeExec::CallFrame& callFrame = mCodeExec.mActiveCallFrameTracking->pushCallFrame(CodeExec::CallFrame::Type::SCRIPT_DIRECT);
			callFrame.mFunction = func;
		}
		return true;
	}

	bool handleReturn() override
	{
		mCodeExec.popCallFrame();
		return true;
	}

	bool handleExternalCall(uint64 address) override
	{
		// Check for address hook at the target address
		//  -> If it fails, we will just continue after the call
		mCodeExec.tryCallAddressHookDev((uint32)address);
		return true;
	}

	bool handleExternalJump(uint64 address) override
	{
		handleReturn();
		return handleExternalCall(address);
	}
};



const std::string& CodeExec::Location::toString(CodeExec& codeExec) const
{
	if (mResolvedString.empty())
	{
		if (nullptr == mFunction)
		{
			mResolvedString = "<unable to resolve location>";
		}
		else
		{
			std::string scriptFilename;
			uint32 lineNumber;
			codeExec.getLemonScriptProgram().resolveLocation(*mFunction, (uint32)mProgramCounter, scriptFilename, lineNumber);
			mResolvedString = std::string(mFunction->getName().getString()) + ", line " + std::to_string(lineNumber);
		}
	}
	return mResolvedString;
}

bool CodeExec::Location::operator==(const Location& other) const
{
	return (mFunction == other.mFunction && mProgramCounter == other.mProgramCounter);
}



CodeExec::CallFrame& CodeExec::CallFrameTracking::pushCallFrame(CallFrame::Type type)
{
	const int parentIndex = mCallStack.empty() ? -1 : mCallStack.back();
	CallFrame& callFrame = (mCallFrames.size() == CALL_FRAMES_LIMIT) ? mCallFrames.back() : vectorAdd(mCallFrames);
	callFrame.mType = type;
	callFrame.mParentIndex = parentIndex;
	callFrame.mDepth = (int)mCallStack.size();
	mCallStack.emplace_back(mCallFrames.size() - 1);
	return callFrame;
}

CodeExec::CallFrame& CodeExec::CallFrameTracking::pushCallFrameFailed(CallFrame::Type type)
{
	const int parentIndex = mCallFrames.empty() ? -1 : mCallStack.back();
	CallFrame& callFrame = (mCallFrames.size() == CALL_FRAMES_LIMIT) ? mCallFrames.back() : vectorAdd(mCallFrames);
	callFrame.mType = type;
	callFrame.mParentIndex = parentIndex;
	callFrame.mDepth = (int)mCallStack.size();
	return callFrame;
}

void CodeExec::CallFrameTracking::popCallFrame()
{
	if (mCallStack.empty())
		return;

	const uint32 steps = (uint32)mCallFrames[mCallStack.back()].mSteps;
	mCallStack.pop_back();

	if (!mCallStack.empty() && mCallStack.back() < mCallFrames.size())
	{
		// Accumulate steps of children
		mCallFrames[mCallStack.back()].mSteps += steps;
	}
}

void CodeExec::CallFrameTracking::writeCurrentCallStack(std::vector<uint64>& outCallStack)
{
	outCallStack.clear();
	outCallStack.reserve(mCallStack.size());
	for (int i = (int)mCallStack.size() - 1; i >= 0; --i)
	{
		const lemon::Function* function = mCallFrames[mCallStack[i]].mFunction;
		outCallStack.push_back((nullptr != function) ? function->getName().getHash() : 0);
	}
}

void CodeExec::CallFrameTracking::writeCurrentCallStack(std::vector<std::string>& outCallStack)
{
	outCallStack.clear();
	outCallStack.reserve(mCallStack.size());
	for (int i = (int)mCallStack.size() - 1; i >= 0; --i)
	{
		const lemon::Function* function = mCallFrames[mCallStack[i]].mFunction;
		outCallStack.push_back((nullptr != function) ? std::string(function->getName().getString()) : "");
	}
}

void CodeExec::CallFrameTracking::processCallFrames()
{
	for (size_t i = 0; i < mCallFrames.size(); )
	{
		i = processCallFramesRecursive(i);
	}
}

size_t CodeExec::CallFrameTracking::processCallFramesRecursive(size_t index)
{
	CallFrame& current = mCallFrames[index];
	current.mAnyChildFailed = (current.mType == CallFrame::Type::FAILED_HOOK);

	++index;
	while (index < mCallFrames.size())
	{
		CallFrame& child = mCallFrames[index];
		if (child.mDepth <= current.mDepth)
			break;

		index = processCallFramesRecursive(index);
		if (child.mAnyChildFailed)
		{
			current.mAnyChildFailed = true;
		}
	}
	return index;
}



CodeExec::CodeExec() :
	mLemonScriptProgram(*new LemonScriptProgram()),
	mEmulatorInterface(EngineMain::getDelegate().useDeveloperFeatures() ? *new EmulatorInterfaceDev() : *new EmulatorInterface()),
	mLemonScriptRuntime(*new LemonScriptRuntime(mLemonScriptProgram, mEmulatorInterface))
{
	mLemonScriptProgram.startup();
	mRuntimeEnvironment.mEmulatorInterface = &mEmulatorInterface;

	mIsDeveloperMode = EngineMain::getDelegate().useDeveloperFeatures();
	if (mIsDeveloperMode)
	{
		mLemonScriptProgram.getLemonScriptBindings().setDebugNotificationInterface(this);
		mEmulatorInterface.setDebugNotificationInterface(this);
		mMainCallFrameTracking.mCallFrames.reserve(CALL_FRAMES_LIMIT);
		mMainCallFrameTracking.mCallStack.reserve(0x40);
		mVRAMWrites.reserve(0x800);
	}
}

CodeExec::~CodeExec()
{
	delete &mEmulatorInterface;
	delete &mLemonScriptRuntime;
	delete &mLemonScriptProgram;
}

void CodeExec::startup()
{
	mEmulatorInterface.clear();
	mEmulatorInterface.applyRomInjections();	// Not asking the engine delegate here, as it might not give a meaningful answer yet; just assume it's okay
}

void CodeExec::reset()
{
	// Reset emulator interface
	mEmulatorInterface.clear();
	if (EngineMain::instance().getDelegate().allowModdedData())
	{
		mEmulatorInterface.applyRomInjections();
	}
}

void CodeExec::cleanScriptDebug()
{
	LogDisplay::instance().clearLogErrors();
	LogDisplay::instance().clearScriptLogValues();
	LogDisplay::instance().clearColorLogEntries();

	mMainCallFrameTracking.clear();
	mUnknownAddressesSet.clear();
	mUnknownAddressesInOrder.clear();
	clearWatches();
	mVRAMWritePool.clear();
	mVRAMWrites.clear();
}

bool CodeExec::reloadScripts(bool enforceFullReload, bool retainRuntimeState)
{
	if (retainRuntimeState)
	{
		// If the runtime is already active, save its current state
		if (hasValidState() && mLemonScriptRuntime.getCallStackSize() != 0)
		{
			VectorBinarySerializer serializer(false, mSerializedRuntimeState);
			if (!getLemonScriptRuntime().serializeRuntime(serializer))
			{
				mSerializedRuntimeState.clear();
			}
		}
	}
	else
	{
		// Clear the old serialization, it's not needed
		mSerializedRuntimeState.clear();
	}
	mExecutionState = ExecutionState::INACTIVE;

	const Configuration& config = Configuration::instance();
	LemonScriptProgram::LoadOptions options;
	options.mEnforceFullReload = enforceFullReload;
	options.mModuleSelection = EngineMain::getDelegate().mayLoadScriptMods() ? LemonScriptProgram::LoadOptions::ModuleSelection::ALL_MODS : LemonScriptProgram::LoadOptions::ModuleSelection::BASE_GAME_ONLY;
	options.mAppVersion = EngineMain::getDelegate().getAppMetaData().mBuildVersionNumber;
	const WString mainScriptPath = config.mScriptsDir + config.mMainScriptName;

	const LemonScriptProgram::LoadScriptsResult result = mLemonScriptProgram.loadScripts(mainScriptPath.toStdString(), options);
	if (result == LemonScriptProgram::LoadScriptsResult::PROGRAM_CHANGED)
	{
		mLemonScriptRuntime.onProgramUpdated();
	}
	cleanScriptDebug();

	return (result != LemonScriptProgram::LoadScriptsResult::FAILED);
}

void CodeExec::restoreRuntimeState(bool hasSaveState)
{
	if (mSerializedRuntimeState.empty())
	{
		// We don't have a valid runtime state, so it has to be reloaded from ASM, or we have to reset
		reinitRuntime(nullptr, hasSaveState ? CallStackInitPolicy::READ_FROM_ASM : CallStackInitPolicy::RESET);
	}
	else
	{
		// Scripts got reloaded in-game
		reinitRuntime(nullptr, CallStackInitPolicy::READ_FROM_ASM, &mSerializedRuntimeState);
		mSerializedRuntimeState.clear();	// Not needed any more now
	}
}

void CodeExec::reinitRuntime(const LemonScriptRuntime::CallStackWithLabels* enforcedCallStack, CallStackInitPolicy callStackInitPolicy, const std::vector<uint8>* serializedRuntimeState)
{
	cleanScriptDebug();

	if (callStackInitPolicy == CallStackInitPolicy::USE_EXISTING)
	{
		// Nothing to do in this case, call stack was already loaded
		RMX_ASSERT(nullptr == enforcedCallStack, "Can't use existing call stack and an enforced call stack at the same time");
	}
	else
	{
		// The runtime requires a program
		if (!mLemonScriptRuntime.hasValidProgram())
			return;

		bool success = false;
		if (serializedRuntimeState != nullptr && !serializedRuntimeState->empty())
		{
			VectorBinarySerializer serializer(true, *serializedRuntimeState);
			success = getLemonScriptRuntime().serializeRuntime(serializer);
		}

		mLemonScriptRuntime.getInternalLemonRuntime().clearAllControlFlows();

		if (enforcedCallStack != nullptr && !enforcedCallStack->empty())
		{
			for (const auto& pair : *enforcedCallStack)
			{
				success = mLemonScriptRuntime.callFunctionByNameAtLabel(pair.first, pair.second, true);
				RMX_CHECK(success, "Could not apply previous lemon script stack", );
			}
		}

		// Scan asm call stack if needed
		if (!success && callStackInitPolicy == CallStackInitPolicy::READ_FROM_ASM)
		{
			std::vector<uint32> callstack;
			uint32 stackPointer = mEmulatorInterface.getRegister(EmulatorInterface::Register::A7);
			RMX_CHECK((stackPointer & 0x00ff0000) == 0x00ff0000, "Stack pointer in register A7 is not pointing to a RAM address", );
			stackPointer |= 0xffff0000;
			RMX_CHECK(stackPointer >= GameProfile::instance().mAsmStackRange.first && stackPointer <= GameProfile::instance().mAsmStackRange.second, "Stack pointer in register A7 is not inside the ASM stack range", );
			while (stackPointer < GameProfile::instance().mAsmStackRange.second)
			{
				callstack.push_back(mEmulatorInterface.readMemory32(stackPointer));
				stackPointer += 4;
			}

			std::reverse(callstack.begin(), callstack.end());

			// Build up initial script call stack
			const std::vector<GameProfile::LemonStackEntry>* lemonStack = getLemonStackByAsmStack(callstack);
			if (nullptr != lemonStack)
			{
				for (const GameProfile::LemonStackEntry& entry : *lemonStack)
				{
					success = mLemonScriptRuntime.callFunctionByNameAtLabel(entry.mFunctionName, entry.mLabelName, true);
					RMX_CHECK(success, "Could not apply lemon script stack", );
				}
			}
			else
			{
				std::string str;
				for (uint32 i : callstack)
					str += " " + rmx::hexString(i, 8, "");
				RMX_ERROR("Save state stack could not be represented in lemon script:\n" + str, );
			}
		}

		// Fallback if loading went wrong or there is no save state at all
		if (!success || mLemonScriptRuntime.getCallStackSize() == 0)
		{
			// Start from scratch
			mLemonScriptRuntime.callFunctionByName("scriptMainEntryPoint", true);
		}
	}

	// Execute init once
	mLemonScriptRuntime.callFunctionByName("Init", false);

	EngineMain::getDelegate().onRuntimeInit(*this);

	mExecutionState = ExecutionState::READY;
	mAccumulatedStepsOfCurrentFrame = 0;
}

bool CodeExec::performFrameUpdate()
{
	if (!canExecute())
		return false;

	lemon::Runtime::setActiveEnvironment(&mRuntimeEnvironment);

	const bool beginningNewFrame = (mExecutionState != ExecutionState::INTERRUPTED);
	if (beginningNewFrame)
	{
		mAccumulatedStepsOfCurrentFrame = 0;

		if (mIsDeveloperMode)
		{
			// Reset debug draws etc.
			LogDisplay::instance().clearColorLogEntries();

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

			// Reset call frame tracking
			mMainCallFrameTracking.clear();

			static std::vector<const lemon::Function*> callstack;	// This is static to avoid reallocations
			mLemonScriptRuntime.getCallStack(callstack);
			for (const lemon::Function* func : callstack)
			{
				CallFrame& callFrame = mMainCallFrameTracking.pushCallFrame(CallFrame::Type::SCRIPT_STACK);
				callFrame.mFunction = func;
			}
		}

		// Perform pre-update hook, if there is one
		//  -> This acts like a call from wherever the last script execution stopped / yielded
		tryCallUpdateHook(false);
	}

	// Run script
	runScript(false, &mMainCallFrameTracking);

	const bool completedNewFrame = (mExecutionState == ExecutionState::YIELDED);
	if (completedNewFrame)
	{
		// Perform post-update hook, if there is one
		//  -> Note that the hook must yield execution, otherwise parts of the next frame get executed
		if (canExecute() && tryCallUpdateHook(true))
		{
			runScript(true, &mMainCallFrameTracking);
		}
		mAccumulatedStepsOfCurrentFrame = 0;
	}

	// Return whether the frame was completed in any way (halted counts as completed)
	return (mExecutionState != ExecutionState::INTERRUPTED);
}

void CodeExec::yieldExecution()
{
	mCurrentlyRunningScript = false;
	mLemonScriptRuntime.getInternalLemonRuntime().triggerStopSignal();
}

bool CodeExec::executeScriptFunction(const std::string& functionName, bool showErrorOnFail)
{
	if (canExecute())
	{
		// TODO: This would be a good use case for using a different control flow than the main one
		lemon::Runtime::setActiveEnvironment(&mRuntimeEnvironment);

		if (mLemonScriptRuntime.callFunctionByName(functionName, showErrorOnFail))
		{
			const size_t oldAccumulatedSteps = mAccumulatedStepsOfCurrentFrame;

		#if 0
			// Dead code, as call frame tracking is always disabled for single script function calls
			//  -> However, this might change later on, e.g. if we had tracking for multiple control flows individually (see TODO above)
			CallFrameTracking* tracking = nullptr;
			if (nullptr != tracking)
			{
				const size_t originalNumCallFrames = tracking->mCallFrames.size();

				CallFrame& callFrame = tracking->pushCallFrame(CallFrame::Type::SCRIPT_STACK);
				callFrame.mFunction = mLemonScriptRuntime.getCurrentFunction();
				runScript(true, tracking);

				// Revert call frames from that call
				tracking->mCallFrames.resize(originalNumCallFrames);
			}
			else
		#endif
			{
				runScript(true, nullptr);
			}

			// Revert accumulated steps
			//  -> This kind of script function execution should not count against the accumulator, as we might execute functions while the game is paused (i.e. the accumulator won't get reset)
			mAccumulatedStepsOfCurrentFrame = oldAccumulatedSteps;
			return true;
		}
	}
	return false;
}

void CodeExec::setupCallFrame(std::string_view functionName, std::string_view labelName)
{
	mCallFramesToAdd.emplace_back(functionName, labelName);
	mHasCallFramesToAdd = true;
}

void CodeExec::processCallFrames()
{
	mMainCallFrameTracking.processCallFrames();
}

void CodeExec::getCallStackFromCallFrameIndex(std::vector<uint64>& outCallStack, int callFrameIndex)
{
	while (callFrameIndex >= 0 && callFrameIndex < (int)mMainCallFrameTracking.mCallFrames.size())
	{
		CallFrame& callFrame = mMainCallFrameTracking.mCallFrames[callFrameIndex];
		if (nullptr != callFrame.mFunction)
		{
			outCallStack.push_back(callFrame.mFunction->getName().getHash());
		}

		// Continue with parent
		callFrameIndex = callFrame.mParentIndex;
	}
}

void CodeExec::clearWatches(bool clearPersistent)
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

void CodeExec::addWatch(uint32 address, uint16 bytes, bool persistent)
{
	address &= 0x00ffffff;

	// Check if already exists
	for (const Watch* watch : mWatches)
	{
		if (watch->mAddress == address && watch->mBytes == bytes)
			return;
	}

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

void CodeExec::removeWatch(uint32 address, uint16 bytes)
{
	address &= 0x00ffffff;

	// Try to find the watch
	int index = -1;
	for (int i = 0; i < (int)mWatches.size(); ++i)
	{
		if (mWatches[i]->mAddress == address && mWatches[i]->mBytes == bytes)
		{
			index = i;
			break;
		}
	}
	if (index == -1)
		return;

	// Remove it here
	deleteWatch(*mWatches[index]);
	mWatches.erase(mWatches.begin() + index);

	// Remove it in EmulatorInterface
	mEmulatorInterface.getWatches().erase(mEmulatorInterface.getWatches().begin() + index);
}

bool CodeExec::canExecute() const
{
	switch (mExecutionState)
	{
		case ExecutionState::READY:
		case ExecutionState::YIELDED:
		case ExecutionState::INTERRUPTED:
			return true;

		case ExecutionState::INACTIVE:
		case ExecutionState::HALTED:
		case ExecutionState::ERROR:
		default:
			return false;
	}
}

bool CodeExec::hasValidState() const
{
	switch (mExecutionState)
	{
		case ExecutionState::YIELDED:
		case ExecutionState::INTERRUPTED:
			return true;

		case ExecutionState::INACTIVE:
		case ExecutionState::READY:
		case ExecutionState::HALTED:
		case ExecutionState::ERROR:
		default:
			return false;
	}
}

void CodeExec::runScript(bool executeSingleFunction, CallFrameTracking* callFrameTracking)
{
	// There are four stop conditions:
	//  a) Yield from script, sets mCurrentlyRunningScript to false     -> this is the usual case if executeSingleFunction == false (but must not happen otherwise)
	//  b) Returned from function that is initially on top of the stack -> this is the usual case if executeSingleFunction == true (and can't happen otherwise)
	//  c) Reached runtime steps limit, i.e. script got stuck in a loop
	//  d) Script stopped because its runtime stack was completely emptied
	if (!canExecute())
		return;

	mActiveInstance = this;
	mCurrentlyRunningScript = true;
	const size_t abortOnCallStackSize = executeSingleFunction ? (std::max<size_t>(mLemonScriptRuntime.getCallStackSize(), 1) - 1) : 0;
	mActiveCallFrameTracking = mIsDeveloperMode ? callFrameTracking : nullptr;

	size_t stepsCounter = 0;
	size_t nextCheckSteps = 0x40000;
	const uint32 ticksStart = SDL_GetTicks();

	while (true)
	{
		// Execute next runtime steps
		size_t stepsExecutedThisCall;
		try
		{
			const bool success = (nullptr != mActiveCallFrameTracking) ? executeRuntimeStepsDev(stepsExecutedThisCall, abortOnCallStackSize) : executeRuntimeSteps(stepsExecutedThisCall, abortOnCallStackSize);
			if (!success)
			{
				if (executeSingleFunction)
				{
					// Execution of function finished
					mExecutionState = ExecutionState::YIELDED;
				}
				else
				{
					RMX_ERROR("Script execution completely stopped because call stack was emptied", );
					mExecutionState = ExecutionState::HALTED;
				}
				break;
			}

			if (!mCurrentlyRunningScript)
			{
				// Execution got yielded -- this is the usual way to end a frame, i.e. with executeSingleFunction == false
				mExecutionState = ExecutionState::YIELDED;
				RMX_CHECK(!executeSingleFunction, "Single function script execution got yielded; don't use yieldExecution inside functions that get called directly from the engine", );
				break;
			}

			if (executeSingleFunction && mLemonScriptRuntime.getCallStackSize() <= abortOnCallStackSize)
			{
				// Execution of function finished
				mExecutionState = ExecutionState::YIELDED;
				break;
			}
		}
		catch (const std::exception& e)
		{
			RMX_ERROR("Caught exception during script execution: " << e.what(), );
		}

		// Regularly check if we should better interrupt execution
		stepsCounter += stepsExecutedThisCall;
		if (stepsCounter >= nextCheckSteps)
		{
			// Limit execution to this number of steps
			const constexpr int32 MAX_STEPS = 0x8000000;	// Needed for S3AIR entering special stage in OxygenApp
			if (mAccumulatedStepsOfCurrentFrame + stepsCounter >= MAX_STEPS)
			{
				mExecutionState = ExecutionState::INTERRUPTED;

				// Show a message box (but only once)
				static bool showMessageBox = true;
				if (showMessageBox)
				{
					bool gameRecordingSaved = false;
					if (Configuration::instance().mGameRecorder.mIsRecording)
					{
						gameRecordingSaved = (Application::instance().getSimulation().saveGameRecording() != 0);
					}

					showErrorWithScriptLocation("Reached limit for runtime steps per update; if this happens, the program probably got stuck in a loop.", gameRecordingSaved ? "A game recording file was written that could be helpful for debugging this issue." : "");
					showMessageBox = false;
				}
				break;
			}

			// Limit execution to 100 ms
			if (SDL_GetTicks() - ticksStart >= 100 && !PlatformFunctions::isDebuggerPresent())
			{
				mExecutionState = ExecutionState::INTERRUPTED;
				break;
			}

			nextCheckSteps += 0x20000;
		}
	}

	mAccumulatedStepsOfCurrentFrame += stepsCounter;
	mCurrentlyRunningScript = false;
	mActiveInstance = nullptr;
}

bool CodeExec::executeRuntimeSteps(size_t& stepsExecuted, size_t minimumCallStackSize)
{
	lemon::Runtime& runtime = mLemonScriptRuntime.getInternalLemonRuntime();
	RuntimeExecuteConnector connector(*this);
	runtime.executeSteps(connector, 5000, minimumCallStackSize);

	stepsExecuted = connector.mStepsExecuted;
	return (connector.mResult != lemon::Runtime::ExecuteResult::Result::HALT);
}

bool CodeExec::executeRuntimeStepsDev(size_t& stepsExecuted, size_t minimumCallStackSize)
{
	// Same as "executeRuntimeSteps", but with additional developer mode stuff, incl. tracking of call frames
	if (mActiveCallFrameTracking->mCallFrames.empty())
		return false;

	lemon::Runtime& runtime = mLemonScriptRuntime.getInternalLemonRuntime();
	RuntimeExecuteConnectorDev connector(*this);
	runtime.executeSteps(connector, 5000, minimumCallStackSize);

	{
		mActiveCallFrameTracking->mCallFrames.back().mSteps += connector.mStepsExecuted;

		// Correct written values for all watches that triggered in this update
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

	stepsExecuted = connector.mStepsExecuted;
	return (connector.mResult != lemon::Runtime::ExecuteResult::Result::HALT);
}

bool CodeExec::tryCallAddressHook(uint32 address)
{
	return mLemonScriptRuntime.callAddressHook(address);
}

bool CodeExec::tryCallAddressHookDev(uint32 address)
{
	if (mLemonScriptRuntime.callAddressHook(address))
	{
		// Call script function
		CallFrame& callFrame = mActiveCallFrameTracking->pushCallFrame(CallFrame::Type::SCRIPT_HOOK);
		callFrame.mFunction = mLemonScriptRuntime.getCurrentFunction();
		callFrame.mAddress = address;
		return true;
	}
	else
	{
		CallFrame& callFrame = mActiveCallFrameTracking->pushCallFrameFailed(CallFrame::Type::FAILED_HOOK);
		callFrame.mAddress = address;

		if (mUnknownAddressesSet.count(address) == 0)
		{
			mUnknownAddressesSet.insert(address);
			mUnknownAddressesInOrder.push_back(address);
		}
		return false;
	}
}

bool CodeExec::tryCallUpdateHook(bool postUpdate)
{
	if (mLemonScriptRuntime.callUpdateHook(postUpdate))
	{
		if (nullptr != mActiveCallFrameTracking)
		{
			CallFrame& callFrame = mActiveCallFrameTracking->pushCallFrame(CallFrame::Type::SCRIPT_DIRECT);
			callFrame.mFunction = mLemonScriptRuntime.getCurrentFunction();
		}
		return true;
	}
	return false;
}

void CodeExec::applyCallFramesToAdd()
{
	// Add call frames as defined via script function "System.insertOuterCallFrame"
	for (const auto& pair : mCallFramesToAdd)
	{
		const bool success = mLemonScriptRuntime.callFunctionByNameAtLabel(pair.first, pair.second, true);
		RMX_CHECK(success, "Could not insert outer call frame", continue);

		if (nullptr != mActiveCallFrameTracking)
		{
			CallFrame& callFrame = mActiveCallFrameTracking->pushCallFrame(CallFrame::Type::SCRIPT_STACK);
			callFrame.mFunction = mLemonScriptRuntime.getCurrentFunction();
		}
	}
	mCallFramesToAdd.clear();
	mHasCallFramesToAdd = false;
}

void CodeExec::popCallFrame()
{
	mActiveCallFrameTracking->popCallFrame();

	if (mHasCallFramesToAdd)
	{
		applyCallFramesToAdd();
	}
}

uint32 CodeExec::getCurrentWatchValue(uint32 address, uint16 bytes) const
{
	switch (bytes)
	{
		case 1:  return mEmulatorInterface.readMemory8 (address);
		case 2:  return mEmulatorInterface.readMemory16(address);
		case 4:  return mEmulatorInterface.readMemory32(address);
		default: return 0;
	}
}

void CodeExec::deleteWatch(Watch& watch)
{
	for (Watch::Hit* hit : watch.mHits)
		mWatchHitPool.returnObject(*hit);
	watch.mHits.clear();
	mWatchPool.returnObject(watch);
}

void CodeExec::showErrorWithScriptLocation(const std::string& errorText, const std::string& subText)
{
	std::string locationString = mLemonScriptRuntime.getOwnCurrentScriptLocationString();
	if (locationString.empty())
	{
		RMX_ERROR(errorText << "\n" << subText, );
	}
	else
	{
		RMX_ERROR(errorText << "\nIn " << locationString << "." << (subText.empty() ? "" : "\n") << subText, );
	}
}

void CodeExec::onWatchTriggered(size_t watchIndex, uint32 address, uint16 bytes)
{
	if (watchIndex >= mWatches.size())	// This may happen if a watch gets added and changed in the same frame
		return;

	Location location;
	mLemonScriptRuntime.getLastStepLocation(location.mFunction, location.mProgramCounter);

	Watch& watch = *mWatches[watchIndex];
	{
		// Add hit
		Watch::Hit& hit = mWatchHitPool.rentObject();
		hit.mWrittenValue = (watch.mBytes <= 4) ? getCurrentWatchValue(watch.mAddress, watch.mBytes) : getCurrentWatchValue(hit.mAddress, hit.mBytes);
		hit.mAddress = address;
		hit.mBytes = bytes;
		hit.mLocation = location;
		watch.mHits.push_back(&hit);

		if (nullptr != mActiveCallFrameTracking)
			hit.mCallFrameIndex = (int)mActiveCallFrameTracking->mCallFrames.size() - 1;
		mWatchHitsThisUpdate.emplace_back(&watch, &hit);
	}
	watch.mLastHitLocation = location;

	Application::instance().getSimulation().stopSingleStepContinue();
}

void CodeExec::onVRAMWrite(uint16 address, uint16 bytes)
{
	// Not more than the limit
	if (mVRAMWrites.size() >= mVRAMWrites.capacity())
		return;

	Location location;
	mLemonScriptRuntime.getLastStepLocation(location.mFunction, location.mProgramCounter);

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
	if (nullptr != mActiveCallFrameTracking)
		write.mCallFrameIndex = (int)mActiveCallFrameTracking->mCallFrames.size() - 1;
	mVRAMWrites.push_back(&write);
}

void CodeExec::onLog(LogDisplay::ScriptLogSingleEntry& scriptLogSingleEntry)
{
	if (nullptr != mActiveCallFrameTracking)
		scriptLogSingleEntry.mCallFrameIndex = (int)mActiveCallFrameTracking->mCallFrames.size() - 1;
}
