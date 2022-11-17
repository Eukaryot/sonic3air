/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/program/DataType.h"


namespace lemon
{
	class MemoryAccessHandler;
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
		void getLastStepLocation(Location& outLocation) const;

		inline size_t getValueStackSize() const  { return mValueStackPtr - mValueStackStart; }

		template<typename T>
		FORCE_INLINE T popValueStack()
		{
			--mValueStackPtr;
			return AnyBaseValue(*mValueStackPtr).get<T>();
		}

		template<typename T>
		FORCE_INLINE void pushValueStack(T value)
		{
			*mValueStackPtr = AnyBaseValue(value).get<uint64>();
			++mValueStackPtr;
			RMX_ASSERT(mValueStackPtr < &mValueStackBuffer[0x78], "Value stack error: Too many elements");
		}

		template<typename T>
		FORCE_INLINE T readValueStack(int offset) const
		{
			return AnyBaseValue(mValueStackPtr[offset]).get<T>();
		}

		template<typename T>
		FORCE_INLINE void writeValueStack(int offset, T value) const
		{
			mValueStackPtr[offset] = AnyBaseValue(value).get<uint64>();
		}

		FORCE_INLINE void moveValueStack(int change)
		{
			mValueStackPtr += change;
		}

	private:
		Runtime& mRuntime;
		const Program* mProgram = nullptr;

		CArray<State> mCallStack;	// Not using std::vector for performance reasons in debug builds
		uint64 mValueStackBuffer[0x80] = { 0 };
		uint64* mValueStackStart = &mValueStackBuffer[4];	// Leave 4 elements so that removing too many elements from the stack doesn't break everything immediately
		uint64* mValueStackPtr   = &mValueStackBuffer[4];
		int64 mLocalVariablesBuffer[0x400] = { 0 };
		size_t mLocalVariablesSize = 0;
		State mLastStepState;

		// Only as optimization for OpcodeExec
		int64* mCurrentLocalVariables = nullptr;
		int64* mGlobalVariables = nullptr;
		MemoryAccessHandler* mMemoryAccessHandler = nullptr;
	};

}
