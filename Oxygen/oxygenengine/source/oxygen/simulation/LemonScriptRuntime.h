/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/LemonScriptBindings.h"

#include <lemon/compiler/PreprocessorDefinition.h>


class EmulatorInterface;
class LemonScriptProgram;
namespace lemon
{
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
	static bool getCurrentScriptFunction(std::string* outFunctionName, std::wstring* outFileName, uint32* outLineNumber, std::string* outModuleName);
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
	bool callFunctionByName(const std::string& functionName, bool showErrorOnFail = true);
	bool callFunctionByNameAtLabel(const std::string& functionName, const std::string& labelName, bool showErrorOnFail = true);

	size_t getCallStackSize() const;
	void getCallStack(std::vector<const lemon::Function*>& outCallStack) const;
	void getCallStackWithLabels(CallStackWithLabels& outCallStack) const;
	const lemon::Function* getCurrentFunction() const;

	void setGlobalVariableValue(const std::string& variableName, uint64 value);

	void getLastStepLocation(const lemon::ScriptFunction*& outFunction, size_t& outProgramCounter) const;
	std::string getOwnCurrentScriptLocationString() const;

private:
	static std::string buildScriptLocationString(lemon::Runtime& runtime);
	static uint32 getLineNumberInFile(const lemon::ScriptFunction& function, size_t programCounter);

private:
	struct Internal;
	Internal& mInternal;

	LemonScriptProgram& mProgram;
};
