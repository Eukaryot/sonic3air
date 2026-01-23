/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/program/Module.h"


namespace lemon
{
	namespace
	{
		const size_t NUM_PREDEFINED_DATA_TYPES = 14;
	}


	GlobalsLookup::GlobalsLookup()
	{
		// Add predefined data types
		PredefinedDataTypes::collectPredefinedDataTypes(mDataTypes);
		RMX_ASSERT(mDataTypes.size() == NUM_PREDEFINED_DATA_TYPES, "Wrong number of predefined data types");
	}

	void GlobalsLookup::clear()
	{
		mAllIdentifiers.clear();
		mFunctionsByName.clear();

		mNextFunctionID = 0;
		mNextVariableID = 0;
		mNextConstantArrayID = 0;

		// Clear all data types except for the predefined ones
		mDataTypes.resize(NUM_PREDEFINED_DATA_TYPES);
	}

	void GlobalsLookup::addDefinitionsFromModule(const Module& module)
	{
		for (Constant* constant : module.mPreprocessorDefinitions)
		{
			mPreprocessorDefinitions.setDefinition(constant->getName(), constant->getValue().get<int64>());
			registerConstant(*constant);	// Also add as a normal constant to be able to access them outside of preprocessor directives as well
		}
		for (Function* function : module.mFunctions)
		{
			registerFunction(*function);
		}
		for (Variable* variable : module.mGlobalVariables)
		{
			registerGlobalVariable(*variable);
		}
		for (Constant* constant : module.mConstants)
		{
			registerConstant(*constant);
		}
		for (size_t i = 0; i < module.mNumGlobalConstantArrays; ++i)
		{
			registerConstantArray(*module.mConstantArrays[i]);
		}
		for (Define* define : module.mDefines)
		{
			registerDefine(*define);
		}
		for (const DataTypeDefinition* dataType : module.mDataTypes)
		{
			registerDataType(dataType);
		}

		RMX_ASSERT(mNextFunctionID == module.mFirstFunctionID, "Mismatch in function ID when adding module '" << module.getModuleName() << "' (" << mNextFunctionID << " vs. " << module.mFirstFunctionID << ")");
		RMX_ASSERT(mNextVariableID == module.mFirstVariableID, "Mismatch in variable ID when adding module '" << module.getModuleName() << "' (" << mNextVariableID << " vs. " << module.mFirstVariableID << ")");
		RMX_ASSERT(mNextConstantArrayID == module.mFirstConstantArrayID, "Mismatch in constant array ID when adding module '" << module.getModuleName() << "' (" << mNextConstantArrayID << " vs. " << module.mFirstConstantArrayID << ")");
		mNextFunctionID += (uint32)module.mFunctions.size();
		mNextVariableID += (uint32)module.mGlobalVariables.size();
		mNextConstantArrayID += (uint32)module.mConstantArrays.size();
	}

	const GlobalsLookup::Identifier* GlobalsLookup::resolveIdentifierByHash(uint64 nameHash) const
	{
		return mapFind(mAllIdentifiers, nameHash);
	}

	const std::vector<GlobalsLookup::FunctionReference>& GlobalsLookup::getFunctionsByName(uint64 nameHash) const
	{
		static const std::vector<FunctionReference> EMPTY_FUNCTIONS;
		const auto it = mFunctionsByName.find(nameHash);
		return (it == mFunctionsByName.end()) ? EMPTY_FUNCTIONS : it->second;
	}

	const GlobalsLookup::FunctionReference* GlobalsLookup::getFunctionByNameAndSignature(uint64 nameHash, uint32 signatureHash, bool* outAnyFound) const
	{
		const std::vector<FunctionReference>& candidateFunctions = getFunctionsByName(nameHash);
		if (candidateFunctions.empty())
		{
			if (nullptr != outAnyFound)
				*outAnyFound = false;
		}
		else
		{
			if (nullptr != outAnyFound)
				*outAnyFound = true;

			for (const FunctionReference& func : candidateFunctions)
			{
				if (func.mFunction->getSignatureHash() == signatureHash)
					return &func;
			}
		}
		return nullptr;
	}

	const std::vector<GlobalsLookup::FunctionReference>& GlobalsLookup::getMethodsByName(uint64 contextNameHash) const
	{
		static const std::vector<FunctionReference> EMPTY_FUNCTIONS;
		const auto it = mMethodsByName.find(contextNameHash);
		return (it == mMethodsByName.end()) ? EMPTY_FUNCTIONS : it->second;
	}

	void GlobalsLookup::registerFunction(Function& function)
	{
		const uint64 nameHash = function.getName().getHash();
		if (function.getContext().isEmpty())
		{
			FunctionReference& ref = vectorAdd(mFunctionsByName[nameHash]);
			ref.mFunction = &function;
			ref.mIsDeprecated = function.getFlags().isSet(Function::Flag::DEPRECATED);

			for (const Function::AliasName& aliasName : function.getAliasNames())
			{
				FunctionReference& ref = vectorAdd(mFunctionsByName[aliasName.mName.getHash()]);
				ref.mFunction = &function;
				ref.mIsDeprecated = aliasName.mIsDeprecated;
			}
		}
		else
		{
			const uint64 contextHash = function.getContext().getHash();

			FunctionReference& ref = vectorAdd(mMethodsByName[contextHash + nameHash]);
			ref.mFunction = &function;
			ref.mIsDeprecated = function.getFlags().isSet(Function::Flag::DEPRECATED);

			for (const Function::AliasName& aliasName : function.getAliasNames())
			{
				FunctionReference& ref = vectorAdd(mMethodsByName[contextHash + aliasName.mName.getHash()]);
				ref.mFunction = &function;
				ref.mIsDeprecated = aliasName.mIsDeprecated;
			}
		}
	}

	void GlobalsLookup::registerGlobalVariable(Variable& variable)
	{
		const uint64 nameHash = variable.getName().getHash();
		mAllIdentifiers[nameHash].set(&variable);
	}

	void GlobalsLookup::registerConstant(Constant& constant)
	{
		const uint64 nameHash = constant.getName().getHash();
		mAllIdentifiers[nameHash].set(&constant);
	}

	void GlobalsLookup::registerConstantArray(ConstantArray& constantArray)
	{
		const uint64 nameHash = constantArray.getName().getHash();
		mAllIdentifiers[nameHash].set(&constantArray);
	}

	void GlobalsLookup::registerDefine(Define& define)
	{
		const uint64 nameHash = define.getName().getHash();
		mAllIdentifiers[nameHash].set(&define);

		// Invalidate the "resolved" pointer in identifiers, as they possible got invalid by now
		define.invalidateResolvedIdentifiers();
	}

	const FlyweightString* GlobalsLookup::getStringLiteralByHash(uint64 hash) const
	{
		return mStringLiterals.getStringByHash(hash);
	}

	const DataTypeDefinition* GlobalsLookup::findDataTypeByName(uint64 nameHash) const
	{
		// TODO: Optimize this by using a map
		for (const DataTypeDefinition* existingDataType : mDataTypes)
		{
			if (existingDataType->getName().getHash() == nameHash)
				return existingDataType;
		}
		return nullptr;
	}

	void GlobalsLookup::registerDataType(const DataTypeDefinition* dataTypeDefinition)
	{
		RMX_ASSERT(dataTypeDefinition->getID() == mDataTypes.size(), "Wrong data type ID");
		mDataTypes.push_back(dataTypeDefinition);
		mAllIdentifiers[dataTypeDefinition->getName().getHash()].set(dataTypeDefinition);
	}

	const DataTypeDefinition* GlobalsLookup::readDataType(VectorBinarySerializer& serializer) const
	{
		uint16 index = serializer.read<uint16>();
		if (index >= mDataTypes.size())
			index = 0;
		return mDataTypes[index];
	}

	void GlobalsLookup::serializeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition*& dataTypeDefinition) const
	{
		if (serializer.isReading())
		{
			dataTypeDefinition = readDataType(serializer);
		}
		else
		{
			serializer.write<uint16>((nullptr == dataTypeDefinition) ? 0 : dataTypeDefinition->getID());
		}
	}

}
