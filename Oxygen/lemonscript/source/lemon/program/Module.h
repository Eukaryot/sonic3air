/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Constant.h"
#include "lemon/program/ConstantArray.h"
#include "lemon/program/Define.h"
#include "lemon/program/Function.h"
#include "lemon/program/StoredString.h"
#include <unordered_map>


namespace lemon
{
	class GlobalsLookup;

	class API_EXPORT Module
	{
	friend class Program;
	friend class GlobalsLookup;
	friend class ScriptFunction;

	public:
		explicit Module(const std::string& name);
		~Module();

		inline const std::string& getModuleName() const { return mModuleName; }
		inline uint64 getModuleId() const { return mModuleId; }

		void clear();

		void startCompiling(const GlobalsLookup& globalsLookup);

		void dumpDefinitionsToScriptFile(const std::wstring& filename);

		// Functions
		inline const std::vector<ScriptFunction*>& getScriptFunctions() const { return mScriptFunctions; }
		const Function* getFunctionByUniqueId(uint64 uniqueId) const;

		ScriptFunction& addScriptFunction(std::string_view name, const DataTypeDefinition* returnType, const Function::ParameterList* parameters = nullptr);
		UserDefinedFunction& addUserDefinedFunction(std::string_view name, const UserDefinedFunction::FunctionWrapper& functionWrapper, uint8 flags = 0);

		// Variables
		inline const std::vector<Variable*>& getGlobalVariables() const  { return mGlobalVariables; }
		GlobalVariable& addGlobalVariable(std::string_view name, const DataTypeDefinition* dataType);
		UserDefinedVariable& addUserDefinedVariable(std::string_view name, const DataTypeDefinition* dataType);
		ExternalVariable& addExternalVariable(std::string_view name, const DataTypeDefinition* dataType);

		// Constants
		Constant& addConstant(std::string_view name, const DataTypeDefinition* dataType, uint64 value);

		// Constant arrays
		ConstantArray& addConstantArray(std::string_view name, const DataTypeDefinition* elementDataType, const uint64* values, size_t size);

		// Defines
		Define& addDefine(std::string_view name, const DataTypeDefinition* dataType);

		// String literals
		const StringLookup& getStringLiterals() const  { return mStringLiterals; }
		const StoredString* addStringLiteral(std::string_view str);
		const StoredString* addStringLiteral(std::string_view str, uint64 hash);

		// Serialization
		bool serialize(VectorBinarySerializer& serializer);

		inline uint64 getCompiledCodeHash() const     { return mCompiledCodeHash; }
		inline void setCompiledCodeHash(uint64 hash)  { mCompiledCodeHash = hash; }

	private:
		void addFunctionInternal(Function& func);
		void addGlobalVariable(Variable& variable, std::string_view name, uint64 nameHash, const DataTypeDefinition* dataType);
		LocalVariable& createLocalVariable();
		void destroyLocalVariable(LocalVariable& variable);

	private:
		std::string mModuleName;
		uint64 mModuleId = 0;

		// Functions
		uint32 mFirstFunctionId = 0;
		std::vector<Function*> mFunctions;
		std::vector<ScriptFunction*> mScriptFunctions;
		ObjectPool<ScriptFunction, 64> mScriptFunctionPool;
		ObjectPool<UserDefinedFunction, 16> mUserDefinedFunctionPool;

		// Variables
		uint32 mFirstVariableId = 0;
		std::vector<Variable*> mGlobalVariables;
		ObjectPool<LocalVariable, 16> mLocalVariablesPool;

		// Constants
		std::vector<Constant*> mConstants;
		ObjectPool<Constant, 64> mConstantPool;

		// Constant arrays
		std::vector<ConstantArray*> mConstantArrays;
		ObjectPool<ConstantArray, 16> mConstantArrayPool;

		// Defines
		std::vector<Define*> mDefines;
		ObjectPool<Define, 64> mDefinePool;

		// String literals
		StringLookup mStringLiterals;

		// Misc
		uint64 mCompiledCodeHash = 0;
	};

}
