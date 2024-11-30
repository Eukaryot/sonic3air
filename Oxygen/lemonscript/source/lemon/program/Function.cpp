/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/Function.h"
#include "lemon/program/Module.h"
#include "lemon/compiler/Utility.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/utility/PragmaSplitter.h"
#include "lemon/utility/QuickDataHasher.h"


namespace lemon
{
	namespace detail
	{
		uint32 getVoidSignatureHash()
		{
			uint32 value = PredefinedDataTypes::VOID.getDataTypeHash();
			return rmx::getFNV1a_32((const uint8*)&value, sizeof(uint32));
		}
	}


	void Function::SignatureBuilder::clear(const DataTypeDefinition& returnType)
	{
		mData.clear();
		mData.push_back(returnType.getDataTypeHash());
	}

	void Function::SignatureBuilder::addParameterType(const DataTypeDefinition& dataType)
	{
		mData.push_back(dataType.getDataTypeHash());
	}

	uint32 Function::SignatureBuilder::getSignatureHash()
	{
		uint32 hash = rmx::getFNV1a_32((const uint8*)&mData[0], mData.size() * sizeof(uint32));
		while (hash == 0)		// That should be a really rare case anyway
		{
			mData.push_back(0xcd000000);		// Just add anything to get away from hash 0
			hash = rmx::getFNV1a_32((const uint8*)&mData[0], mData.size() * sizeof(uint32));
		}
		return hash;
	}


	void Function::setParametersByTypes(const std::vector<const DataTypeDefinition*>& parameterTypes)
	{
		mParameters.clear();
		mParameters.resize(parameterTypes.size());
		for (size_t i = 0; i < parameterTypes.size(); ++i)
		{
			mParameters[i].mDataType = parameterTypes[i];
		}
		mSignatureHash = 0;
	}

	uint32 Function::getVoidSignatureHash()
	{
		static const uint32 signatureHash = detail::getVoidSignatureHash();
		return signatureHash;
	}

	uint32 Function::getSignatureHash() const
	{
		if (mSignatureHash == 0)
		{
			static SignatureBuilder builder;
			builder.clear(*mReturnType);
			for (const Parameter& parameter : mParameters)
				builder.addParameterType(*parameter.mDataType);
			mSignatureHash = builder.getSignatureHash();
		}
		return mSignatureHash;
	}


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

		mLocalVariablesByIdentifier.emplace(name.getHash(), &variable);

		variable.mID = (uint32)mLocalVariablesByID.size();
		mLocalVariablesByID.emplace_back(&variable);

		return variable;
	}

	bool ScriptFunction::getLabel(FlyweightString labelName, size_t& outOffset) const
	{
		for (const Label& label : mLabels)
		{
			if (label.mName == labelName)
			{
				outOffset = (size_t)label.mOffset;
				return true;
			}
		}
		return false;
	}

	void ScriptFunction::addLabel(FlyweightString labelName, size_t offset)
	{
		Label& label = vectorAdd(mLabels);
		label.mName = labelName;
		label.mOffset = (uint32)offset;
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

	void ScriptFunction::addOrProcessPragma(std::string_view pragmaString, bool consumeIfProcessed)
	{
		PragmaSplitter pragmaSplitter(pragmaString);
		if (!pragmaSplitter.mEntries.empty())
		{
			bool hadAddressHook = false;
			bool hadAlias = false;
			for (const lemon::PragmaSplitter::Entry& entry : pragmaSplitter.mEntries)
			{
				if (entry.mArgument == "alias")
				{
					vectorAdd(mAliasNames).mName = entry.mValue;
					hadAlias = true;
				}
				else if (entry.mArgument == "address-hook")
				{
					// Create address hook
					RMX_CHECK(!entry.mValue.empty(), "Address hook must have a value", continue);
					const uint32 address = (uint32)rmx::parseInteger(entry.mValue);
					mAddressHooks.push_back(address);
					hadAddressHook = true;
				}
				else if (entry.mArgument == "translated")
				{
					// You can use "translated" to denote that some code was already put into script, but should not be an actual address hook
					hadAddressHook = true;
				}
				else if (entry.mArgument == "deprecated")
				{
					if (hadAlias)
					{
						mAliasNames.back().mIsDeprecated = true;
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


	void NativeFunction::setFunction(const FunctionWrapper& functionWrapper)
	{
		mFunctionWrapper = &functionWrapper;
		mReturnType = functionWrapper.getReturnType();
		setParametersByTypes(functionWrapper.getParameterTypes());
	}

	NativeFunction& NativeFunction::setParameterInfo(size_t index, const std::string& identifier)
	{
		RMX_ASSERT(index < mParameters.size(), "Invalid parameter index " << index);
		RMX_ASSERT(!mParameters[index].mName.isValid(), "Parameter identifier is already set for index " << index);
		mParameters[index].mName.set(identifier);
		return *this;
	}

	void NativeFunction::execute(const Context context) const
	{
		RuntimeDetailHandler* runtimeDetailHandler = context.mControlFlow.getRuntime().getRuntimeDetailHandler();
		if (nullptr != runtimeDetailHandler)
		{
			runtimeDetailHandler->preExecuteExternalFunction(*this, context.mControlFlow);
			mFunctionWrapper->execute(context);
			runtimeDetailHandler->postExecuteExternalFunction(*this, context.mControlFlow);
		}
		else
		{
			mFunctionWrapper->execute(context);
		}
	}

}
