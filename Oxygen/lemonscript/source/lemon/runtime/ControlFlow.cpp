/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
		for (size_t k = 0; k < 0x80; ++k)
		{
			mValueStackBuffer[k] = 0;
		}
		mValueStackStart = &mValueStackBuffer[4];
		mValueStackPtr   = &mValueStackBuffer[4];
		for (size_t k = 0; k < 0x400; ++k)
		{
			mLocalVariablesBuffer[k] = 0;
		}
		mLocalVariablesSize = 0;
		mLastStepState = State();
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

	void ControlFlow::getLastStepLocation(Location& outLocation) const
	{
		if (nullptr == mLastStepState.mRuntimeFunction)
		{
			outLocation.mFunction = nullptr;
			outLocation.mProgramCounter = 0;
		}
		else
		{
			outLocation.mFunction = mLastStepState.mRuntimeFunction->mFunction;
			outLocation.mProgramCounter = mLastStepState.mRuntimeFunction->translateFromRuntimeProgramCounter(mLastStepState.mProgramCounter);
		}
	}

}
