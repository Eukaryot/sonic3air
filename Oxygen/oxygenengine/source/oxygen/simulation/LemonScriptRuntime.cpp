/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/LemonScriptRuntime.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/Profiling.h"
#include "oxygen/helper/Utils.h"

#include <lemon/compiler/TokenManager.h>
#include <lemon/program/GlobalsLookup.h>
#include <lemon/program/Module.h>
#include <lemon/program/Program.h>
#include <lemon/runtime/Runtime.h>
#include <lemon/runtime/RuntimeFunction.h>


namespace
{

	class RuntimeDetailHandler final : public lemon::RuntimeDetailHandler
	{
		void preExecuteExternalFunction(const lemon::NativeFunction& function, const lemon::ControlFlow& controlFlow) override
		{
			Profiling::pushRegion(ProfilingRegion::SIMULATION_USER_CALL);
		}

		void postExecuteExternalFunction(const lemon::NativeFunction& function, const lemon::ControlFlow& controlFlow) override
		{
			Profiling::popRegion(ProfilingRegion::SIMULATION_USER_CALL);
		}
	};


	bool isOperatorToken(const lemon::Token& token, lemon::Operator op)
	{
		return token.isA<lemon::OperatorToken>() && (token.as<lemon::OperatorToken>().mOperator == op);
	}

}


struct LemonScriptRuntime::Internal
{
	lemon::Runtime mRuntime;
	RuntimeDetailHandler mRuntimeDetailHandler;
	LinearLookupTable<const lemon::RuntimeFunction*, 0x400000, 6, 1024> mAddressHookLookup;
};



bool LemonScriptRuntime::getCurrentScriptFunction(std::string_view* outFunctionName, std::wstring* outFileName, uint32* outLineNumber, std::string* outModuleName)
{
	lemon::ControlFlow* controlFlow = lemon::Runtime::getActiveControlFlow();
	if (nullptr == controlFlow)
		return false;

	lemon::ControlFlow::Location location;
	controlFlow->getLastStepLocation(location);
	if (nullptr == location.mFunction)
		return false;

	if (nullptr != outFunctionName)
		*outFunctionName = location.mFunction->getName().getString();
	if (nullptr != outFileName)
		*outFileName = location.mFunction->mSourceFileInfo->mFilename;
	if (nullptr != outLineNumber)
		*outLineNumber = getLineNumberInFile(*location.mFunction, location.mProgramCounter);
	if (nullptr != outModuleName)
		*outModuleName = location.mFunction->getModule().getModuleName();
	return true;
}

std::string LemonScriptRuntime::getCurrentScriptLocationString()
{
	lemon::ControlFlow* controlFlow = lemon::Runtime::getActiveControlFlow();
	if (nullptr == controlFlow)
		return "";

	return buildScriptLocationString(*controlFlow);
}

const std::string_view* LemonScriptRuntime::tryResolveStringHash(uint64 hash)
{
	lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
	if (nullptr == runtime)
		return nullptr;

	const lemon::FlyweightString* str = runtime->resolveStringByKey(hash);
	if (nullptr == str)
		return nullptr;

	return &str->getStringRef();
}


LemonScriptRuntime::LemonScriptRuntime(LemonScriptProgram& program, EmulatorInterface& emulatorInterface) :
	mInternal(*new Internal()),
	mProgram(program)
{
	// Memory access handler
	mInternal.mRuntime.setMemoryAccessHandler(&emulatorInterface);

#if 0
	// Detail handler (used for more detailled profiling)
	//  -> Be aware that it has quite some performance impact itself...
	mInternal.mRuntime.setRuntimeDetailHandler(&mInternal.mRuntimeDetailHandler);
#endif
}

LemonScriptRuntime::~LemonScriptRuntime()
{
	delete &mInternal;
}

lemon::Runtime& LemonScriptRuntime::getInternalLemonRuntime()
{
	return mInternal.mRuntime;
}

bool LemonScriptRuntime::hasValidProgram() const
{
	return mProgram.hasValidProgram();
}

void LemonScriptRuntime::onProgramUpdated()
{
	// Assign lemon script program to runtime, implicitly resetting the runtime as well
	mInternal.mRuntime.setProgram(mProgram.getInternalLemonProgram());

	// Reset the lookup table for address hook runtime functions
	mInternal.mAddressHookLookup.clear();

	// Build all runtime functions right away
	mInternal.mRuntime.buildAllRuntimeFunctions();
}

bool LemonScriptRuntime::serializeRuntime(VectorBinarySerializer& serializer)
{
	return mInternal.mRuntime.serializeState(serializer);
}

bool LemonScriptRuntime::callUpdateHook(bool postUpdate)
{
	const LemonScriptProgram::Hook* hook = mProgram.checkForUpdateHook(postUpdate);
	if (nullptr != hook)
	{
		RMX_ASSERT(nullptr != hook->mFunction, "Invalid update hook function");
		mInternal.mRuntime.callFunction(*hook->mFunction);
		return true;
	}
	return false;
}

bool LemonScriptRuntime::callAddressHook(uint32 address)
{
	const lemon::RuntimeFunction** runtimeFunctionPtr = mInternal.mAddressHookLookup.find(address);
	if (nullptr != runtimeFunctionPtr)
	{
		mInternal.mRuntime.callRuntimeFunction(**runtimeFunctionPtr);
	}
	else
	{
		// Get the hook from the program first
		const LemonScriptProgram::Hook* hook = mProgram.checkForAddressHook(address);
		if (nullptr == hook)
			return false;

		// Try to get the respective runtime function
		RMX_ASSERT(nullptr != hook->mFunction, "Invalid address hook function");
		const lemon::RuntimeFunction* runtimeFunction = mInternal.mRuntime.getRuntimeFunction(*hook->mFunction);
		if (nullptr != runtimeFunction)
		{
			mInternal.mAddressHookLookup.add(address, runtimeFunction);
			mInternal.mRuntime.callRuntimeFunction(*runtimeFunction);
		}
		else
		{
			RMX_ASSERT(false, "Unable to get runtime function for address hook at " << rmx::hexString(hook->mAddress, 8));
			mInternal.mRuntime.callFunction(*hook->mFunction);
		}
	}
	return true;
}

void LemonScriptRuntime::callFunction(const lemon::ScriptFunction& function)
{
	mInternal.mRuntime.callFunction(function);
}

bool LemonScriptRuntime::callFunctionByName(lemon::FlyweightString functionName, bool showErrorOnFail)
{
	return callFunctionByNameAtLabel(functionName, "", showErrorOnFail);
}

bool LemonScriptRuntime::callFunctionByNameAtLabel(lemon::FlyweightString functionName, lemon::FlyweightString labelName, bool showErrorOnFail)
{
	const bool success = mInternal.mRuntime.callFunctionByName(functionName, labelName);
	if (!success && showErrorOnFail)
	{
		if (labelName.isEmpty())
		{
			RMX_ERROR("Failed to call function '" << functionName.getString() << "'", );
		}
		else
		{
			RMX_ERROR("Failed to call label '" << labelName.getString() << "' in '" << functionName.getString() << "'", );
		}
	}
	return success;
}

size_t LemonScriptRuntime::getCallStackSize() const
{
	return mInternal.mRuntime.getMainControlFlow().getCallStack().count;
}

void LemonScriptRuntime::getCallStack(std::vector<const lemon::Function*>& outCallStack) const
{
	outCallStack.clear();
	const CArray<lemon::ControlFlow::State>& runtimeCallStack = mInternal.mRuntime.getMainControlFlow().getCallStack();
	for (size_t i = 0; i < runtimeCallStack.count; ++i)
	{
		const lemon::ControlFlow::State& state = runtimeCallStack[i];
		outCallStack.push_back(state.mRuntimeFunction->mFunction);
	}
}

void LemonScriptRuntime::getCallStackWithLabels(CallStackWithLabels& outCallStack) const
{
	outCallStack.clear();
	std::vector<lemon::ControlFlow::Location> locations;
	mInternal.mRuntime.getMainControlFlow().getCallStack(locations);
	for (const lemon::ControlFlow::Location& location : locations)
	{
		const lemon::ScriptFunction::Label* label = location.mFunction->findLabelByOffset(location.mProgramCounter);
		if (nullptr != label)
		{
			outCallStack.emplace_back(location.mFunction->getName().getString(), label->mName.getString());
		}
	}
}

const lemon::Function* LemonScriptRuntime::getCurrentFunction() const
{
	const lemon::RuntimeFunction* runtimeFunction = mInternal.mRuntime.getMainControlFlow().getCallStack().back().mRuntimeFunction;
	return (nullptr == runtimeFunction) ? nullptr : runtimeFunction->mFunction;
}

int64 LemonScriptRuntime::getGlobalVariableValue_int64(lemon::FlyweightString variableName)
{
	lemon::Variable* variable = mProgram.getGlobalVariableByHash(variableName.getHash());
	if (nullptr != variable)
	{
		return mInternal.mRuntime.getGlobalVariableValue_int64(*variable);
	}
	return 0;
}

void LemonScriptRuntime::setGlobalVariableValue_int64(lemon::FlyweightString variableName, int64 value)
{
	lemon::Variable* variable = mProgram.getGlobalVariableByHash(variableName.getHash());
	if (nullptr != variable)
	{
		mInternal.mRuntime.setGlobalVariableValue_int64(*variable, value);
	}
}

void LemonScriptRuntime::getLastStepLocation(const lemon::ScriptFunction*& outFunction, size_t& outProgramCounter) const
{
	lemon::ControlFlow::Location location;
	mInternal.mRuntime.getSelectedControlFlow().getLastStepLocation(location);
	outFunction = location.mFunction;
	outProgramCounter = location.mProgramCounter;
}

std::string LemonScriptRuntime::getOwnCurrentScriptLocationString() const
{
	return buildScriptLocationString(mInternal.mRuntime.getSelectedControlFlow());
}

std::string LemonScriptRuntime::buildScriptLocationString(const lemon::ControlFlow& controlFlow)
{
	lemon::ControlFlow::Location location;
	controlFlow.getLastStepLocation(location);
	if (nullptr == location.mFunction)
		return "";

	const std::string functionName(location.mFunction->getName().getString());
	const std::wstring& fileName = location.mFunction->mSourceFileInfo->mFilename;
	const uint32 lineNumber = getLineNumberInFile(*location.mFunction, location.mProgramCounter);
	const std::string& moduleName = location.mFunction->getModule().getModuleName();
	return "function '" + functionName + "' at line " + std::to_string(lineNumber) + " of file '" + WString(fileName).toStdString() + "' in module '" + moduleName + "'";
}

uint32 LemonScriptRuntime::getLineNumberInFile(const lemon::ScriptFunction& function, size_t programCounter)
{
	const auto& opcodes = function.mOpcodes;
	const uint32 lineNumber = (programCounter < opcodes.size()) ? opcodes[programCounter].mLineNumber : opcodes.back().mLineNumber;
	return (lineNumber < function.mSourceBaseLineOffset) ? 0 : (lineNumber - function.mSourceBaseLineOffset);
}
