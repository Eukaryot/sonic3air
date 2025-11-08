/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/ControlFlow.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/RuntimeFunction.h"


namespace lemon
{

	ControlFlow::ControlFlow(Runtime& runtime) :
		mRuntime(runtime)
	{
	}

	void ControlFlow::reset()
	{
		mCallStack.clear();
		for (size_t k = 0; k < VALUE_STACK_MAX_SIZE; ++k)
		{
			mValueStackBuffer[k] = 0;
		}
		mValueStackStart = &mValueStackBuffer[VALUE_STACK_FIRST_INDEX];
		mValueStackPtr   = &mValueStackBuffer[VALUE_STACK_FIRST_INDEX];

		for (size_t k = 0; k < VAR_STACK_LIMIT; ++k)
		{
			mLocalVariablesBuffer[k] = 0;
		}
		mLocalVariablesSize = 0;
	}

	void ControlFlow::getCallStack(std::vector<Location>& outLocations) const
	{
		outLocations.resize(getCallStack().count);
		for (size_t i = 0; i < getCallStack().count; ++i)
		{
			const State& state = getCallStack()[i];
			if (nullptr != state.mRuntimeFunction)
			{
				outLocations[i].mFunction = state.mRuntimeFunction->mFunction;
				outLocations[i].mProgramCounter = state.mRuntimeFunction->translateFromRuntimeProgramCounter(state.mProgramCounter);
			}
			else
			{
				outLocations[i].mFunction = nullptr;
				outLocations[i].mProgramCounter = 0;
			}
		}
	}

	void ControlFlow::getRecentExecutionLocation(Location& outLocation) const
	{
		if (!mCallStack.empty())
		{
			const State& state = mCallStack.back();
			outLocation.mFunction = state.mRuntimeFunction->mFunction;
			outLocation.mProgramCounter = state.mRuntimeFunction->translateFromRuntimeProgramCounter(state.mProgramCounter);
		}
		else
		{
			outLocation.mFunction = nullptr;
			outLocation.mProgramCounter = 0;
		}
	}

	void ControlFlow::getCurrentExecutionLocation(Location& outLocation) const
	{
		if (!mCallStack.empty() && &mRuntime == Runtime::getActiveRuntime())
		{
			const State& state = mCallStack.back();
			outLocation.mFunction = state.mRuntimeFunction->mFunction;

			// Don't use the state's program counter, as it can be slightly out-dated
			//  -> Instead, get the actual program counter directly from the runtime
			//  -> However, this is only valid during actual code execution
			outLocation.mProgramCounter = state.mRuntimeFunction->translateFromRuntimeProgramCounter((const uint8*)mRuntime.getCurrentOpcode());
		}
		else
		{
			outLocation.mFunction = nullptr;
			outLocation.mProgramCounter = 0;
		}
	}

	const ScriptFunction* ControlFlow::getCurrentFunction() const
	{
		if (mCallStack.empty())
			return nullptr;

		const RuntimeFunction* runtimeFunction = mCallStack.back().mRuntimeFunction;
		return (nullptr != runtimeFunction) ? runtimeFunction->mFunction : nullptr;
	}

}
