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

		template<>
		FORCE_INLINE float readValueStack(int offset) const
		{
			const uint32 asInteger = (uint32)mValueStackPtr[offset];
			return *reinterpret_cast<const float*>(&asInteger);
		}

		template<>
		FORCE_INLINE void writeValueStack(int offset, float value) const
		{
			static_assert(sizeof(float) == 4);
			const uint32 asInteger = *reinterpret_cast<uint32*>(&value);
			mValueStackPtr[offset] = asInteger;
		}

		template<>
		FORCE_INLINE double readValueStack(int offset) const
		{
			const uint64 asInteger = mValueStackPtr[offset];
			return *reinterpret_cast<const double*>(&asInteger);
		}

		template<>
		FORCE_INLINE void writeValueStack(int offset, double value) const
		{
			static_assert(sizeof(double) == 8);
			const uint64 asInteger = *reinterpret_cast<uint64*>(&value);
			mValueStackPtr[offset] = asInteger;
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
