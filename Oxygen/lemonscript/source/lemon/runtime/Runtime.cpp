/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/RuntimeFunction.h"
#include "lemon/runtime/RuntimeOpcodeContext.h"
#include "lemon/program/Program.h"
#include "lemon/program/StringRef.h"


namespace lemon
{
	namespace
	{
		int matchCallerProgramCounter(const Program& program, const ControlFlow::State& parentState, const ControlFlow::State& childLocation)
		{
			const std::vector<Opcode>& opcodes = parentState.mRuntimeFunction->mFunction->mOpcodes;
			const int oldPC = (int)parentState.mRuntimeFunction->translateFromRuntimeProgramCounter(parentState.mProgramCounter);
			int newPC = -1;

			// Pass 1: Search normal function calls
			{
				const uint64 nameAndSignatureHash = childLocation.mRuntimeFunction->mFunction->getNameAndSignatureHash();
				for (size_t k = 0; k < opcodes.size(); ++k)
				{
					if (opcodes[k].mType == Opcode::Type::CALL && (uint64)opcodes[k].mParameter == nameAndSignatureHash && opcodes[k].mDataType == BaseType::VOID)
					{
						const int programCounter = (int)(k + 1);
						if (newPC == -1 || std::abs(programCounter - oldPC) < std::abs(newPC - oldPC))
						{
							newPC = programCounter;
						}
					}
				}
			}

			// Pass 2: Search external calls, no filtering
			if (newPC < 0)
			{
				for (size_t k = 0; k < opcodes.size(); ++k)
				{
					if (opcodes[k].mType == Opcode::Type::EXTERNAL_CALL)
					{
						const int programCounter = (int)(k + 1);
						if (newPC == -1 || std::abs(programCounter - oldPC) < std::abs(newPC - oldPC))
						{
							newPC = programCounter;
						}
					}
				}
			}

			// Pass 3: Search normal function calls to user functions
			if (newPC < 0)
			{
				for (size_t k = 0; k < opcodes.size(); ++k)
				{
					if (opcodes[k].mType == Opcode::Type::CALL && opcodes[k].mDataType == BaseType::VOID)
					{
						const uint64 nameAndSignatureHash = (uint32)opcodes[k].mParameter;
						const Function* function = program.getFunctionBySignature(nameAndSignatureHash);
						if (nullptr != function && function->getType() == Function::Type::NATIVE)
						{
							const int programCounter = (int)(k + 1);
							if (newPC == -1 || std::abs(programCounter - oldPC) < std::abs(newPC - oldPC))
							{
								newPC = programCounter;
							}
						}
					}
				}
			}

			return (newPC == -1) ? oldPC : newPC;
		}
	}



	template<> int8   MemoryAccessHandler::read(uint64 address)  { return read8 (address); }
	template<> int16  MemoryAccessHandler::read(uint64 address)  { return read16(address); }
	template<> int32  MemoryAccessHandler::read(uint64 address)  { return read32(address); }
	template<> int64  MemoryAccessHandler::read(uint64 address)  { return read64(address); }
	template<> uint8  MemoryAccessHandler::read(uint64 address)  { return read8 (address); }
	template<> uint16 MemoryAccessHandler::read(uint64 address)  { return read16(address); }
	template<> uint32 MemoryAccessHandler::read(uint64 address)  { return read32(address); }
	template<> uint64 MemoryAccessHandler::read(uint64 address)  { return read64(address); }

	template<> void MemoryAccessHandler::write(uint64 address, int8 value)   { write8 (address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, int16 value)  { write16(address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, int32 value)  { write32(address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, int64 value)  { write64(address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, uint8 value)  { write8 (address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, uint16 value) { write16(address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, uint32 value) { write32(address, value); }
	template<> void MemoryAccessHandler::write(uint64 address, uint64 value) { write64(address, value); }



	Runtime::Runtime()
	{
		// Create default control flow
		mControlFlows.push_back(new ControlFlow(*this));
		mSelectedControlFlow = mControlFlows[0];

		mRuntimeOpcodesPool.setPageSize(0x40000);
	}

	Runtime::~Runtime()
	{
		for (ControlFlow* controlFlow : mControlFlows)
		{
			delete controlFlow;
		}
	}

	void Runtime::reset()
	{
		for (ControlFlow* controlFlow : mControlFlows)
		{
			controlFlow->reset();	// Existing control flows are only reset, not destroyed
		}
		mSelectedControlFlow = mControlFlows[0];	// Reset to main control flow

		mRuntimeFunctions.clear();
		mRuntimeFunctionsMapped.clear();
		mRuntimeFunctionsBySignature.clear();
		mRuntimeOpcodesPool.clear();
		mStrings.clear();

		if (nullptr != mProgram)
		{
			// Setup runtime functions (all empty at first)
			const std::vector<ScriptFunction*>& scriptFunctions = mProgram->getScriptFunctions();
			mRuntimeFunctions.resize(scriptFunctions.size());
			for (size_t i = 0; i < scriptFunctions.size(); ++i)
			{
				RuntimeFunction& runtimeFunc = mRuntimeFunctions[i];
				ScriptFunction& scriptFunc = *scriptFunctions[i];
				runtimeFunc.mFunction = &scriptFunc;

				// Register in lookups
				mRuntimeFunctionsMapped[&scriptFunc] = &runtimeFunc;
				std::vector<RuntimeFunction*>& funcs = mRuntimeFunctionsBySignature[scriptFunc.getNameAndSignatureHash()];
				funcs.insert(funcs.begin(), &runtimeFunc);		// Insert as first
			}

			// Load all string literals
			mProgram->collectAllStringLiterals(mStrings);
		}
	}

	void Runtime::setProgram(const Program& program)
	{
		mProgram = &program;
		for (ControlFlow* controlFlow : mControlFlows)
		{
			controlFlow->mProgram = &program;
		}

		reset();

		// Assign initial values to global variables
		{
			// TODO: "getGlobalVariables()" includes both user-defined variables and the script-defined globals variables,
			//       but "mGlobalVariables" actually only needs to store values for the script-defined ones
			mGlobalVariables.resize(program.getGlobalVariables().size());
			for (size_t index = 0; index < program.getGlobalVariables().size(); ++index)
			{
				Variable& variable = *program.getGlobalVariables()[index];
				mGlobalVariables[index] = (variable.getType() == Variable::Type::GLOBAL) ? static_cast<GlobalVariable&>(variable).mInitialValue : 0;
			}
		}
		for (ControlFlow* controlFlow : mControlFlows)
		{
			controlFlow->mGlobalVariables = &mGlobalVariables[0];
		}
	}

    void Runtime::setMemoryAccessHandler(MemoryAccessHandler* handler)
	{
		mMemoryAccessHandler = handler;
		for (ControlFlow* controlFlow : mControlFlows)
		{
			controlFlow->mMemoryAccessHandler = mMemoryAccessHandler;
		}
	}

	void Runtime::setRuntimeDetailHandler(RuntimeDetailHandler* handler)
	{
		mRuntimeDetailHandler = handler;
	}

	void Runtime::buildAllRuntimeFunctions()
	{
		for (Function* function : mProgram->getFunctions())
		{
			if (function->getType() == Function::Type::SCRIPT)
			{
				getRuntimeFunction(*static_cast<ScriptFunction*>(function));
			}
		}
	}

	RuntimeFunction* Runtime::getRuntimeFunction(const ScriptFunction& scriptFunction)
	{
		const auto it = mRuntimeFunctionsMapped.find(&scriptFunction);
		if (it == mRuntimeFunctionsMapped.end())
			return nullptr;

		RuntimeFunction* runtimeFunction = it->second;
		runtimeFunction->build(*this);
		return runtimeFunction;
	}

	RuntimeFunction* Runtime::getRuntimeFunctionBySignature(uint64 signatureHash, size_t index)
	{
		const auto it = mRuntimeFunctionsBySignature.find(signatureHash);
		if (it == mRuntimeFunctionsBySignature.end() || index >= it->second.size())
			return nullptr;

		RuntimeFunction* runtimeFunction = it->second[index];
		runtimeFunction->build(*this);
		return runtimeFunction;
	}

	bool Runtime::hasStringWithKey(uint64 key) const
	{
		return (nullptr != mStrings.getStringByHash(key));
	}

	const FlyweightString* Runtime::resolveStringByKey(uint64 key) const
	{
		return mStrings.getStringByHash(key);
	}

	uint64 Runtime::addString(std::string_view str)
	{
		const FlyweightString flyweightString(str);
		mStrings.addString(flyweightString);
		return flyweightString.getHash();
	}

	int64 Runtime::getGlobalVariableValue_int64(const Variable& variable)
	{
		const int64* valuePtr = accessGlobalVariableValue(variable);
		return (nullptr == valuePtr) ? 0 : *valuePtr;
	}

	void Runtime::setGlobalVariableValue_int64(const Variable& variable, int64 value)
	{
		int64* valuePtr = accessGlobalVariableValue(variable);
		if (nullptr != valuePtr)
		{
			*valuePtr = value;
		}
	}

	int64* Runtime::accessGlobalVariableValue(const Variable& variable)
	{
		RMX_CHECK((variable.getID() & 0xf0000000) == 0x10000000, "Variable " << variable.getName().getString() << " is not a global variable", return nullptr);
		const uint32 index = variable.getID() & 0x0fffffff;
		RMX_CHECK(index < mGlobalVariables.size(), "Variable index " << index << " is not valid", return nullptr);
		return &mGlobalVariables[index];
	}

	void Runtime::callFunction(const RuntimeFunction& runtimeFunction, size_t baseCallIndex)
	{
		// Push new state to call stack
		ControlFlow::State& state = *mSelectedControlFlow->mCallStack.add();
		state.mRuntimeFunction = &runtimeFunction;
		state.mBaseCallIndex = baseCallIndex;
		state.mProgramCounter = runtimeFunction.getFirstRuntimeOpcode();
		state.mLocalVariablesStart = mSelectedControlFlow->mLocalVariablesSize;
	}

	void Runtime::callFunction(const Function& function, size_t baseCallIndex)
	{
		switch (function.getType())
		{
			case Function::Type::SCRIPT:
			{
				const ScriptFunction& func = static_cast<const ScriptFunction&>(function);
				callFunction(*getRuntimeFunction(func));
				break;
			}

			case Function::Type::NATIVE:
			{
				const NativeFunction& func = static_cast<const NativeFunction&>(function);

				// Directly execute it
				mActiveControlFlow = mSelectedControlFlow;
				func.execute(NativeFunction::Context(*mSelectedControlFlow));
				mActiveControlFlow = nullptr;
				break;
			}
		}
	}

	bool Runtime::callFunctionAtLabel(const Function& function, FlyweightString labelName)
	{
		if (function.getType() != Function::Type::SCRIPT)
			return false;

		const ScriptFunction& func = static_cast<const ScriptFunction&>(function);
		size_t offset = 0xffffffff;
		if (!func.getLabel(labelName, offset))
			return false;

		RuntimeFunction* runtimeFunction = getRuntimeFunction(func);
		RMX_ASSERT(nullptr != runtimeFunction, "Got invalid runtime function");
		callFunction(*runtimeFunction);

		// Build up scope accordingly (all local variables will have a value of zero, though)
		int numLocalVars = (int)func.mLocalVariablesByID.size();
		//for (size_t i = 0; i < offset; ++i)
		//{
		//	if (func.mOpcodes[i].mType == Opcode::Type::MOVE_VAR_STACK)
		//	{
		//		numLocalVars += (int)func.mOpcodes[i].mParameter;
		//	}
		//}
		memset(&mSelectedControlFlow->mLocalVariablesBuffer[mSelectedControlFlow->mLocalVariablesSize], 0, numLocalVars * sizeof(int64));
		mSelectedControlFlow->mLocalVariablesSize += numLocalVars;
		mSelectedControlFlow->mCallStack.back().mProgramCounter = runtimeFunction->translateToRuntimeProgramCounter(offset);
		return true;
	}

	bool Runtime::callFunctionByName(FlyweightString functionName, FlyweightString labelName)
	{
		const uint64 nameAndSignatureHash = functionName.getHash() + Function::getVoidSignatureHash();
		const Function* function = mProgram->getFunctionBySignature(nameAndSignatureHash);
		if (nullptr != function)
		{
			if (labelName.isEmpty())
			{
				callFunction(*function);
				return true;
			}
			else
			{
				if (callFunctionAtLabel(*function, labelName))
					return true;
			}
		}
		return false;
	}

	bool Runtime::returnFromFunction()
	{
		if (mSelectedControlFlow->mCallStack.count == 0)
			return false;

		mSelectedControlFlow->mLocalVariablesSize = mSelectedControlFlow->mCallStack.back().mLocalVariablesStart;
		mSelectedControlFlow->mCallStack.pop_back();
		return true;
	}

	void Runtime::executeSteps(Runtime::ExecuteResult& result, size_t stepsLimit)
	{
		stepsLimit *= (sizeof(RuntimeOpcode) + 8);		// Rough estimate for average runtime opcode size

		result.mStepsExecuted = 0;
		if (mSelectedControlFlow->mCallStack.count == 0)
		{
			result.mResult = ExecuteResult::HALT;
			return;
		}

		ControlFlow::State& state = mSelectedControlFlow->mCallStack.back();
		const uint8*& programCounter = state.mProgramCounter;
		const RuntimeOpcodeBuffer& runtimeOpcodeBuffer = state.mRuntimeFunction->mRuntimeOpcodeBuffer;

		// Reached the end already?
		//  -> Should not happen actually, as all functions end with a return opcode
		if (programCounter > runtimeOpcodeBuffer.getEnd())
		{
			returnFromFunction();
			result.mResult = ExecuteResult::RETURN;
			return;
		}

		mActiveControlFlow = mSelectedControlFlow;
		RMX_CHECK(mSelectedControlFlow->mValueStackPtr >= mSelectedControlFlow->mValueStackStart, "Value stack error: Removed elements from empty stack", mSelectedControlFlow->mValueStackPtr = mSelectedControlFlow->mValueStackStart);
		RMX_CHECK(mSelectedControlFlow->mValueStackPtr < &mSelectedControlFlow->mValueStackBuffer[0x78], "Value stack error: Too many elements", mSelectedControlFlow->mValueStackPtr = &mSelectedControlFlow->mValueStackBuffer[0x77]);

		result.mResult = ExecuteResult::CONTINUE;
		mSelectedControlFlow->mLastStepState.mRuntimeFunction = state.mRuntimeFunction;
		mSelectedControlFlow->mCurrentLocalVariables = &mSelectedControlFlow->mLocalVariablesBuffer[state.mLocalVariablesStart];

		RuntimeOpcodeContext context;
		context.mControlFlow = mSelectedControlFlow;
		context.mOpcode = (const RuntimeOpcode*)programCounter;

		const uint8* programCounterInitial = programCounter;
		while (true)
		{
			// Update location for the next step
			mSelectedControlFlow->mLastStepState.mProgramCounter = (uint8*)context.mOpcode;

			// Optimization: Do multiple opcodes in a row without overheads if possible
			if (context.mOpcode->mSuccessiveHandledOpcodes >= 4)
			{
				(*context.mOpcode->mExecFunc)(context);
				context.mOpcode = context.mOpcode->mNext;

				(*context.mOpcode->mExecFunc)(context);
				context.mOpcode = context.mOpcode->mNext;

				(*context.mOpcode->mExecFunc)(context);
				context.mOpcode = context.mOpcode->mNext;

				(*context.mOpcode->mExecFunc)(context);
				context.mOpcode = context.mOpcode->mNext;
			}
			else if (context.mOpcode->mSuccessiveHandledOpcodes > 0)
			{
				(*context.mOpcode->mExecFunc)(context);
				context.mOpcode = context.mOpcode->mNext;
			}
			else
			{
				programCounter = (uint8*)context.mOpcode + context.mOpcode->mSize;

				switch (context.mOpcode->mOpcodeType)
				{
					case Opcode::Type::JUMP_CONDITIONAL:
					{
						--mSelectedControlFlow->mValueStackPtr;
						if (*mSelectedControlFlow->mValueStackPtr != 0)
							break;

						// Fallthrough to unconditional jump
					}

					case Opcode::Type::JUMP:
					{
						result.mStepsExecuted += (size_t)(programCounter - programCounterInitial);
						programCounter = &runtimeOpcodeBuffer[(size_t)context.mOpcode->getParameter<uint32>()];

						// Check if steps limit is reached (this usually means the limit was exceeded already, but that's okay)
						//  -> This is needed to prevent endless loops
						if (result.mStepsExecuted >= stepsLimit)
						{
							mActiveControlFlow = nullptr;
							return;
						}

						programCounterInitial = programCounter;
						break;
					}

					case Opcode::Type::CALL:
					{
						// Inline execution of native function, if possible here
						//  -> Note that this optimization is actually hardly ever used, thanks to the CALL runtime opcode replacement in OptimizedOpcodeProvider
						if (context.mOpcode->mFlags & RuntimeOpcode::FLAG_CALL_INLINE_RESOLVED)
						{
							const NativeFunction& func = *context.mOpcode->getParameter<const NativeFunction*>();
							func.execute(NativeFunction::Context(*mSelectedControlFlow));
							break;
						}
						else
						{
							result.mResult = ExecuteResult::CALL;
							result.mStepsExecuted += (size_t)(programCounter - programCounterInitial);
							result.mCallTarget = context.mOpcode->getParameter<uint64>();
							result.mRuntimeOpcode = context.mOpcode;
							mActiveControlFlow = nullptr;
							return;
						}
					}

					case Opcode::Type::RETURN:
					{
						returnFromFunction();
						result.mResult = ExecuteResult::RETURN;
						result.mStepsExecuted += (size_t)(programCounter - programCounterInitial);
						mActiveControlFlow = nullptr;
						return;
					}

					case Opcode::Type::EXTERNAL_CALL:
					{
						--mSelectedControlFlow->mValueStackPtr;
						result.mResult = ExecuteResult::EXTERNAL_CALL;
						result.mStepsExecuted += (size_t)(programCounter - programCounterInitial);
						result.mCallTarget = *mSelectedControlFlow->mValueStackPtr;
						mActiveControlFlow = nullptr;
						return;
					}

					case Opcode::Type::EXTERNAL_JUMP:
					{
						--mSelectedControlFlow->mValueStackPtr;
						returnFromFunction();
						result.mResult = ExecuteResult::EXTERNAL_JUMP;
						result.mStepsExecuted += (size_t)(programCounter - programCounterInitial);
						result.mCallTarget = *mSelectedControlFlow->mValueStackPtr;
						mActiveControlFlow = nullptr;
						return;
					}

					default:
						throw std::runtime_error("Unhandled opcode");
				}

				context.mOpcode = (const RuntimeOpcode*)programCounter;
			}
		}
	}

	const Function* Runtime::handleResultCall(const ExecuteResult& result)
	{
		RMX_ASSERT(result.mResult == ExecuteResult::CALL, "Do not use 'lemon::Runtime::handleResultCall' when execution result is not CALL");
		RMX_ASSERT(nullptr != result.mRuntimeOpcode, "No runtime opcode given in 'lemon::Runtime::handleResultCall'");

		// Consider base function call, if additional data says so
		const size_t baseCallIndex = (result.mRuntimeOpcode->mFlags & RuntimeOpcode::FLAG_CALL_IS_BASE_CALL) ? (mSelectedControlFlow->getState().mBaseCallIndex + 1) : 0;

		const constexpr uint8 FLAG_RESOLVED_RUNTIME_FUNC = (RuntimeOpcode::FLAG_CALL_TARGET_RESOLVED | RuntimeOpcode::FLAG_CALL_TARGET_RUNTIME_FUNC);
		if ((result.mRuntimeOpcode->mFlags & FLAG_RESOLVED_RUNTIME_FUNC) == FLAG_RESOLVED_RUNTIME_FUNC)
		{
			// Take the runtime function shortcut (this is the most common one)
			const RuntimeFunction* runtimeFunction = result.mRuntimeOpcode->getParameter<const RuntimeFunction*>();
			callFunction(*runtimeFunction, baseCallIndex);
			return runtimeFunction->mFunction;
		}
		else if (result.mRuntimeOpcode->mFlags & RuntimeOpcode::FLAG_CALL_TARGET_RESOLVED)
		{
			// Take the shortcut to a normal function
			const Function* function = result.mRuntimeOpcode->getParameter<const Function*>();
			callFunction(*function, baseCallIndex);
			return function;
		}
		else
		{
			RuntimeOpcode& runtimeOpcode = const_cast<RuntimeOpcode&>(*result.mRuntimeOpcode);

			// If it's a script function call, there should be an associated runtime function that can be called directly
			RuntimeFunction* runtimeFunction = getRuntimeFunctionBySignature(result.mCallTarget, baseCallIndex);
			if (nullptr != runtimeFunction)
			{
				// Create a shortcut for next time
				runtimeOpcode.setParameter(runtimeFunction);
				runtimeOpcode.mFlags |= (RuntimeOpcode::FLAG_CALL_TARGET_RESOLVED | RuntimeOpcode::FLAG_CALL_TARGET_RUNTIME_FUNC);

				// Call the function now
				callFunction(*runtimeFunction, baseCallIndex);
				return runtimeFunction->mFunction;
			}

			// Another try, in case it's not a script function
			const Function* function = mProgram->getFunctionBySignature(result.mCallTarget, baseCallIndex);
			if (nullptr != function)
			{
				// Create a shortcut for next time
				runtimeOpcode.setParameter(function);
				runtimeOpcode.mFlags |= RuntimeOpcode::FLAG_CALL_TARGET_RESOLVED;

				if (function->getType() == Function::Type::NATIVE)
				{
					if (static_cast<const NativeFunction*>(function)->mFlags & NativeFunction::FLAG_ALLOW_INLINE_EXECUTION)
					{
						runtimeOpcode.mFlags |= RuntimeOpcode::FLAG_CALL_INLINE_RESOLVED;
					}
				}

				// Call the function now
				callFunction(*function, baseCallIndex);
				return function;
			}

			// Failed
			return nullptr;
		}
	}

	void Runtime::getLastStepLocation(ControlFlow::Location& outLocation) const
	{
		mSelectedControlFlow->getLastStepLocation(outLocation);
	}

	bool Runtime::serializeState(VectorBinarySerializer& serializer, std::string* outError)
	{
		// Format version history:
		//  - 0x00 = First version, no signature yet
		//  - 0x01 = Added signature and version number + serialize global variable names

		if (nullptr == mProgram)
		{
			if (nullptr != outError)
				*outError = "No program loaded";
			return false;
		}

		if (serializer.isReading())
		{
			// Reset only the control flows, no full reset is needed here
			//  -> In fact it would even cause issues down the line, as this is not meant to e.g. invalidate cached runtime functions
			for (ControlFlow* controlFlow : mControlFlows)
			{
				controlFlow->reset();
			}
		}

		// Signature and version number
		const uint32 SIGNATURE = *(uint32*)"LMN|";
		uint16 version = 0x01;
		if (serializer.isReading())
		{
			const uint32 signature = *(const uint32*)serializer.peek();
			if (signature == SIGNATURE)
			{
				serializer.skip(4);
				version = serializer.read<uint16>();
			}
			else
			{
				version = 0;	// First serialization format version had no signature at all
			}
		}
		else
		{
			serializer.write(SIGNATURE);
			serializer.write(version);
		}

		// Serialize call stack
		// TODO: Support multiple control flows?
		{
			ControlFlow& controlFlow = *mControlFlows[0];
			serializer.serializeAs<uint32>(controlFlow.mCallStack.count);
			if (serializer.isReading())
			{
				controlFlow.mCallStack.resize(controlFlow.mCallStack.count);
				for (uint16 i = 0; i < controlFlow.mCallStack.count; ++i)
				{
					const std::string_view functionName = serializer.readStringView();
					const uint64 nameHash = rmx::getMurmur2_64(functionName);
					uint32 signatureHash = serializer.read<uint32>();
					const Function* function = mProgram->getFunctionBySignature(nameHash + signatureHash, 0);	// Note that this does not support function overloading, but maybe that's no problem at all
				#if 1
					// This is only added (in early 2022) for compatibility with older save states and can be removed again somewhere down the line
					if (nullptr == function && signatureHash == 0xd202ef8d)		// Signature hash for void functions has changed
					{
						signatureHash = 0x76e88724;
						function = mProgram->getFunctionBySignature(nameHash + signatureHash, 0);	// Note that this does not support function overloading, but maybe that's no problem at all
					}
				#endif
					if (nullptr == function || function->getType() != Function::Type::SCRIPT)
					{
						if (nullptr != outError)
							*outError = "Could not match function signature for script function of name '" + std::string(functionName) + "'";
						controlFlow.mCallStack.clear();
						return false;
					}
					RuntimeFunction* runtimeFunction = getRuntimeFunction(static_cast<const ScriptFunction&>(*function));
					controlFlow.mCallStack[i].mRuntimeFunction = runtimeFunction;
					controlFlow.mCallStack[i].mProgramCounter = runtimeFunction->translateToRuntimeProgramCounter(serializer.read<uint32>());

					controlFlow.mCallStack[i].mLocalVariablesStart = controlFlow.mLocalVariablesSize;
					const size_t numLocalVars = serializer.read<uint32>();
					for (size_t k = controlFlow.mLocalVariablesSize; k < controlFlow.mLocalVariablesSize + numLocalVars; ++k)
					{
						controlFlow.mLocalVariablesBuffer[k] = serializer.read<int64>();
					}
					controlFlow.mLocalVariablesSize += numLocalVars;
				}

				// Make corrections to the program counters for the case that the call points changed
				for (uint16 i = 0; i < controlFlow.mCallStack.count - 1; ++i)
				{
					const size_t opcodeIndex = (size_t)matchCallerProgramCounter(*mProgram, controlFlow.mCallStack[i], controlFlow.mCallStack[i + 1]);
					controlFlow.mCallStack[i].mProgramCounter = controlFlow.mCallStack[i].mRuntimeFunction->translateToRuntimeProgramCounter(opcodeIndex);
				}
			}
			else
			{
				for (uint16 i = 0; i < controlFlow.mCallStack.count; ++i)
				{
					serializer.write(controlFlow.mCallStack[i].mRuntimeFunction->mFunction->getName().getString());
					serializer.write(controlFlow.mCallStack[i].mRuntimeFunction->mFunction->getSignatureHash());
					serializer.writeAs<uint32>(controlFlow.mCallStack[i].mRuntimeFunction->translateFromRuntimeProgramCounter(controlFlow.mCallStack[i].mProgramCounter));

					const size_t localVarsStart = controlFlow.mCallStack[i].mLocalVariablesStart;
					const size_t localVarsEnd = ((size_t)(i+1) < controlFlow.mCallStack.count) ? controlFlow.mCallStack[i+1].mLocalVariablesStart : controlFlow.mLocalVariablesSize;
					serializer.writeAs<uint32>(localVarsEnd - localVarsStart);
					for (size_t k = localVarsStart; k < localVarsEnd; ++k)
					{
						serializer.writeAs<int64>(controlFlow.mLocalVariablesBuffer[k]);
					}
				}
			}

			// Serialize value stack
			if (serializer.isReading())
			{
				const uint32 size = serializer.read<uint32>();
				controlFlow.mValueStackPtr = &controlFlow.mValueStackStart[size];
				for (uint32 i = 0; i < size; ++i)
				{
					controlFlow.mValueStackStart[i] = serializer.read<uint64>();
				}
			}
			else
			{
				const uint32 size = (uint32)controlFlow.getValueStackSize();
				serializer.write(size);
				for (size_t i = 0; i < size; ++i)
				{
					serializer.write(controlFlow.mValueStackStart[i]);
				}
			}
		}

		// Serialize global variables
		if (version >= 0x01)
		{
			if (serializer.isReading())
			{
				const size_t numGlobalsSerialized = (size_t)serializer.read<uint32>();
				std::string name;
				for (size_t i = 0; i < numGlobalsSerialized; ++i)
				{
					serializer.serialize(name);
					const int64 value = serializer.read<uint64>();
					const uint64 nameHash = rmx::getMurmur2_64(name);
					Variable* variable = mProgram->getGlobalVariableByName(nameHash);
					if (nullptr != variable && variable->getType() == Variable::Type::GLOBAL)
					{
						const size_t index = variable->getID() & 0x0fffffff;
						RMX_CHECK(index < mGlobalVariables.size(), "Invalid global variable index", continue);
						mGlobalVariables[index] = value;
					}
				}
			}
			else
			{
				RMX_ASSERT(mGlobalVariables.size() == mProgram->getGlobalVariables().size(), "Runtime globals and program globals are supposed to match");
				serializer.writeAs<uint32>(mGlobalVariables.size());
				for (size_t i = 0; i < mGlobalVariables.size(); ++i)
				{
					serializer.write(mProgram->getGlobalVariables()[i]->getName().getString());
					serializer.write<uint64>(mGlobalVariables[i]);
				}
			}
		}
		else
		{
			// Legacy code for version 0x00
			if (serializer.isReading())
			{
				const size_t numGlobalsSerialized = (size_t)serializer.read<uint32>();
				const size_t numGlobalsInProgram = mGlobalVariables.size();
				const size_t numGlobalsShared = std::min(numGlobalsSerialized, numGlobalsInProgram);
				for (size_t i = 0; i < numGlobalsShared; ++i)
				{
					mGlobalVariables[i] = serializer.read<uint64>();
				}
				if (numGlobalsSerialized > numGlobalsInProgram)
				{
					serializer.skip((numGlobalsShared - numGlobalsInProgram) * 8);
				}
			}
			else
			{
				serializer.writeAs<uint32>(mGlobalVariables.size());
				for (int64 variable : mGlobalVariables)
				{
					serializer.write<uint64>(variable);
				}
			}
		}

		// Done
		return true;
	}

}
