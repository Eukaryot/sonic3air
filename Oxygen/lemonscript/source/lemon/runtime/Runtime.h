/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/StringRef.h"
#include "lemon/runtime/ControlFlow.h"


namespace lemon
{
	class Function;
	class Program;
	class NativeFunction;
	class Variable;
	struct RuntimeOpcode;


	class API_EXPORT MemoryAccessHandler
	{
	public:
		struct SpecializationResult
		{
			enum Result
			{
				NO_SPECIALIZATION,
				HAS_SPECIALIZATION,
				INVALID_ACCESS
			};

			Result mResult = Result::NO_SPECIALIZATION;
			uint8* mDirectAccessPointer = nullptr;
			bool mSwapBytes = false;
		};

		virtual uint8  read8 (uint64 address) = 0;
		virtual uint16 read16(uint64 address) = 0;
		virtual uint32 read32(uint64 address) = 0;
		virtual uint64 read64(uint64 address) = 0;
		virtual void write8 (uint64 address, uint8 value) = 0;
		virtual void write16(uint64 address, uint16 value) = 0;
		virtual void write32(uint64 address, uint32 value) = 0;
		virtual void write64(uint64 address, uint64 value) = 0;

		template<typename T> T read(uint64 address) { T::UNSUPPORTED_TYPE; }
		template<typename T> void write(uint64 address, T value) { T::UNSUPPORTED_TYPE; }

		virtual void getDirectAccessSpecialization(SpecializationResult& outResult, uint64 address, size_t size, bool writeAccess)  {}
	};


	class API_EXPORT Environment	// This is meant to be derived from, if needed, to provide custom information
	{
	public:
		inline explicit Environment(uint64 type) : mType(type) {}
		inline uint64 getType() const  { return mType; }

	private:
		uint64 mType = 0;	// Can be used to differentiate between various implementations
	};


	class API_EXPORT RuntimeDetailHandler
	{
	public:
		virtual void preExecuteExternalFunction(const NativeFunction& function, const ControlFlow& controlFlow)  {}
		virtual void postExecuteExternalFunction(const NativeFunction& function, const ControlFlow& controlFlow) {}
	};


	class API_EXPORT Runtime
	{
	friend class OpcodeExecUtils;
	friend class OpcodeExec;
	friend class OptimizedOpcodeExec;
	friend class RuntimeFunction;
	friend struct RuntimeOpcodeContext;

	public:
		struct ExecuteResult
		{
			enum class Result
			{
				OKAY,
				HALT
			};

			Result mResult = Result::OKAY;
			size_t mStepsExecuted = 0;
		};

		struct ExecuteConnector : public ExecuteResult
		{
			virtual bool handleCall(const Function* func, uint64 callTarget) = 0;
			virtual bool handleReturn() = 0;
			virtual bool handleExternalCall(uint64 address) = 0;
			virtual bool handleExternalJump(uint64 address) = 0;
		};

	public:
		inline static ControlFlow* getActiveControlFlow()	{ return mActiveControlFlow; }
		inline static Runtime* getActiveRuntime()			{ return (nullptr == mActiveControlFlow) ? nullptr : &mActiveControlFlow->getRuntime(); }

		inline static const Environment* getActiveEnvironment()					{ return mActiveEnvironment; }
		inline static void setActiveEnvironment(const Environment* environment)	{ mActiveEnvironment = environment; }

	public:
		Runtime();
		~Runtime();

		void reset();

		inline const Program& getProgram() const  { return *mProgram; }
		void setProgram(const Program& program);

		inline MemoryAccessHandler* getMemoryAccessHandler() const  { return mMemoryAccessHandler; }
		void setMemoryAccessHandler(MemoryAccessHandler* handler);

		inline RuntimeDetailHandler* getRuntimeDetailHandler() const  { return mRuntimeDetailHandler; }
		void setRuntimeDetailHandler(RuntimeDetailHandler* handler);

		void buildAllRuntimeFunctions();

		RuntimeFunction* getRuntimeFunction(const ScriptFunction& scriptFunction);
		RuntimeFunction* getRuntimeFunctionBySignature(uint64 signatureHash, size_t index = 0);

		bool hasStringWithKey(uint64 key) const;
		const FlyweightString* resolveStringByKey(uint64 key) const;
		uint64 addString(std::string_view str);

		int64 getGlobalVariableValue_int64(const Variable& variable);
		void setGlobalVariableValue_int64(const Variable& variable, int64 value);
		int64* accessGlobalVariableValue(const Variable& variable);

		inline const ControlFlow& getMainControlFlow() const  { return *mControlFlows[0]; }
		inline const ControlFlow& getSelectedControlFlow() const  { return *mSelectedControlFlow; }

		void callRuntimeFunction(const RuntimeFunction& runtimeFunction, size_t baseCallIndex = 0);
		void callFunction(const Function& function, size_t baseCallIndex = 0);
		bool callFunctionAtLabel(const Function& function, FlyweightString labelName);
		bool callFunctionByName(FlyweightString functionName, FlyweightString labelName = FlyweightString());
		bool returnFromFunction();

		void executeSteps(ExecuteConnector& result, size_t stepsLimit, size_t minimumCallStackSize);
		const Function* handleResultCall(const RuntimeOpcode& runtimeOpcode);

		inline void triggerStopSignal()  { mReceivedStopSignal = true; }

		bool serializeState(VectorBinarySerializer& serializer, std::string* outError = nullptr);

	private:
		inline static ControlFlow* mActiveControlFlow = nullptr;
		inline static const Environment* mActiveEnvironment = nullptr;

	private:
		const Program* mProgram = nullptr;
		MemoryAccessHandler* mMemoryAccessHandler = nullptr;
		RuntimeDetailHandler* mRuntimeDetailHandler = nullptr;

		std::vector<RuntimeFunction> mRuntimeFunctions;
		std::unordered_map<const ScriptFunction*, RuntimeFunction*> mRuntimeFunctionsMapped;
		std::unordered_map<uint64, std::vector<RuntimeFunction*>> mRuntimeFunctionsBySignature;   // Key is the hashed function name + signature hash
		rmx::OneTimeAllocPool mRuntimeOpcodesPool;

		std::vector<int64> mGlobalVariables;

		StringLookup mStrings;

		// TODO: Add functions to create / destroy control flows, otherwise we're stuck with just the main control flow
		std::vector<ControlFlow*> mControlFlows;		// Contains at least one control flow at all times = the main control flow at index 0
		ControlFlow* mSelectedControlFlow = nullptr;	// The currently selected control flow used by methods like "executeSteps" and "callFunction"; this must always be a valid pointer

		bool mReceivedStopSignal = false;
	};

}
