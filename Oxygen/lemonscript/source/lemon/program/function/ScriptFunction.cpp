/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/function/ScriptFunction.h"
#include "lemon/program/Module.h"
#include "lemon/compiler/Utility.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/utility/PragmaSplitter.h"
#include "lemon/utility/QuickDataHasher.h"


namespace lemon
{

	ScriptFunction::~ScriptFunction()
	{
		for (LocalVariable* variable : mLocalVariablesByID)
		{
			mModule->destroyLocalVariable(*variable);
		}
	}

	LocalVariable* ScriptFunction::getLocalVariableByIdentifier(uint64 nameHash) const
	{
		const auto it = mLocalVariablesByIdentifier.find(nameHash);
		return (it == mLocalVariablesByIdentifier.end()) ? nullptr : it->second;
	}

	LocalVariable& ScriptFunction::getLocalVariableByID(uint32 id) const
	{
		return *mLocalVariablesByID[id];
	}

	LocalVariable& ScriptFunction::addLocalVariable(FlyweightString name, const DataTypeDefinition* dataType, uint32 lineNumber)
	{
		// Check if it already exists!
		if (mLocalVariablesByIdentifier.count(name.getHash()))
		{
			CHECK_ERROR(false, "Variable already exists", lineNumber);
		}

		LocalVariable& variable = mModule->createLocalVariable();
		variable.mName = name;
		variable.mDataType = dataType;

		// Set local memory offset and size
		const size_t variableSize = (dataType->getBytes() + 7) / 8 * 8;		// Align to multiples of 8 bytes (i.e. int64 size)
		variable.mLocalMemoryOffset = mLocalVariablesMemorySize;
		variable.mLocalMemorySize = variableSize;
		mLocalVariablesMemorySize += variableSize;

		// Register variable
		mLocalVariablesByIdentifier.emplace(name.getHash(), &variable);

		variable.mID = (uint32)mLocalVariablesByID.size();
		mLocalVariablesByID.emplace_back(&variable);

		return variable;
	}

	const ScriptFunction::Label* ScriptFunction::findLabelByName(FlyweightString labelName) const
	{
		for (const Label& label : mLabels)
		{
			if (label.mName == labelName)
			{
				return &label;
			}
		}
		return nullptr;
	}

	const ScriptFunction::Label* ScriptFunction::findLabelByOffset(size_t offset) const
	{
		// Note that this won't handle multiple labels at the same position too well
		for (const Label& label : mLabels)
		{
			if (label.mOffset == offset)
			{
				return &label;
			}
		}
		return nullptr;
	}

	void ScriptFunction::addLabel(FlyweightString labelName, size_t offset, const std::vector<AddressHook>& addressHooks)
	{
		Label& label = vectorAdd(mLabels);
		label.mName = labelName;
		label.mOffset = (uint32)offset;
		label.mLabelAddressHooks = addressHooks;
	}

	void ScriptFunction::addOrProcessPragma(std::string_view pragmaString, bool consumeIfProcessed)
	{
		PragmaSplitter pragmaSplitter(pragmaString);
		if (!pragmaSplitter.mEntries.empty())
		{
			ScriptFunction::AddressHook* lastAddressHook = nullptr;
			bool hadAddressHook = false;
			bool hadAlias = false;
			for (const lemon::PragmaSplitter::Entry& entry : pragmaSplitter.mEntries)
			{
				if (entry.mArgument == "alias")
				{
					// Add alias for this function
					vectorAdd(mAliasNames).mName = entry.mValue;
					hadAlias = true;
				}
				else if (entry.mArgument == "address-hook")
				{
					// Create address hook
					RMX_CHECK(!entry.mValue.empty(), "Address hook must have a value", continue);
					ScriptFunction::AddressHook& addressHook = vectorAdd(mAddressHooks);
					addressHook.mAddress = (uint32)rmx::parseInteger(entry.mValue);
					lastAddressHook = &addressHook;
					hadAddressHook = true;
				}
				else if (entry.mArgument == "translated")
				{
					// You can use "translated" to denote that some code was already put into script, but should not be an actual address hook
					hadAddressHook = true;
				}
				else if (entry.mArgument == "off")
				{
					// Mark address hook as disabled
					if (nullptr != lastAddressHook)
					{
						lastAddressHook->mDisabled = true;
					}
				}
				else if (entry.mArgument == "deprecated")
				{
					if (hadAlias)
					{
						// Mark alias as deprecated
						mAliasNames.back().mIsDeprecated = true;
					}
					else
					{
						// Mark function itself as deprecated
						mFlags.set(Flag::DEPRECATED);
					}
				}
			}

			// If there was an address hook, there's no need to store this pragma
			if (consumeIfProcessed && hadAddressHook)
				return;
		}

		// Store this pragma as string
		mPragmas.emplace_back(pragmaString);
	}

	uint64 ScriptFunction::addToCompiledHash(uint64 hash) const
	{
		QuickDataHasher dataHasher(hash);
		for (const Opcode& opcode : mOpcodes)
		{
			dataHasher.prepareNextData(10);
			dataHasher.addData((uint8)opcode.mType);
			dataHasher.addData((uint8)opcode.mDataType);
			if (opcode.mParameter != 0)
				dataHasher.addData((uint64)opcode.mParameter);
		}
		return dataHasher.getHash();
	}

}
