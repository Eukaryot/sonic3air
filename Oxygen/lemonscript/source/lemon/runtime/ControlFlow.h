/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

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
		ControlFlow(Runtime& runtime);

		inline Runtime& getRuntime()  { return mRuntime; }
		inline const Program& getProgram()  { return *mProgram; }

		void reset();

		inline const ControlFlow::State& getState() const  { return mCallStack.back(); }
		inline const CArray<ControlFlow::State>& getCallStack() const  { return mCallStack; }

		void getCallStack(std::vector<ControlFlow::Location>& outLocations) const;
		void getLastStepLocation(Location& outLocation) const;

		inline size_t getValueStackSize() const  { return mValueStackPtr - mValueStackStart; }

		FORCE_INLINE uint64 popValueStack(const DataTypeDefinition* dataType)
		{
			--mValueStackPtr;
			return *mValueStackPtr;
		}

		FORCE_INLINE void pushValueStack(const DataTypeDefinition* dataType, uint64 value)
		{
			*mValueStackPtr = value;
			++mValueStackPtr;
			RMX_ASSERT(mValueStackPtr < &mValueStackBuffer[0x78], "Value stack error: Too many elements");
		}

		template<typename T>
		FORCE_INLINE T readValueStack(int offset) const
		{
			return (T)mValueStackPtr[offset];
		}

		template<typename T>
		FORCE_INLINE void writeValueStack(int offset, T value) const
		{
			mValueStackPtr[offset] = value;
		}

		FORCE_INLINE void moveValueStack(int change)
		{
			mValueStackPtr += change;
		}

	private:
		Runtime& mRuntime;

	// TODO: Move more functionality from runtime to this class, and change the following part to private
	public:
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
