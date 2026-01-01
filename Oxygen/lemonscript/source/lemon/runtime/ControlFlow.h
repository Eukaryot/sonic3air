/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/program/DataType.h"
#include "lemon/utility/AnyBaseValue.h"


namespace lemon
{
	class MemoryAccessHandler;
	class Module;
	class Program;
	class Runtime;
	class RuntimeFunction;
	class ScriptFunction;


	class API_EXPORT ControlFlow
	{
	friend class Runtime;
	friend class OpcodeExec;
	friend class OptimizedOpcodeExec;
	friend struct RuntimeOpcodeContext;

	public:
		struct State
		{
			const RuntimeFunction* mRuntimeFunction = nullptr;
			size_t mBaseCallIndex = 0;
			const uint8* mProgramCounter = nullptr;
			size_t mLocalVariablesStart = 0;
		};

		struct Location
		{
			const ScriptFunction* mFunction = nullptr;
			size_t mProgramCounter = 0;
		};

	public:
		explicit ControlFlow(Runtime& runtime);

		inline Runtime& getRuntime()  { return mRuntime; }
		inline const Program& getProgram()  { return *mProgram; }
		inline MemoryAccessHandler& getMemoryAccessHandler() { return *mMemoryAccessHandler; }

		void reset();

		inline const ControlFlow::State& getState() const  { return mCallStack.back(); }
		inline const CArray<ControlFlow::State>& getCallStack() const  { return mCallStack; }

		void getCallStack(std::vector<ControlFlow::Location>& outLocations) const;
		void getRecentExecutionLocation(Location& outLocation) const;
		void getCurrentExecutionLocation(Location& outLocation) const;
		const ScriptFunction* getCurrentFunction() const;
		const Module* getCurrentModule() const;

		inline size_t getValueStackSize() const  { return mValueStackPtr - mValueStackStart; }

		template<typename T>
		FORCE_INLINE T popValueStack()
		{
			--mValueStackPtr;
			return BaseTypeConversion::convert<uint64, T>(*mValueStackPtr);
		}

		template<typename T>
		FORCE_INLINE void pushValueStack(T value)
		{
			*mValueStackPtr = BaseTypeConversion::convert<T, uint64>(value);
			++mValueStackPtr;
			RMX_ASSERT(mValueStackPtr < &mValueStackBuffer[VALUE_STACK_LAST_INDEX], "Value stack error: Too many elements");
		}

		template<typename T>
		FORCE_INLINE T readValueStack(int offset) const
		{
			return BaseTypeConversion::convert<uint64, T>(mValueStackPtr[offset]);
		}

		template<typename T>
		FORCE_INLINE void writeValueStack(int offset, T value) const
		{
			mValueStackPtr[offset] = BaseTypeConversion::convert<T, uint64>(value);
		}

		FORCE_INLINE void moveValueStack(int change)
		{
			mValueStackPtr += change;
		}

	private:
		inline static const size_t VALUE_STACK_MAX_SIZE    = 0x100;
		inline static const size_t VALUE_STACK_FIRST_INDEX = 4;			// Leave 4 elements so that removing too many elements from the stack doesn't break everything immediately
		inline static const size_t VALUE_STACK_LAST_INDEX  = VALUE_STACK_MAX_SIZE - 8;
		inline static const size_t VAR_STACK_LIMIT         = 0x1000;

		Runtime& mRuntime;
		const Program* mProgram = nullptr;

		CArray<State> mCallStack;	// Not using std::vector for performance reasons in debug builds
		uint64 mValueStackBuffer[VALUE_STACK_MAX_SIZE] = { 0 };
		uint64* mValueStackStart = &mValueStackBuffer[VALUE_STACK_FIRST_INDEX];
		uint64* mValueStackPtr   = &mValueStackBuffer[VALUE_STACK_FIRST_INDEX];
		int64 mLocalVariablesBuffer[VAR_STACK_LIMIT] = { 0 };
		size_t mLocalVariablesSize = 0;							// Current used size of the local variables buffer

		// Only as optimization for OpcodeExec
		int64* mCurrentLocalVariables = nullptr;
		MemoryAccessHandler* mMemoryAccessHandler = nullptr;
	};

}
