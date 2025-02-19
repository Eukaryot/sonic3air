/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/bindings/LemonScriptBindings.h"

#include <lemon/compiler/PreprocessorDefinition.h>
#include <lemon/program/FunctionWrapper.h>


class EmulatorInterface;
class LemonScriptProgram;
namespace lemon
{
	class ControlFlow;
	class Function;
	class GlobalsLookup;
	class Runtime;
	class RuntimeFunction;
	class ScriptFunction;
}


class LemonScriptRuntime
{
public:
	typedef std::vector<std::pair<std::string, std::string>> CallStackWithLabels;

public:
	static bool getCurrentScriptFunction(std::string_view* outFunctionName, std::wstring* outFileName, uint32* outLineNumber, std::string* outModuleName);
	static std::string getCurrentScriptLocationString();
	static const std::string_view* tryResolveStringHash(uint64 hash);

public:
	LemonScriptRuntime(LemonScriptProgram& program, EmulatorInterface& emulatorInterface);
	~LemonScriptRuntime();

	lemon::Runtime& getInternalLemonRuntime();
	inline LemonScriptProgram& getLemonScriptProgram() { return mProgram; }

	bool hasValidProgram() const;
	void onProgramUpdated();

	bool serializeRuntime(VectorBinarySerializer& serializer);

	bool callUpdateHook(bool postUpdate);
	bool callAddressHook(uint32 address);

	void callFunction(const lemon::ScriptFunction& function);
	bool callFunctionByName(lemon::FlyweightString functionName, bool showErrorOnFail = true);
	bool callFunctionByNameAtLabel(lemon::FlyweightString functionName, lemon::FlyweightString labelName, bool showErrorOnFail = true);

	size_t getCallStackSize() const;
	void getCallStack(std::vector<const lemon::Function*>& outCallStack) const;
	void getCallStackWithLabels(CallStackWithLabels& outCallStack) const;
	const lemon::Function* getCurrentFunction() const;

	lemon::AnyBaseValue getGlobalVariableValue(lemon::FlyweightString variableName, const lemon::DataTypeDefinition* dataType);
	void setGlobalVariableValue(lemon::FlyweightString variableName, lemon::AnyBaseValue value, const lemon::DataTypeDefinition* dataType);

	template<typename T>
	T getGlobalVariableValue(lemon::FlyweightString variableName)
	{
		return getGlobalVariableValue(variableName, lemon::traits::getDataType<T>()).get<T>();
	}

	template<typename T>
	void setGlobalVariableValue(lemon::FlyweightString variableName, T value)
	{
		lemon::AnyBaseValue valueToSet;
		valueToSet.set<T>(value);
		setGlobalVariableValue(variableName, valueToSet, lemon::traits::getDataType<T>());
	}

	void getCurrentExecutionLocation(const lemon::ScriptFunction*& outFunction, size_t& outProgramCounter) const;
	std::string getOwnCurrentScriptLocationString() const;

private:
	static std::string buildScriptLocationString(const lemon::ControlFlow& controlFlow);
	static uint32 getLineNumberInFile(const lemon::ScriptFunction& function, size_t programCounter);

private:
	struct Internal;
	Internal& mInternal;

	LemonScriptProgram& mProgram;
};
