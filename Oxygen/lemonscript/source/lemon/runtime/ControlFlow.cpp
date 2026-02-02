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
#include "lemon/program/Program.h"


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

	const Module* ControlFlow::getCurrentModule() const
	{
		const ScriptFunction* function = getCurrentFunction();
		return (nullptr != function) ? &function->getModule() : nullptr;
	}

	int64 ControlFlow::readVariableGeneric(uint32 variableId)
	{
		const Variable::Type type = (Variable::Type)(variableId >> 28);
		switch (type)
		{
			default:
			case Variable::Type::LOCAL:
			{
				const LocalVariable& variable = getCurrentFunction()->getLocalVariableByID(variableId);
				return readLocalVariable<int64>(variable.getLocalMemoryOffset());
			}

			case Variable::Type::GLOBAL:
			{
				const GlobalVariable& variable = mProgram->getGlobalVariableByID(variableId).as<GlobalVariable>();
				const int64* valuePtr = getRuntime().accessGlobalVariableValue(variable);
				return (nullptr != valuePtr) ? *valuePtr : 0;
			}

			case Variable::Type::USER:
			{
				const UserDefinedVariable& variable = mProgram->getGlobalVariableByID(variableId).as<UserDefinedVariable>();
				variable.mGetter(*this);		// This is supposed to write a value to the value stack
				return popValueStack<int64>();
			}

			case Variable::Type::EXTERNAL:
			{
				const ExternalVariable& variable = mProgram->getGlobalVariableByID(variableId).as<ExternalVariable>();
				const int64* valuePtr = variable.mAccessor();
				return (nullptr != valuePtr) ? *valuePtr : 0;
			}
		}
	}

	void ControlFlow::writeVariableGeneric(uint32 variableId, int64 value)
	{
		const Variable::Type type = (Variable::Type)(variableId >> 28);
		switch (type)
		{
			default:
			case Variable::Type::LOCAL:
			{
				const LocalVariable& variable = getCurrentFunction()->getLocalVariableByID(variableId);
				writeLocalVariable(variable.getLocalMemoryOffset(), value);
				break;
			}

			case Variable::Type::GLOBAL:
			{
				const GlobalVariable& variable = mProgram->getGlobalVariableByID(variableId).as<GlobalVariable>();
				int64* valuePtr = getRuntime().accessGlobalVariableValue(variable);
				if (nullptr != valuePtr)
					*valuePtr = value;
				break;
			}

			case Variable::Type::USER:
			{
				const UserDefinedVariable& variable = mProgram->getGlobalVariableByID(variableId).as<UserDefinedVariable>();
				pushValueStack(value);
				variable.mSetter(*this);		// This is supposed to read a value from the value stack
				break;
			}

			case Variable::Type::EXTERNAL:
			{
				const ExternalVariable& variable = mProgram->getGlobalVariableByID(variableId).as<ExternalVariable>();
				int64* valuePtr = variable.mAccessor();
				if (nullptr != valuePtr)
					*valuePtr = value;
				break;
			}
		}
	}

	uint8* ControlFlow::accessVariableGeneric(uint32 variableId)
	{
		const Variable::Type type = (Variable::Type)(variableId >> 28);
		switch (type)
		{
			case Variable::Type::LOCAL:
			{
				// This requires local variables with different sizes, currently they're all just one uint64 each
				//return reinterpret_cast<uint8*>(mCurrentLocalVariables[variableId]);

				RMX_ASSERT(false, "Unhandled variable type for 'accessVariableGeneric'");
				return nullptr;
			}

			case Variable::Type::GLOBAL:
			{
				const GlobalVariable& variable = mProgram->getGlobalVariableByID(variableId).as<GlobalVariable>();
				return reinterpret_cast<uint8*>(getRuntime().accessGlobalVariableValue(variable));
			}

			default:
			{
				RMX_ASSERT(false, "Unhandled variable type for 'accessVariableGeneric'");
				return nullptr;
			}
		}
	}

}
