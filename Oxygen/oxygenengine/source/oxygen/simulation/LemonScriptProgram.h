/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/bindings/LemonScriptBindings.h"

#include <lemon/program/function/ScriptFunction.h>


class Mod;
namespace lemon
{
	class GlobalsLookup;
	class Program;
	class RuntimeFunction;
	class ScriptFunction;
	class Variable;
	struct SourceFileInfo;
}


class LemonScriptProgram
{
public:
	struct ResolvedLocation
	{
		const lemon::SourceFileInfo* mSourceFileInfo = nullptr;
		std::string mScriptFilename;
		uint32 mLineNumber = 0;
	};

	struct LoadOptions
	{
		enum class ModuleSelection
		{
			BASE_GAME_ONLY,
			//SELECTED_MODS,	// TODO: Implement this as well, with a set of mod names
			ALL_MODS
		};

		bool mEnforceFullReload = false;
		ModuleSelection mModuleSelection = ModuleSelection::ALL_MODS;
		uint32 mAppVersion = 0;
	};

	enum class LoadScriptsResult
	{
		NO_CHANGE,
		PROGRAM_CHANGED,
		FAILED
	};

	struct GlobalDefine
	{
		lemon::FlyweightString mName;
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
			ADDRESS			// Reacts to program counter address
		};

		Type   mType = Type::ADDRESS;
		uint32 mAddress = 0;
		uint32 mIndex = 0;
		const lemon::ScriptFunction* mFunction = nullptr;
		const lemon::ScriptFunction::Label* mLabel = nullptr;	// Only used for address-hooks at labels inside functions
	};

public:
	LemonScriptProgram();
	~LemonScriptProgram();

	LemonScriptBindings& getLemonScriptBindings();
	lemon::Program& getInternalLemonProgram();

	bool hasValidProgram() const;
	LoadScriptsResult loadScripts(std::string_view baseScriptFilename, const LoadOptions& loadOptions);

	const Hook* checkForUpdateHook(bool post);
	const Hook* checkForAddressHook(uint32 address);
	size_t getNumAddressHooks() const;

	std::string_view getFunctionNameByHash(uint64 hash) const;
	lemon::Variable* getGlobalVariableByHash(uint64 hash) const;
	const std::vector<GlobalDefine>& getGlobalDefines() const  { return mGlobalDefines; }

	const Mod* getModByModule(const lemon::Module& module) const;
	const std::vector<const lemon::Module*>& getModules() const;

	void resolveLocation(ResolvedLocation& outResolvedLocation, uint32 functionId, uint32 programCounter) const;

public:
	static void resolveLocation(ResolvedLocation& outResolvedLocation, const lemon::Function& function, uint32 programCounter);

private:
	enum class LoadingResult
	{
		SUCCESS,
		FAILED_CONTINUE,
		FAILED_RETRY
	};

private:
	LoadingResult loadAllScriptModules(const LoadOptions& loadOptions, std::string_view baseScriptFilename, const std::vector<const Mod*>& modsToLoad);
	LoadingResult loadScriptModule(lemon::Module& module, lemon::GlobalsLookup& globalsLookup, const std::wstring& filename);
	void collectHooksFromFunctions();
	void evaluateDefines();

	Hook& addHook(Hook::Type type, uint32 address);

private:
	struct Internal;
	Internal& mInternal;

	std::vector<GlobalDefine> mGlobalDefines;
};
