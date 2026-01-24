/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
						if (nullptr != function && function->isA<NativeFunction>())
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
		mEncounteredBuildError = false;

		clearAllControlFlows();

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

	void Runtime::clearAllControlFlows()
	{
		for (ControlFlow* controlFlow : mControlFlows)
		{
			controlFlow->reset();	// Existing control flows are only reset, not destroyed
		}
		mSelectedControlFlow = mControlFlows[0];	// Reset to main control flow
	}

	void Runtime::setProgram(const Program& program)
	{
		mProgram = &program;
		for (ControlFlow* controlFlow : mControlFlows)
		{
			controlFlow->mProgram = &program;
		}

		reset();
		setupGlobalVariables();
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

	void Runtime::resetRuntimeState()
	{
		// Reset global variables back to defaults
		setupGlobalVariables();
	}

	void Runtime::buildAllRuntimeFunctions()
	{
		for (Function* function : mProgram->getFunctions())
		{
			if (function->isA<ScriptFunction>())
			{
				getRuntimeFunction(function->as<ScriptFunction>());
			}
		}
	}

	RuntimeFunction* Runtime::getRuntimeFunction(const ScriptFunction& scriptFunction)
	{
		const auto it = mRuntimeFunctionsMapped.find(&scriptFunction);
		if (it == mRuntimeFunctionsMapped.end())
			return nullptr;

		RuntimeFunction* runtimeFunction = it->second;
		if (!runtimeFunction->build(*this))
		{
			mEncounteredBuildError = true;
			return nullptr;
		}

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
		const uint64 hash = rmx::getMurmur2_64(str);
		if (nullptr == mStrings.getStringByHash(hash))
		{
			mStrings.addString(str, hash);
		}
		return hash;
	}

	AnyBaseValue Runtime::getGlobalVariableValue(const GlobalVariable& variable)
	{
		AnyBaseValue result;
		const int64* valuePtr = accessGlobalVariableValue(variable);
		if (nullptr != valuePtr)
			result.set<int64>(*valuePtr);
		return result;
	}

	void Runtime::setGlobalVariableValue(const GlobalVariable& variable, AnyBaseValue value)
	{
		int64* valuePtr = accessGlobalVariableValue(variable);
		if (nullptr != valuePtr)
		{
			*valuePtr = value.get<int64>();
		}
	}

	int64* Runtime::accessGlobalVariableValue(const GlobalVariable& variable)
	{
		RMX_CHECK(variable.getType() == Variable::Type::GLOBAL, "Variable " << variable.getName() << " is not a global variable", return nullptr);
		const size_t offset = variable.getStaticMemoryOffset();
		return (int64*)&mStaticMemory[offset];
	}

	void Runtime::callRuntimeFunction(const RuntimeFunction& runtimeFunction, size_t baseCallIndex)
	{
		if (mSelectedControlFlow->mLocalVariablesSize + runtimeFunction.mFunction->mLocalVariablesByID.size() > ControlFlow::VAR_STACK_LIMIT)
		{
			throw std::runtime_error("Reached var stack limit, possibly due to recursive function calls");
		}

		// Push new state to call stack
		ControlFlow::State& state = *mSelectedControlFlow->mCallStack.add();
		state.mRuntimeFunction = &runtimeFunction;
		state.mBaseCallIndex = baseCallIndex;
		state.mProgramCounter = runtimeFunction.getFirstRuntimeOpcode();
		state.mLocalVariablesStart = mSelectedControlFlow->mLocalVariablesSize;
		RMX_ASSERT(nullptr != state.mProgramCounter, "Invalid program counter in function " << runtimeFunction.mFunction->getName());
	}

	void Runtime::callFunction(const Function& function, size_t baseCallIndex)
	{
		switch (function.getType())
		{
			case Function::Type::SCRIPT:
			{
				const ScriptFunction& func = function.as<ScriptFunction>();
				callRuntimeFunction(*getRuntimeFunction(func));
				break;
			}

			case Function::Type::NATIVE:
			{
				// Directly execute it
				const NativeFunction& func = function.as<NativeFunction>();
				func.execute(NativeFunction::Context(*mSelectedControlFlow));
				break;
			}
		}
	}

	bool Runtime::callFunctionAtLabel(const Function& function, FlyweightString labelName)
	{
		if (!function.isA<ScriptFunction>())
			return false;

		const ScriptFunction& func = function.as<ScriptFunction>();
		const ScriptFunction::Label* label = func.findLabelByName(labelName);
		if (nullptr == label)
			return false;

		return callFunctionAtLabel(func, *label);
	}

	bool Runtime::callFunctionAtLabel(const ScriptFunction& function, const ScriptFunction::Label& label)
	{
		RuntimeFunction* runtimeFunction = getRuntimeFunction(function);
		RMX_ASSERT(nullptr != runtimeFunction, "Got invalid runtime function");
		callRuntimeFunction(*runtimeFunction);

		// Build up scope accordingly (all local variables will have a value of zero, though)
		int numLocalVars = (int)function.mLocalVariablesByID.size();
		//for (size_t i = 0; i < label.mOffset; ++i)
		//{
		//	if (func.mOpcodes[i].mType == Opcode::Type::MOVE_VAR_STACK)
		//	{
		//		numLocalVars += (int)func.mOpcodes[i].mParameter;
		//	}
		//}
		memset(&mSelectedControlFlow->mLocalVariablesBuffer[mSelectedControlFlow->mLocalVariablesSize], 0, numLocalVars * sizeof(int64));
		mSelectedControlFlow->mLocalVariablesSize += numLocalVars;
		RMX_CHECK(mSelectedControlFlow->mLocalVariablesSize <= ControlFlow::VAR_STACK_LIMIT, "Reached var stack limit, probably due to recursive function calls", RMX_REACT_THROW);
		mSelectedControlFlow->mCallStack.back().mProgramCounter = runtimeFunction->translateToRuntimeProgramCounter(label.mOffset);
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

	bool Runtime::callFunctionWithParameters(FlyweightString functionName, const FunctionCallParameters& params)
	{
		const DataTypeDefinition& returnType = (nullptr != params.mReturnType) ? *params.mReturnType : PredefinedDataTypes::VOID;

		// Build the function signature hash
		uint32 signatureHash = Function::getVoidSignatureHash();
		if (!returnType.isA<VoidDataType>() || !params.mParams.empty())
		{
			Function::SignatureBuilder builder;
			builder.clear(returnType);
			for (const FunctionCallParameters::Parameter& param : params.mParams)
				builder.addParameterType(*param.mDataType);
			signatureHash = builder.getSignatureHash();
		}

		const uint64 nameAndSignatureHash = functionName.getHash() + signatureHash;
		const Function* function = mProgram->getFunctionBySignature(nameAndSignatureHash);
		if (nullptr != function)
		{
			// Push parameters accordingly
			for (const FunctionCallParameters::Parameter& param : params.mParams)
			{
				mSelectedControlFlow->pushValueStack(param.mStorage);
			}

			// Call
			callFunction(*function);
			return true;
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

	bool Runtime::canExecuteSteps() const
	{
		return !mEncounteredBuildError;
	}

	void Runtime::executeSteps(ExecuteConnector& result, size_t stepsLimit, size_t minimumCallStackSize)
	{
		if (mEncounteredBuildError)
		{
			result.mResult = ExecuteResult::Result::HALT;
			return;
		}

		result.mStepsExecuted = 0;
		result.mResult = ExecuteResult::Result::OKAY;

		if (mSelectedControlFlow->mCallStack.count <= minimumCallStackSize)
		{
			result.mResult = ExecuteResult::Result::HALT;
			return;
		}

		RuntimeOpcodeContext context;
		context.mControlFlow = mSelectedControlFlow;
		mActiveControlFlow = mSelectedControlFlow;
		mCurrentOpcodePtr = &context.mOpcode;

		// Outer loop
		//  -> Gets restarted whenever the currently running function changes
		//  -> Gets exited only by a stop signal or a return
		mReceivedStopSignal = false;
		while (!mReceivedStopSignal)
		{
			ControlFlow::State& state = mSelectedControlFlow->mCallStack.back();

		#ifdef DEBUG
			// Reached the end already?
			//  -> Should not happen actually, as all functions end with a return opcode
			//  -> That's why this check is only active in debug builds
			if (state.mProgramCounter > state.mRuntimeFunction->mRuntimeOpcodeBuffer.getEnd())
			{
				RMX_ASSERT(false, "Program counter exceeded the end of function");
				returnFromFunction();
				result.mResult = ExecuteResult::Result::HALT;
				mActiveControlFlow = nullptr;
				return;
			}
		#endif

			RMX_CHECK(mSelectedControlFlow->mValueStackPtr >= mSelectedControlFlow->mValueStackStart, "Value stack error: Removed elements from empty stack", mSelectedControlFlow->mValueStackPtr = mSelectedControlFlow->mValueStackStart);
			RMX_CHECK(mSelectedControlFlow->mValueStackPtr < &mSelectedControlFlow->mValueStackBuffer[ControlFlow::VALUE_STACK_LAST_INDEX], "Value stack error: Too many elements", mSelectedControlFlow->mValueStackPtr = &mSelectedControlFlow->mValueStackBuffer[0x77]);

			RMX_ASSERT(mSelectedControlFlow->mLocalVariablesSize <= ControlFlow::VAR_STACK_LIMIT, "Reached var stack limit");
			mSelectedControlFlow->mCurrentLocalVariables = reinterpret_cast<uint8*>(&mSelectedControlFlow->mLocalVariablesBuffer[state.mLocalVariablesStart]);
			RMX_ASSERT(nullptr != mSelectedControlFlow->mCurrentLocalVariables, "Reached var stack limit");

			context.mOpcode = (const RuntimeOpcode*)state.mProgramCounter;

			// Inner loop
			//  -> Main execution of opcodes inside a single function
			//  -> Not exited on jumps
			//  -> Gets exited by changing stayInsideInnerLoop when the running function was changed
			//  -> Gets exited by a return when control needs to be returned to the caller
			bool stayInsideInnerLoop = true;
			while (stayInsideInnerLoop)
			{
				while (context.mOpcode->mSuccessiveHandledOpcodes > 0)
				{
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

						result.mStepsExecuted += 4;
					}
					else
					{
						(*context.mOpcode->mExecFunc)(context);
						context.mOpcode = context.mOpcode->mNext;

						++result.mStepsExecuted;
					}
				}

				switch (context.mOpcode->mOpcodeType)
				{
					case Opcode::Type::JUMP_CONDITIONAL:
					{
						--mSelectedControlFlow->mValueStackPtr;
						if (*mSelectedControlFlow->mValueStackPtr != 0)
						{
							context.mOpcode = context.mOpcode->mNext;
							++result.mStepsExecuted;
							break;
						}

						// Fallthrough to unconditional jump
					}

					case Opcode::Type::JUMP:
					{
						state.mProgramCounter = reinterpret_cast<const uint8*>(context.mOpcode->getParameter<uint64>());

						// Check if steps limit is reached (this usually means the limit was exceeded already, but that's okay)
						//  -> This is needed to prevent endless loops
						++result.mStepsExecuted;
						if (result.mStepsExecuted >= stepsLimit)
						{
							mActiveControlFlow = nullptr;
							return;
						}

						context.mOpcode = (const RuntimeOpcode*)state.mProgramCounter;
						break;
					}

					case Opcode::Type::JUMP_SWITCH:
					{
						// Jump if top of stack is zero
						if (mSelectedControlFlow->mValueStackPtr[-1] == 0)
						{
							--mSelectedControlFlow->mValueStackPtr;
							context.mOpcode = reinterpret_cast<const RuntimeOpcode*>(context.mOpcode->getParameter<uint64>());
						}
						else
						{
							// Otherwise decrease it and go on with the next opcode
							--mSelectedControlFlow->mValueStackPtr[-1];
							context.mOpcode = context.mOpcode->mNext;
							++result.mStepsExecuted;
						}
						break;
					}

					case Opcode::Type::CALL:
					{
						state.mProgramCounter = (uint8*)context.mOpcode->mNext;
						const uint64 callTarget = context.mOpcode->getParameter<uint64>();
						++result.mStepsExecuted;

						const Function* func = handleResultCall(*context.mOpcode);
						if (result.handleCall(func, callTarget))
						{
							// Restart the outer loop now that the running function has changed
							stayInsideInnerLoop = false;
							break;
						}
						else
						{
							// Call handling failed, return control to the caller
							mActiveControlFlow = nullptr;
							return;
						}
					}

					case Opcode::Type::RETURN:
					{
						mSelectedControlFlow->mLocalVariablesSize = mSelectedControlFlow->mCallStack.back().mLocalVariablesStart;
						mSelectedControlFlow->mCallStack.pop_back();
						++result.mStepsExecuted;

						if (result.handleReturn())
						{
							// Check stop conditions
							if (mSelectedControlFlow->mCallStack.count > minimumCallStackSize && result.mStepsExecuted < stepsLimit)
							{
								// Restart the outer loop now that the running function has changed
								stayInsideInnerLoop = false;
								break;
							}
						}

						// Handling failed or a stop condition triggered, return control to the caller
						mActiveControlFlow = nullptr;
						return;
					}

					case Opcode::Type::EXTERNAL_CALL:
					{
						state.mProgramCounter = (uint8*)context.mOpcode + context.mOpcode->mSize;
						--mSelectedControlFlow->mValueStackPtr;
						const uint64 targetAddress = *mSelectedControlFlow->mValueStackPtr;
						++result.mStepsExecuted;

						if (result.handleExternalCall(targetAddress))
						{
							// Restart the outer loop now that the running function has changed
							stayInsideInnerLoop = false;
							break;
						}
						else
						{
							mActiveControlFlow = nullptr;
							return;
						}
					}

					case Opcode::Type::EXTERNAL_JUMP:
					{
						state.mProgramCounter = (uint8*)context.mOpcode + context.mOpcode->mSize;
						--mSelectedControlFlow->mValueStackPtr;
						returnFromFunction();
						const uint64 targetAddress = *mSelectedControlFlow->mValueStackPtr;
						++result.mStepsExecuted;

						if (result.handleExternalJump(targetAddress))
						{
							// Check stop conditions
							if (mSelectedControlFlow->mCallStack.count > minimumCallStackSize && result.mStepsExecuted < stepsLimit)
							{
								// Restart the outer loop now that the running function has changed
								stayInsideInnerLoop = false;
								break;
							}
						}

						mActiveControlFlow = nullptr;
						return;
					}

					default:
						throw std::runtime_error("Unhandled opcode");
				}
			}
		}

		// Outer loop was exited by a stop signal
		mActiveControlFlow = nullptr;
	}

	const Function* Runtime::handleResultCall(const RuntimeOpcode& runtimeOpcode)
	{
		// Consider base function call, if additional data says so
		const size_t baseCallIndex = runtimeOpcode.mFlags.isSet(RuntimeOpcode::Flag::CALL_IS_BASE_CALL) ? (mSelectedControlFlow->getState().mBaseCallIndex + 1) : 0;

		if (runtimeOpcode.mFlags.isSet(RuntimeOpcode::Flag::CALL_TARGET_RUNTIME_FUNC))
		{
			// Take the runtime function shortcut (this is the most common one)
			const RuntimeFunction* runtimeFunction = runtimeOpcode.getParameter<const RuntimeFunction*>();
			callRuntimeFunction(*runtimeFunction, baseCallIndex);
			return runtimeFunction->mFunction;
		}
		else if (runtimeOpcode.mFlags.isSet(RuntimeOpcode::Flag::CALL_TARGET_RESOLVED))
		{
			// Take the shortcut to a normal function
			const Function* function = runtimeOpcode.getParameter<const Function*>();
			callFunction(*function, baseCallIndex);
			return function;
		}
		else
		{
			const uint64 callTarget = runtimeOpcode.getParameter<uint64>();
			RuntimeOpcode& runtimeOpcodeMutable = const_cast<RuntimeOpcode&>(runtimeOpcode);

			// If it's a script function call, there should be an associated runtime function that can be called directly
			RuntimeFunction* runtimeFunction = getRuntimeFunctionBySignature(callTarget, baseCallIndex);
			if (nullptr != runtimeFunction)
			{
				// Create a shortcut for next time
				runtimeOpcodeMutable.setParameter(runtimeFunction);
				runtimeOpcodeMutable.mFlags.set(RuntimeOpcode::Flag::CALL_TARGET_RUNTIME_FUNC);

				// Call the function now
				callRuntimeFunction(*runtimeFunction, baseCallIndex);
				return runtimeFunction->mFunction;
			}

			// Another try, in case it's not a script function
			const Function* function = mProgram->getFunctionBySignature(callTarget, baseCallIndex);
			if (nullptr != function)
			{
				// Create a shortcut for next time
				runtimeOpcodeMutable.setParameter(function);
				runtimeOpcodeMutable.mFlags.set(RuntimeOpcode::Flag::CALL_TARGET_RESOLVED);

				// Call the function now
				callFunction(*function, baseCallIndex);
				return function;
			}

			// Failed
			return nullptr;
		}
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

					if (nullptr == function || !function->isA<ScriptFunction>())
					{
						if (nullptr != outError)
							*outError = "Could not match function signature for script function of name '" + std::string(functionName) + "'";
						controlFlow.mCallStack.clear();
						return false;
					}

					RuntimeFunction* runtimeFunction = getRuntimeFunction(function->as<ScriptFunction>());
					controlFlow.mCallStack[i].mRuntimeFunction = runtimeFunction;
					controlFlow.mCallStack[i].mProgramCounter = runtimeFunction->translateToRuntimeProgramCounter(serializer.read<uint32>());

					controlFlow.mCallStack[i].mLocalVariablesStart = controlFlow.mLocalVariablesSize;
					const size_t numLocalVars = serializer.read<uint32>();
					for (size_t k = controlFlow.mLocalVariablesSize; k < controlFlow.mLocalVariablesSize + numLocalVars; ++k)
					{
						controlFlow.mLocalVariablesBuffer[k] = serializer.read<int64>();
					}
					controlFlow.mLocalVariablesSize += numLocalVars;
					RMX_CHECK(controlFlow.mLocalVariablesSize <= ControlFlow::VAR_STACK_LIMIT, "Reached var stack limit, probably due to recursive function calls", RMX_REACT_THROW);
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
		const size_t numGlobals = mProgram->getGlobalVariables().size();
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
						const size_t offset = static_cast<GlobalVariable*>(variable)->getStaticMemoryOffset();
						if (offset < mStaticMemory.size())
							memcpy(&mStaticMemory[offset], &value, sizeof(int64));
					}
				}
			}
			else
			{
				serializer.writeAs<uint32>(numGlobals);
				for (size_t i = 0; i < numGlobals; ++i)
				{
					Variable* variable = mProgram->getGlobalVariables()[i];
					serializer.write(variable->getName().getString());
					const size_t offset = (variable->getType() == Variable::Type::GLOBAL) ? static_cast<GlobalVariable*>(variable)->getStaticMemoryOffset() : 0xffffffff;
					if (offset < mStaticMemory.size())
						serializer.write(&mStaticMemory[offset], sizeof(int64));
					else
						serializer.write<int64>(0);
				}
			}
		}
		else
		{
			// Legacy code for version 0x00
			if (serializer.isReading())
			{
				const size_t numGlobalsSerialized = (size_t)serializer.read<uint32>();
				const size_t numGlobalsShared = std::min(numGlobalsSerialized, numGlobals);
				for (size_t i = 0; i < numGlobalsShared; ++i)
				{
					const int64 value = serializer.read<uint64>();
					RMX_CHECK(i < numGlobals, "Invalid global variable index", continue);
					Variable* variable = mProgram->getGlobalVariables()[i];
					if (variable->getType() == Variable::Type::GLOBAL)
					{
						const size_t offset = static_cast<GlobalVariable*>(variable)->getStaticMemoryOffset();
						memcpy(&mStaticMemory[offset], &value, sizeof(int64));
					}
				}
				if (numGlobalsSerialized > numGlobals)
				{
					serializer.skip((numGlobalsShared - numGlobals) * 8);
				}
			}
			else
			{
				RMX_ERROR("Saving as version 0x00 is not supported", );
			}
		}

		// Done
		return true;
	}

	void Runtime::setupGlobalVariables()
	{
		if (nullptr == mProgram)
			return;

		// Setup memory offsets and sizes
		size_t totalSize = 0;
		for (size_t index = 0; index < mProgram->getGlobalVariables().size(); ++index)
		{
			Variable& var = *mProgram->getGlobalVariables()[index];
			if (var.getType() == Variable::Type::GLOBAL)	// The other variable types don't use static memory size
			{
				GlobalVariable& variable = static_cast<GlobalVariable&>(var);

				size_t variableSize = variable.getDataType()->getBytes();
				variableSize = (variableSize + 7) / 8 * 8;		// Align to multiples of 8 bytes (i.e. int64 size)
				variable.mStaticMemoryOffset = totalSize;
				variable.mStaticMemorySize = variableSize;
				totalSize += variableSize;
			}
		}

		mStaticMemory.resize(totalSize);

		for (size_t index = 0; index < mProgram->getGlobalVariables().size(); ++index)
		{
			Variable& var = *mProgram->getGlobalVariables()[index];
			if (var.getType() == Variable::Type::GLOBAL)
			{
				GlobalVariable& variable = static_cast<GlobalVariable&>(var);
				if (variable.getStaticMemorySize() > 0)
				{
					const int64 value = (variable.getType() == Variable::Type::GLOBAL) ? static_cast<GlobalVariable&>(variable).mInitialValue.get<int64>() : 0;
					*(int64*)&mStaticMemory[variable.getStaticMemoryOffset()] = value;
				}
			}
		}
	}

}
