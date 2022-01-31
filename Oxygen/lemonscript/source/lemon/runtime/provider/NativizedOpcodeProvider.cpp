/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/provider/NativizedOpcodeProvider.h"
#include "lemon/runtime/RuntimeFunction.h"
#include "lemon/program/Program.h"
#include "lemon/program/Variable.h"


namespace lemon
{

	void NativizedOpcodeProvider::buildLookup(BuildFunction buildFunction)
	{
		mLookupDictionary.mEntries.clear();
		(*buildFunction)(mLookupDictionary);
	}

	bool NativizedOpcodeProvider::buildRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int& outNumOpcodesConsumed, const Runtime& runtime)
	{
		if (mLookupDictionary.mEntries.empty() || numOpcodesAvailable < (int)Nativizer::MIN_OPCODES)
			return false;

		Nativizer::LookupEntry* bestEntry = nullptr;
		uint64 bestEntryHash = 0;		// Variable exists only for debugging
		{
			uint64 hash = Nativizer::getStartHash();
			for (size_t index = 0; index < (size_t)numOpcodesAvailable; )
			{
				Nativizer::OpcodeSubtypeInfo info;
				Nativizer::getOpcodeSubtypeInfo(info, &opcodes[index], numOpcodesAvailable, *runtime.getMemoryAccessHandler());
				hash = Nativizer::addOpcodeSubtypeInfoToHash(hash, info);
				index += info.mConsumedOpcodes;

				if (index >= Nativizer::MIN_OPCODES)
				{
					const auto it = mLookupDictionary.mEntries.find(hash);
					if (it == mLookupDictionary.mEntries.end())
						return false;

					if (nullptr != it->second.mExecFunc)
					{
						bestEntry = &it->second;
						bestEntryHash = hash;
						outNumOpcodesConsumed = (int)index;
					}
				}
			}
		}

		if (nullptr != bestEntry)
		{
			// Find out number of parameters and total parameter size first
			const Nativizer::LookupEntry& entry = *bestEntry;
			int numParameters = 0;
			size_t parameterSize = 0;
			{
				const Nativizer::LookupEntry::ParameterInfo* parameterPtr = &mLookupDictionary.mParameterData[entry.mParameterStart];
				while (parameterPtr->mOpcodeIndex != 0xff)
				{
					++parameterPtr;
					++numParameters;
				}
				parameterSize = (size_t)parameterPtr->mOffset;
			}

			// Create runtime opcode
			RuntimeOpcode& runtimeOpcode = buffer.addOpcode(parameterSize);
			runtimeOpcode.mExecFunc = entry.mExecFunc;
			{
				// Go through all parameters (again), now adding them to the runtime opcode
				const Nativizer::LookupEntry::ParameterInfo* parameterPtr = &mLookupDictionary.mParameterData[entry.mParameterStart];
				for (int k = 0; k < numParameters; ++k)
				{
					const Nativizer::LookupEntry::ParameterInfo& parameter = parameterPtr[k];
					const Opcode& opcode = opcodes[parameter.mOpcodeIndex];
					const int64 value = opcode.mParameter;
					switch (parameter.mSemantics)
					{
						case Nativizer::LookupEntry::ParameterInfo::Semantics::INTEGER:
						{
							runtimeOpcode.setParameter<int64>(value, parameter.mOffset);
							break;
						}

						case Nativizer::LookupEntry::ParameterInfo::Semantics::GLOBAL_VARIABLE:
						{
							const uint32 variableId = (uint32)opcode.mParameter;
							int64* value = const_cast<Runtime&>(runtime).accessGlobalVariableValue(runtime.getProgram().getGlobalVariableByID(variableId));
							runtimeOpcode.setParameter(value, parameter.mOffset);
							break;
						}

						case Nativizer::LookupEntry::ParameterInfo::Semantics::EXTERNAL_VARIABLE:
						{
							const uint32 variableId = (uint32)opcode.mParameter;
							const ExternalVariable& variable = static_cast<ExternalVariable&>(runtime.getProgram().getGlobalVariableByID(variableId));
							runtimeOpcode.setParameter(variable.mPointer, parameter.mOffset);
							break;
						}

						case Nativizer::LookupEntry::ParameterInfo::Semantics::FIXED_MEMORY_ADDRESS:
						{
							// TODO: "opcode.mDataType" refers to the PUSH_CONSTANT opcode, so it actually does not tell us the correct data type; however, this shouldn't be much of a problem for now
							const uint64 address = opcode.mParameter;
							MemoryAccessHandler::SpecializationResult result;
							runtime.getMemoryAccessHandler()->getDirectAccessSpecialization(result, address, DataTypeHelper::getSizeOfBaseType(opcode.mDataType), false);	// No support for write access here
							RMX_ASSERT(result.mResult == MemoryAccessHandler::SpecializationResult::HAS_SPECIALIZATION, "No memory access specialization found even though this was previously checked");
							runtimeOpcode.setParameter(result.mDirectAccessPointer, parameter.mOffset);
							break;
						}
					}
				}
			}
			return true;
		}
		return false;
	}

}
