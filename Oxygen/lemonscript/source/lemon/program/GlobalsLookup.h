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
	class Module;

	class GlobalsLookup
	{
	friend class Module;

	public:
		void clear();
		void addDefinitionsFromModule(const Module& module);

		// Functions
		const std::vector<Function*>& getFunctionsByName(uint64 nameHash) const;
		void registerFunction(Function& function);

		// Variables
		const Variable* getGlobalVariableByName(uint64 nameHash) const;
		void registerVariable(Variable& variable);

		// Constants
		const Constant* getConstantByName(uint64 nameHash) const;
		void registerConstant(Constant& constant);

		// Constant arrays
		const ConstantArray* getConstantArrayByName(uint64 nameHash) const;
		void registerConstantArray(ConstantArray& constantArray);

		// Defines
		const Define* getDefineByName(uint64 nameHash) const;
		void registerDefine(Define& define);

		// String literals
		const StoredString* getStringLiteralByHash(uint64 hash) const;

	private:
		// Functions
		std::unordered_map<uint64, std::vector<Function*>> mFunctionsByName;	// Key is the hashed function name
		uint32 mNextFunctionId = 0;

		// Variables
		std::unordered_map<uint64, Variable*> mGlobalVariablesByName;
		uint32 mNextVariableId = 0;

		// Constants
		std::unordered_map<uint64, Constant*> mConstantsByName;

		// Constant arrays
		std::unordered_map<uint64, ConstantArray*> mConstantArraysByName;

		// Defines
		std::unordered_map<uint64, Define*> mDefinesByName;

		// String literals
		StringLookup mStringLiterals;
	};

}
