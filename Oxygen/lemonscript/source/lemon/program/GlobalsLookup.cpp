/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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
		for (Define* define : module.mDefines)
		{
			registerDefine(*define);
		}

		RMX_ASSERT(mNextFunctionId == module.mFirstFunctionId, "Mismatch in function ID when adding module '" << module.getModuleName() << "' (" << mNextFunctionId << " vs. " << module.mFirstFunctionId << ")");
		RMX_ASSERT(mNextVariableId == module.mFirstVariableId, "Mismatch in variable ID when adding module '" << module.getModuleName() << "' (" << mNextVariableId << " vs. " << module.mFirstVariableId << ")");
		mNextFunctionId += (uint32)module.mFunctions.size();
		mNextVariableId += (uint32)module.mGlobalVariables.size();
	}

	const std::vector<Function*>& GlobalsLookup::getFunctionsByName(uint64 nameHash) const
	{
		static const std::vector<Function*> EMPTY_FUNCTIONS;
		const auto it = mFunctionsByName.find(nameHash);
		return (it == mFunctionsByName.end()) ? EMPTY_FUNCTIONS : it->second;
	}

	void GlobalsLookup::registerFunction(Function& function)
	{
		mFunctionsByName[function.getNameHash()].push_back(&function);
	}

	const Variable* GlobalsLookup::getGlobalVariableByName(uint64 nameHash) const
	{
		const auto it = mGlobalVariablesByName.find(nameHash);
		return (it == mGlobalVariablesByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerVariable(Variable& variable)
	{
		mGlobalVariablesByName[variable.getNameHash()] = &variable;
	}

	const Constant* GlobalsLookup::getConstantByName(uint64 nameHash) const
	{
		const auto it = mConstantsByName.find(nameHash);
		return (it == mConstantsByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerConstant(Constant& constant)
	{
		const uint64 nameHash = rmx::getMurmur2_64(constant.getName());
		mConstantsByName[nameHash] = &constant;
	}

	const Define* GlobalsLookup::getDefineByName(uint64 nameHash) const
	{
		const auto it = mDefinesByName.find(nameHash);
		return (it == mDefinesByName.end()) ? nullptr : it->second;
	}

	void GlobalsLookup::registerDefine(Define& define)
	{
		const uint64 nameHash = rmx::getMurmur2_64(define.getName());
		mDefinesByName[nameHash] = &define;
	}

	const StoredString* GlobalsLookup::getStringLiteralByHash(uint64 hash) const
	{
		return mStringLiterals.getStringByHash(hash);
	}

}
