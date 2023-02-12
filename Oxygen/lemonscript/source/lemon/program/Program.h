/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/ConstantArray.h"
#include "lemon/program/Define.h"
#include "lemon/program/Function.h"
#include <unordered_map>


namespace lemon
{
	class Module;
	class StringLookup;
	class NativizedOpcodeProvider;
	class MemoryAccessHandler;

	class API_EXPORT Program
	{
	public:
		NativizedOpcodeProvider* mNativizedOpcodeProvider = nullptr;	// TODO: Is this even a good place for this?

	public:
		Program();
		~Program();

		inline const std::vector<const Module*>& getModules() const  { return mModules; }
		void clear();
		void addModule(const Module& module);

		void runNativization(const Module& module, const std::wstring& outputFilename, MemoryAccessHandler& memoryAccessHandler);

		// Functions
		inline const std::vector<Function*>& getFunctions() const  { return mFunctions; }
		inline const std::vector<ScriptFunction*>& getScriptFunctions() const  { return mScriptFunctions; }
		const Function* getFunctionByID(uint32 id) const;
		const Function* getFunctionBySignature(uint64 nameAndSignatureHash, size_t index = 0) const;
		const std::vector<Function*>& getFunctionsByName(uint64 nameHash) const;

		// Variables
		inline const std::vector<Variable*>& getGlobalVariables() const  { return mGlobalVariables; }
		Variable& getGlobalVariableByID(uint32 id) const;
		Variable* getGlobalVariableByName(uint64 nameHash) const;

		// Constant arrays
		inline const std::vector<ConstantArray*>& getConstantArrays() const  { return mConstantArrays; }

		// Defines
		inline const std::vector<Define*>& getDefines() const  { return mDefines; }

		// String literals
		void collectAllStringLiterals(StringLookup& outStrings) const;

		// Data types
		const DataTypeDefinition* getDataTypeByID(uint16 dataTypeID) const;

		inline int getOptimizationLevel() const  { return mOptimizationLevel; }
		void setOptimizationLevel(int level)	 { mOptimizationLevel = level; }

	private:
		void clearInternal();

	private:
		// Modules
		std::vector<const Module*> mModules;

		// Functions
		std::vector<Function*> mFunctions;
		std::vector<ScriptFunction*> mScriptFunctions;
		std::unordered_map<uint64, std::vector<Function*>> mFunctionsBySignature;	// Key is the hashed function name + signature hash
		std::unordered_map<uint64, std::vector<Function*>> mFunctionsByName;		// Key is the hashed function name

		// Variables
		std::vector<Variable*> mGlobalVariables;
		std::unordered_map<uint64, Variable*> mGlobalVariablesByName;

		// Constant arrays
		std::vector<ConstantArray*> mConstantArrays;

		// Defines
		std::vector<Define*> mDefines;

		// Data types
		std::vector<const DataTypeDefinition*> mDataTypes;

		int mOptimizationLevel = 3;
	};

}
