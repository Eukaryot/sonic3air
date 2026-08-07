/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/Program.h"
#include "lemon/program/Module.h"
#include "lemon/translator/Nativizer.h"


namespace lemon
{
	namespace
	{
		static const std::vector<Function*> EMPTY_FUNCTIONS;
	}

	Program::Program()
	{
		PredefinedDataTypes::collectPredefinedDataTypes(mDataTypes);
	}

	Program::~Program()
	{
		clearInternal();
	}

	void Program::clear()
	{
		clearInternal();
		PredefinedDataTypes::collectPredefinedDataTypes(mDataTypes);
	}

	void Program::addModule(const Module& module)
	{
		for (const Module* existingModule : mModules)
		{
			if (existingModule->getModuleId() == module.getModuleId())
			{
				RMX_ERROR("Two modules are using the same module name '" << module.getModuleName() << "'", );
				return;
			}
		}

		mModules.push_back(&module);

		// Functions
		mFunctions.reserve(mFunctions.size() + module.mFunctions.size());
		mScriptFunctions.reserve(mScriptFunctions.size() + module.mFunctions.size());	// This is possibly an overestimation, but that's okay
		for (Function* function : module.mFunctions)
		{
			RMX_ASSERT(mFunctions.size() == function->getID(), "Mismatch between expected (" << mFunctions.size() << ") and actual function ID (" << function->getID() << ")");
			mFunctions.push_back(function);
			if (function->isA<ScriptFunction>())
			{
				mScriptFunctions.push_back(&function->as<ScriptFunction>());
			}

			mFunctionsByName[function->getName().getHash()].push_back(function);
			std::vector<Function*>& funcs = mFunctionsBySignature[function->getNameAndSignatureHash()];
			funcs.insert(funcs.begin(), function);		// Insert as first
		}

		// Callable function addresses
		for (const auto& [address, nameHash] : module.mCallableFunctions)
		{
			const Function* function = getFunctionBySignature(nameHash + Function::getVoidSignatureHash());
			if (nullptr != function)
				mCallableFunctionsByAddress[address] = function;
			else
				RMX_ERROR("Could not resolve callable function by name hash " << rmx::hexString(nameHash, 16), );
		}

		// Global variables
		mGlobalVariables.reserve(mGlobalVariables.size() + module.mGlobalVariables.size());
		for (Variable* variable : module.mGlobalVariables)
		{
			RMX_ASSERT(mGlobalVariables.size() == (variable->getID() & 0x0fffffff), "Mismatch between expected and actual variable ID");
			mGlobalVariables.push_back(variable);
			mGlobalVariablesByName[variable->getName().getHash()] = variable;
		}

		// Constant arrays
		mConstantArrays.reserve(mConstantArrays.size() + module.mConstantArrays.size());
		for (ConstantArray* constantArray : module.mConstantArrays)
		{
			mConstantArrays.push_back(constantArray);
		}

		// Defines
		mDefines.reserve(mDefines.size() + module.mDefines.size());
		for (Define* define : module.mDefines)
		{
			mDefines.push_back(define);
		}

		// Data types
		mDataTypes.reserve(mDataTypes.size() + module.mDataTypes.size());
		for (const DataTypeDefinition* dataTypeDefinition : module.mDataTypes)
		{
			mDataTypes.push_back(dataTypeDefinition);
		}
	}

	void Program::runNativization(const Module& module, const std::wstring& outputFilename, MemoryAccessHandler& memoryAccessHandler)
	{
		String output;
		Nativizer().build(output, module, *this, memoryAccessHandler);

		// Check if this is an actual change in the output file
		{
			String original;
			if (original.loadFile(outputFilename))
			{
				if (output == original)
					return;
			}
		}

		output.saveFile(outputFilename);
	}

	const Function* Program::getFunctionByID(uint32 id) const
	{
		return mFunctions[id];
	}

	const Function* Program::getFunctionBySignature(uint64 nameAndSignatureHash, size_t index) const
	{
		const auto it = mFunctionsBySignature.find(nameAndSignatureHash);
		if (it == mFunctionsBySignature.end())
			return nullptr;

		const std::vector<Function*>& functions = it->second;
		return (index < functions.size()) ? functions[index] : nullptr;
	}

	const std::vector<Function*>& Program::getFunctionsByName(uint64 nameHash) const
	{
		const auto it = mFunctionsByName.find(nameHash);
		return (it == mFunctionsByName.end()) ? EMPTY_FUNCTIONS : it->second;
	}

	const Function* Program::resolveCallableFunctionAddress(uint32 address) const
	{
		const auto it = mCallableFunctionsByAddress.find(address);
		return (it == mCallableFunctionsByAddress.end()) ? nullptr : it->second;
	}

	Variable& Program::getGlobalVariableByID(uint32 id) const
	{
		return *mGlobalVariables[id & 0x0fffffff];
	}

	Variable* Program::getGlobalVariableByName(uint64 nameHash) const
	{
		const auto it = mGlobalVariablesByName.find(nameHash);
		return (it == mGlobalVariablesByName.end()) ? nullptr : it->second;
	}

	void Program::collectAllStringLiterals(StringLookup& outStrings) const
	{
		for (const Module* module : mModules)
		{
			outStrings.addFromList(module->getStringLiterals());
		}
	}

	const DataTypeDefinition* Program::getDataTypeByID(uint16 dataTypeID) const
	{
		return (dataTypeID < mDataTypes.size()) ? mDataTypes[dataTypeID] : nullptr;
	}

	void Program::clearInternal()
	{
		// This is only meant to clear the module list, while the modules themselves stay intact
		mModules.clear();

		// Functions
		mFunctions.clear();
		mScriptFunctions.clear();
		mFunctionsBySignature.clear();
		mFunctionsByName.clear();

		// Variables
		mGlobalVariables.clear();
		mGlobalVariablesByName.clear();

		// Constant arrays
		mConstantArrays.clear();

		// Defines
		mDefines.clear();

		// Data types
		mDataTypes.clear();
	}

}
