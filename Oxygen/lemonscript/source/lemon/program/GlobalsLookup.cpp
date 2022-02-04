/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/program/Module.h"


namespace lemon
{

	void GlobalsLookup::clear()
	{
		mFunctionsByName.clear();
		mGlobalVariablesByName.clear();
		mDefinesByName.clear();
		mConstantsByName.clear();
	}

	void GlobalsLookup::addDefinitionsFromModule(const Module& module)
	{
		for (Function* function : module.mFunctions)
		{
			registerFunction(*function);
		}
		for (Variable* variable : module.mGlobalVariables)
		{
			registerVariable(*variable);
		}
		for (Constant* constant : module.mConstants)
		{
			registerConstant(*constant);
		}
		for (ConstantArray* constantArray : module.mConstantArrays)
		{
			registerConstantArray(*constantArray);
		}
		for (Define* define : module.mDefines)
		{
			registerDefine(*define);
		}

		RMX_ASSERT(mNextFunctionID == module.mFirstFunctionID, "Mismatch in function ID when adding module '" << module.getModuleName() << "' (" << mNextFunctionID << " vs. " << module.mFirstFunctionID << ")");
		RMX_ASSERT(mNextVariableID == module.mFirstVariableID, "Mismatch in variable ID when adding module '" << module.getModuleName() << "' (" << mNextVariableID << " vs. " << module.mFirstVariableID << ")");
		RMX_ASSERT(mNextConstantArrayID == module.mFirstConstantArrayID, "Mismatch in constant array ID when adding module '" << module.getModuleName() << "' (" << mNextConstantArrayID << " vs. " << module.mFirstConstantArrayID << ")");
		mNextFunctionID += (uint32)module.mFunctions.size();
		mNextVariableID += (uint32)module.mGlobalVariables.size();
		mNextConstantArrayID += (uint32)module.mConstantArrays.size();
	}

	const std::vector<Function*>& GlobalsLookup::getFunctionsByName(uint64 nameHash) const
	{
		static const std::vector<Function*> EMPTY_FUNCTIONS;
		const auto it = mFunctionsByName.find(nameHash);
		return (it == mFunctionsByName.end()) ? EMPTY_FUNCTIONS : it->second;
	}

	void GlobalsLookup::registerFunction(Function& function)
	{
		mFunctionsByName[function.getName().getHash()].push_back(&function);
	}

	const Variable* GlobalsLookup::getGlobalVariableByName(uint64 nameHash) const
	{
		const auto it = mGlobalVariablesByName.find(nameHash);
		return (it == mGlobalVariablesByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerVariable(Variable& variable)
	{
		const uint64 nameHash = variable.getName().getHash();
		mGlobalVariablesByName[nameHash] = &variable;
	}

	const Constant* GlobalsLookup::getConstantByName(uint64 nameHash) const
	{
		const auto it = mConstantsByName.find(nameHash);
		return (it == mConstantsByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerConstant(Constant& constant)
	{
		const uint64 nameHash = constant.getName().getHash();
		mConstantsByName[nameHash] = &constant;
	}

	const ConstantArray* GlobalsLookup::getConstantArrayByName(uint64 nameHash) const
	{
		const auto it = mConstantArraysByName.find(nameHash);
		return (it == mConstantArraysByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerConstantArray(ConstantArray& constantArray)
	{
		const uint64 nameHash = constantArray.getName().getHash();
		mConstantArraysByName[nameHash] = &constantArray;
	}

	const Define* GlobalsLookup::getDefineByName(uint64 nameHash) const
	{
		const auto it = mDefinesByName.find(nameHash);
		return (it == mDefinesByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerDefine(Define& define)
	{
		const uint64 nameHash = define.getName().getHash();
		mDefinesByName[nameHash] = &define;
	}

	const FlyweightString* GlobalsLookup::getStringLiteralByHash(uint64 hash) const
	{
		return mStringLiterals.getStringByHash(hash);
	}

}
