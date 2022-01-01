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


namespace lemon
{
	class Function;
	class GlobalsLookup;
	class Program;
	class RuntimeFunction;
	class ScriptFunction;
	class Variable;
}


class LemonScriptProgram
{
public:
	struct LoadOptions
	{
		enum class ModuleSelection
		{
			BASE_GAME_ONLY,
			//SELECTED_MODS,	// TODO: Implement this as well, with a set of mod names
			ALL_MODS
		};

		lemon::PreprocessorDefinitionMap mPreprocessorDefinitions;
		bool mEnforceFullReload = false;
		ModuleSelection mModuleSelection = ModuleSelection::ALL_MODS;
	};

	struct GlobalDefine
	{
		std::string mName;
		uint32 mAddress = 0;
		uint8 mBytes = 0;
		bool mSigned = false;
		uint64 mCategoryHash = 0;	// Hash of first part of the name (before the first dot)
	};

	struct Hook
	{
		enum class Type
		{
			PRE_UPDATE,		// Called once per frame
			POST_UPDATE,	// Called once per frame
			ADDRESS			// Reacts on program counter address
		};

		Type   mType = Type::ADDRESS;
		uint32 mAddress = 0;
		uint32 mIndex = 0;
		const lemon::ScriptFunction* mFunction = nullptr;	// Only really used for update hooks
	};

public:
	LemonScriptProgram();
	~LemonScriptProgram();

	void startup();

	LemonScriptBindings& getLemonScriptBindings();
	lemon::Program& getInternalLemonProgram();

	bool hasValidProgram() const;
	bool loadScripts(const std::string& filename, const LoadOptions& loadOptions);

	const Hook* checkForUpdateHook(bool post);
	const Hook* checkForAddressHook(uint32 address);
	size_t getNumAddressHooks() const;

	const std::string& getFunctionNameByHash(uint64 hash) const;
	lemon::Variable* getGlobalVariableByHash(uint64 hash) const;
	const std::vector<GlobalDefine>& getGlobalDefines() const  { return mGlobalDefines; }

	void resolveLocation(uint32 functionId, uint32 programCounter, std::string& scriptFilename, uint32& lineNumber) const;

public:
	static void resolveLocation(const lemon::Function& function, uint32 programCounter, std::string& scriptFilename, uint32& lineNumber);

private:
	bool loadScriptModule(lemon::Module& module, lemon::GlobalsLookup& globalsLookup, const std::wstring& filename, const lemon::PreprocessorDefinitionMap& preprocessorDefinitions);
	void evaluateFunctionPragmas();
	void evaluateDefines();

	Hook& addHook(Hook::Type type, uint32 address, const std::string& functionName);

private:
	struct Internal;
	Internal& mInternal;

	std::vector<GlobalDefine> mGlobalDefines;
};
