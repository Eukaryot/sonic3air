/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/StoredString.h"
#include "lemon/runtime/ControlFlow.h"
#include <unordered_map>


namespace lemon
{
	class Function;
	class Program;
	class StoredString;
	class UserDefinedFunction;
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


	class API_EXPORT RuntimeDetailHandler
	{
	public:
		virtual void preExecuteExternalFunction(const UserDefinedFunction& function, const ControlFlow& controlFlow)  {}
		virtual void postExecuteExternalFunction(const UserDefinedFunction& function, const ControlFlow& controlFlow) {}
	};


	class API_EXPORT Runtime
	{
	friend class OpcodeExecUtils;
	friend class OpcodeExec;
	friend class OptimizedOpcodeExec;
	friend class NativizedCode;
	friend struct RuntimeOpcodeContext;

	public:
		struct ExecuteResult
		{
			enum Result
			{
				CONTINUE,
				CALL,
				RETURN,
				EXTERNAL_CALL,
				EXTERNAL_JUMP,
				HALT
			};

			Result mResult = Result::HALT;
			size_t mStepsExecuted = 0;

			uint64 mCallTarget = 0;							// For result CALL, EXTERNAL_CALL and EXTERNAL_JUMP
			const RuntimeOpcode* mRuntimeOpcode = nullptr;	// For result CALL only
		};

	public:
		inline static ControlFlow* getActiveControlFlow() { return mActiveControlFlow; }
		inline static Runtime* getActiveRuntime() { return (nullptr == mActiveControlFlow) ? nullptr : &mActiveControlFlow->getRuntime(); }

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
		const StoredString* resolveStringByKey(uint64 key) const;
		uint64 addString(const std::string& str);
		uint64 addString(const std::string_view& str);
		uint64 addString(const char* str, size_t length);

		int64 getGlobalVariableValue(const Variable& variable);
		void setGlobalVariableValue(const Variable& variable, int64 value);
		int64* accessGlobalVariableValue(const Variable& variable);

		inline const ControlFlow& getMainControlFlow() const  { return *mMainControlFlow; }

		void callFunction(const RuntimeFunction& runtimeFunction, size_t baseCallIndex = 0);
		void callFunction(const Function& function, size_t baseCallIndex = 0);
		bool callFunctionAtLabel(const Function& function, const std::string& labelName);
		bool callFunctionByName(const std::string& functionName, const std::string& labelName = "");
		bool returnFromFunction();

		void executeSteps(Runtime::ExecuteResult& result, size_t stepsLimit = 1000);
		const Function* handleResultCall(const ExecuteResult& result);

		void getLastStepLocation(ControlFlow::Location& outLocation) const;

		bool serializeState(VectorBinarySerializer& serializer, std::string* outError = nullptr);

	private:
		inline static ControlFlow* mActiveControlFlow = nullptr;

	private:
		const Program* mProgram = nullptr;
		MemoryAccessHandler* mMemoryAccessHandler = nullptr;
		RuntimeDetailHandler* mRuntimeDetailHandler = nullptr;

		std::vector<RuntimeFunction> mRuntimeFunctions;
		std::unordered_map<const ScriptFunction*, RuntimeFunction*> mRuntimeFunctionsMapped;
		std::unordered_map<uint64, std::vector<RuntimeFunction*>> mRuntimeFunctionsBySignature;   // Key is the hashed function name + signature hash

		std::vector<int64> mGlobalVariables;

		StringLookup mStrings;

		ControlFlow* mMainControlFlow = nullptr;
	};

}
