/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/DebuggingInterfaces.h"
#include "oxygen/simulation/LemonScriptRuntime.h"
#include "oxygen/simulation/RuntimeEnvironment.h"

class EmulatorInterface;
namespace lemon
{
	class Environment;
	class ScriptFunction;
}


class CodeExec final : public DebugNotificationInterface
{
friend struct RuntimeExecuteConnector;
friend struct RuntimeExecuteConnectorDev;

public:
	enum class ExecutionState
	{
		INACTIVE	= 0,	// Scripts did not get initialized yet
		READY		= 1,	// Execution has not started yet, but is ready to start
		YIELDED		= 2,	// The usual state when frame execution was successfully completed
		INTERRUPTED	= 3,	// Pausing of execution when it took too long
		HALTED		= 4,	// Stopped due to empty call stack or other script halt
		ERROR		= 5		// Execution is impossible, e.g. due to a compile error
	};

	enum class CallStackInitPolicy
	{
		RESET,
		USE_EXISTING,
		READ_FROM_ASM
	};

	struct Location
	{
		const lemon::ScriptFunction* mFunction = nullptr;
		size_t mProgramCounter = 0;
		mutable std::string mResolvedString;

		const std::string& toString(CodeExec& codeExec) const;
		bool operator==(const Location& other) const;
	};

	static const constexpr size_t CALL_FRAMES_LIMIT = 0x1000;

	struct CallFrame
	{
		enum class Type
		{
			SCRIPT_STACK,
			SCRIPT_DIRECT,
			SCRIPT_HOOK,
			FAILED_HOOK
		};

		Type mType;
		const lemon::Function* mFunction = nullptr;
		uint32 mAddress = 0xffffffff;
		int mParentIndex = -1;
		int mDepth = 0;
		size_t mSteps = 0;
		bool mAnyChildFailed = false;
	};

	struct CallFrameTracking
	{
		std::vector<CallFrame> mCallFrames;
		std::vector<size_t> mCallStack;			// Uses index in mCallFrames

		inline void clear()  { mCallFrames.clear(); mCallStack.clear(); }

		CallFrame& pushCallFrame(CallFrame::Type type);
		CallFrame& pushCallFrameFailed(CallFrame::Type type);
		void popCallFrame();
		void writeCurrentCallStack(std::vector<uint64>& outCallStack);
		void writeCurrentCallStack(std::vector<std::string>& outCallStack);
		void processCallFrames();
		size_t processCallFramesRecursive(size_t index);
	};

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
	static inline CodeExec* getActiveInstance() { return mActiveInstance; }

public:
	CodeExec();
	~CodeExec();

	void startup();
	void reset();

	void cleanScriptDebug();
	bool reloadScripts(bool enforceFullReload, bool retainRuntimeState);
	void restoreRuntimeState(bool hasSaveState);
	void reinitRuntime(const LemonScriptRuntime::CallStackWithLabels* enforcedCallStack, CallStackInitPolicy callStackInitPolicy, const std::vector<uint8>* serializedRuntimeState = nullptr);

	inline ExecutionState getExecutionState() const  { return mExecutionState; }
	inline bool isCodeExecutionPossible() const { return canExecute(); }
	inline bool willBeginNewFrame() const { return canExecute() && (mExecutionState != ExecutionState::INTERRUPTED); }

	bool performFrameUpdate();
	void yieldExecution();

	bool executeScriptFunction(const std::string& functionName, bool showErrorOnFail);

	inline EmulatorInterface& getEmulatorInterface()	{ return mEmulatorInterface; }
	inline LemonScriptRuntime& getLemonScriptRuntime()	{ return mLemonScriptRuntime; }
	inline LemonScriptProgram& getLemonScriptProgram()	{ return mLemonScriptProgram; }

	void setupCallFrame(std::string_view functionName, std::string_view labelName = "");

	void processCallFrames();
	inline const std::vector<CallFrame>& getCallFrames() const  { return mMainCallFrameTracking.mCallFrames; }
	void getCallStackFromCallFrameIndex(std::vector<uint64>& outCallStack, int callFrameIndex);

	inline const std::vector<uint32>& getUnknownAddresses() const  { return mUnknownAddressesInOrder; }

	inline const std::vector<Watch*>& getWatches() const  { return mWatches; }
	void clearWatches(bool clearPersistent = false);
	void addWatch(uint32 address, uint16 bytes, bool persistent);
	void removeWatch(uint32 address, uint16 bytes);

	inline const std::vector<VRAMWrite*>& getVRAMWrites() const  { return mVRAMWrites; }

private:
	bool canExecute() const;
	bool hasValidState() const;
	void runScript(bool executeSingleFunction, CallFrameTracking* callFrameTracking);

	bool executeRuntimeSteps(size_t& stepsExecuted, size_t minimumCallStackSize);
	bool executeRuntimeStepsDev(size_t& stepsExecuted, size_t minimumCallStackSize);

	bool tryCallAddressHook(uint32 address);
	bool tryCallAddressHookDev(uint32 address);
	bool tryCallUpdateHook(bool postUpdate);

	void applyCallFramesToAdd();

	void popCallFrame();

	uint32 getCurrentWatchValue(uint32 address, uint16 bytes) const;
	void deleteWatch(Watch& watch);

	void showErrorWithScriptLocation(const std::string& errorText, const std::string& subText = "");

private:
	// Interface implementations
	void onWatchTriggered(size_t watchIndex, uint32 address, uint16 bytes) override;
	void onVRAMWrite(uint16 address, uint16 bytes) override;
	void onLog(LogDisplay::ScriptLogSingleEntry& scriptLogSingleEntry) override;

private:
	// Order of these three instances is important, as we got a clear dependency chain here
	LemonScriptProgram& mLemonScriptProgram;	// Move instance to Simulation?
	EmulatorInterface&  mEmulatorInterface;
	LemonScriptRuntime& mLemonScriptRuntime;
	RuntimeEnvironment mRuntimeEnvironment;

	bool mIsDeveloperMode = false;
	ExecutionState mExecutionState = ExecutionState::INACTIVE;
	bool mCurrentlyRunningScript = false;
	size_t mAccumulatedStepsOfCurrentFrame = 0;

	CallFrameTracking* mActiveCallFrameTracking = nullptr;	// If this a null pointer, then no tracking is active
	CallFrameTracking mMainCallFrameTracking;

	LemonScriptRuntime::CallStackWithLabels mCallFramesToAdd;
	bool mHasCallFramesToAdd = false;

	std::vector<uint8> mSerializedRuntimeState;		// Only stored if script reloading failed

	std::set<uint32> mUnknownAddressesSet;
	std::vector<uint32> mUnknownAddressesInOrder;

	std::vector<Watch*> mWatches;
	std::vector<std::pair<Watch*, Watch::Hit*>> mWatchHitsThisUpdate;
	RentableObjectPool<Watch, 32> mWatchPool;
	RentableObjectPool<Watch::Hit, 32> mWatchHitPool;

	std::vector<VRAMWrite*> mVRAMWrites;
	RentableObjectPool<VRAMWrite, 32> mVRAMWritePool;

private:
	static inline CodeExec* mActiveInstance = nullptr;
};
