/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/translator/Nativizer.h"
#include "lemon/translator/NativizerInternal.h"
#include "lemon/translator/SourceCodeWriter.h"
#include "lemon/program/Module.h"
#include "lemon/program/OpcodeHelper.h"
#include "lemon/runtime/OpcodeProcessor.h"


namespace lemon
{
	namespace
	{

		void writeBinaryBlob(CppWriter& writer, const std::string& identifier, const uint8* data, size_t length)
		{
			// Output as a string literal, as this seems to be much faster to build for MSVC
			writer.writeLine("const char " + identifier + "[] =");
			writer.beginBlock();
			size_t count = 0;
			String line = "\"";
			for (size_t i = 0; i < length; ++i)
			{
				if (count > 0)
				{
					if ((count % 128) == 0)
					{
						line << "\"";
						writer.writeLine(line);
						line = "\"";
					}
				}
				line << rmx::hexString(data[i], 2, "\\x");
				++count;
			}
			if (!line.empty())
			{
				line << "\"";
				writer.writeLine(line);
			}
			writer.endBlock("};");
		}
	}



	void Nativizer::LookupDictionary::addEmptyEntries(const uint64* hashes, size_t numHashes)
	{
		mEntries.reserve(mEntries.size() + numHashes);
		for (size_t i = 0; i < numHashes; ++i)
		{
			mEntries.emplace(hashes[i], LookupEntry());
		}
	}

	void Nativizer::LookupDictionary::loadFunctions(const CompactFunctionEntry* entries, size_t numEntries)
	{
		mEntries.reserve(mEntries.size() + numEntries);
		for (size_t i = 0; i < numEntries; ++i)
		{
			mEntries.emplace(entries[i].mHash, LookupEntry(entries[i].mFunctionPointer, entries[i].mParameterStart));
		}
	}

	void Nativizer::LookupDictionary::loadParameterInfo(const uint8* data, size_t count)
	{
		std::vector<uint8> decompressedData;
		ZlibDeflate::decode(decompressedData, data, count);
		mParameterData.resize(decompressedData.size() / 4);
		data = &decompressedData[0];
		for (LookupEntry::ParameterInfo& info : mParameterData)
		{
			info.mOffset = *(uint16*)&data[0];
			info.mOpcodeIndex = data[2];
			info.mSemantics = (LookupEntry::ParameterInfo::Semantics)data[3];
			data += 4;
		}
	}


	void Nativizer::getOpcodeSubtypeInfo(OpcodeSubtypeInfo& outInfo, const Opcode* opcodes, size_t numOpcodesAvailable, MemoryAccessHandler& memoryAccessHandler)
	{
		const Opcode& firstOpcode = opcodes[0];

		// Setup some defaults
		//  -> Note that "outInfo.mFirstOpcodeIndex" does not get touched, as this function can't know about the correct value
		outInfo.mConsumedOpcodes = 1;
		outInfo.mType = firstOpcode.mType;
		outInfo.mSpecialType = OpcodeSubtypeInfo::SpecialType::NONE;
		outInfo.mSubtypeData = 0;

		// First check for applying special types
		if (numOpcodesAvailable >= 2 && firstOpcode.mType == Opcode::Type::PUSH_CONSTANT && opcodes[1].mType == Opcode::Type::READ_MEMORY)
		{
			const Opcode& readMemoryOpcode = opcodes[1];
			const bool consumeInput = (readMemoryOpcode.mParameter == 0);
			const uint64 address = firstOpcode.mParameter;
			const BaseType baseType = readMemoryOpcode.mDataType;

			MemoryAccessHandler::SpecializationResult result;
			memoryAccessHandler.getDirectAccessSpecialization(result, address, DataTypeHelper::getSizeOfBaseType(baseType), false);
			if (result.mResult == MemoryAccessHandler::SpecializationResult::HAS_SPECIALIZATION)
			{
				outInfo.mConsumedOpcodes = 2;
				outInfo.mType = Opcode::Type::NOP;
				outInfo.mSpecialType = OpcodeSubtypeInfo::SpecialType::FIXED_MEMORY_READ;
				outInfo.mSubtypeData |= ((uint32)baseType) << 16;		// Data type, including signed/unsigned
				if (result.mSwapBytes)
					outInfo.mSubtypeData |= 0x0001;						// Flag to signal that byte swap is needed
				if (!consumeInput)
					outInfo.mSubtypeData |= 0x0002;						// Flag to signal that input is *not* consumed
				return;
			}
		}

		// The subtype determined here is supposed to include only the information that actually needs to be considered for nativization
		//  -> If both the opcode type and the subtype is identical for two opcodes, their nativized version is the same
		switch (firstOpcode.mType)
		{
			case Opcode::Type::PUSH_CONSTANT:
			{
				outInfo.mSubtypeData |= ((uint32)firstOpcode.mDataType) << 16;				// Data type, including signed/unsigned
			#if 1
				// Just as an experiment: Use specialized nativization for constants, at least in a small range of common constant values
				//  -> Unfortunately, this has no real positive effect on performance that would be worth the extra cost
				if (firstOpcode.mParameter >= 0 && firstOpcode.mParameter <= 1)
				{
					outInfo.mSubtypeData |= 0x8000;				// Mark as specialized subtype for constants
					outInfo.mSubtypeData |= firstOpcode.mParameter;
				}
			#endif
				break;
			}

			case Opcode::Type::GET_VARIABLE_VALUE:
			case Opcode::Type::SET_VARIABLE_VALUE:
				outInfo.mSubtypeData |= ((uint32)BaseTypeHelper::makeIntegerUnsigned(firstOpcode.mDataType)) << 16;		// Data type, ignoring signed/unsigned
				outInfo.mSubtypeData |= (firstOpcode.mParameter >> 28);													// Variable type
				break;

			case Opcode::Type::READ_MEMORY:
				outInfo.mSubtypeData |= ((uint32)BaseTypeHelper::makeIntegerUnsigned(firstOpcode.mDataType)) << 16;		// Data type, ignoring signed/unsigned
				outInfo.mSubtypeData |= firstOpcode.mParameter;															// Flag for variant that does not consume its input
				break;

			case Opcode::Type::WRITE_MEMORY:
				outInfo.mSubtypeData |= ((uint32)BaseTypeHelper::makeIntegerUnsigned(firstOpcode.mDataType)) << 16;		// Data type, ignoring signed/unsigned
				outInfo.mSubtypeData |= firstOpcode.mParameter;															// Flag for exchanged inputs variant
				break;

			case Opcode::Type::CAST_VALUE:
				outInfo.mSubtypeData |= (uint32)OpcodeHelper::getCastSourceType(firstOpcode) << 16;		// Type of cast
				outInfo.mSubtypeData |= (uint32)OpcodeHelper::getCastTargetType(firstOpcode);			// Type of cast
				break;

			case Opcode::Type::ARITHM_ADD:
			case Opcode::Type::ARITHM_SUB:
			case Opcode::Type::ARITHM_AND:
			case Opcode::Type::ARITHM_OR:
			case Opcode::Type::ARITHM_XOR:
			case Opcode::Type::ARITHM_SHL:
			case Opcode::Type::ARITHM_NEG:
			case Opcode::Type::ARITHM_NOT:
			case Opcode::Type::ARITHM_BITNOT:
			case Opcode::Type::COMPARE_EQ:
			case Opcode::Type::COMPARE_NEQ:
				outInfo.mSubtypeData |= ((uint32)BaseTypeHelper::makeIntegerUnsigned(firstOpcode.mDataType)) << 16;		// Data type, ignoring signed/unsigned
				break;

			case Opcode::Type::ARITHM_MUL:
			case Opcode::Type::ARITHM_DIV:
			case Opcode::Type::ARITHM_MOD:
			case Opcode::Type::ARITHM_SHR:
			case Opcode::Type::COMPARE_LT:
			case Opcode::Type::COMPARE_LE:
			case Opcode::Type::COMPARE_GT:
			case Opcode::Type::COMPARE_GE:
				outInfo.mSubtypeData |= ((uint32)firstOpcode.mDataType) << 16;			// Data type, including signed/unsigned
				break;

			// All others have only one subtype
			default:
				break;
		}
	}

	uint64 Nativizer::getStartHash()
	{
		// Still using FNV1a here instead of Murmur2, as it allows for building the hash incrementally
		return rmx::startFNV1a_64();
	}

	uint64 Nativizer::addOpcodeSubtypeInfoToHash(uint64 hash, const OpcodeSubtypeInfo& info)
	{
		const uint8* typePointer = (info.mSpecialType == OpcodeSubtypeInfo::SpecialType::NONE) ? (uint8*)&info.mType : (uint8*)&info.mSpecialType;
		hash = rmx::addToFNV1a_64(hash, typePointer, 1);
		hash = rmx::addToFNV1a_64(hash, (uint8*)&info.mSubtypeData, 4);
		return hash;
	}

	void Nativizer::build(String& output, const Module& module, const Program& program, MemoryAccessHandler& memoryAccessHandler)
	{
		mModule = &module;
		mProgram = &program;
		mMemoryAccessHandler = &memoryAccessHandler;

		// Setup the output dictionary, with a first parameter data entry that will be pointed at by functions without parameters
		{
			mBuiltDictionary.mEntries.clear();
			mBuiltDictionary.mParameterData.resize(1);
			mBuiltDictionary.mParameterData[0].mOffset = 0;
			mBuiltDictionary.mParameterData[0].mOpcodeIndex = 0xff;
			mBuiltDictionary.mParameterData[0].mSemantics = (LookupEntry::ParameterInfo::Semantics)0xff;
		}

		// Start writing
		CppWriter writer(output);
		writer.writeLine("#define NATIVIZED_CODE_AVAILABLE");
		writer.writeEmptyLine();

		// Go through all compiled opcodes
		for (const ScriptFunction* func : module.getScriptFunctions())
		{
			buildFunction(writer, *func);
		}

		// Write reflection lookup
		if (!mBuiltDictionary.mEntries.empty())
		{
			writer.writeEmptyLine();
			writer.writeLine("void createNativizedCodeLookup(Nativizer::LookupDictionary& dict)");
			writer.beginBlock();
			{
				std::vector<uint64> entriesToWrite;
				entriesToWrite.reserve(mBuiltDictionary.mEntries.size());	// Certainly overestimated, but who cares
				for (const std::pair<uint64, LookupEntry>& pair : mBuiltDictionary.mEntries)
				{
					if (nullptr == pair.second.mExecFunc)
					{
						entriesToWrite.push_back(pair.first);
					}
				}
				const uint8* data = (const uint8*)&entriesToWrite[0];
				const size_t bytes = entriesToWrite.size() * 8;
				const size_t chunks = (bytes + 0x7fff) / 0x8000;
				for (size_t i = 0; i < chunks; ++i)
				{
					const std::string identifier = "emptyEntries" + std::to_string(i);
					const size_t restBytes = std::min<size_t>(bytes - i * 0x8000, 0x8000);
					writeBinaryBlob(writer, identifier, &data[i * 0x8000], restBytes);
					writer.writeLine("dict.addEmptyEntries(reinterpret_cast<const uint64*>(" + identifier + "), " + rmx::hexString(restBytes / 8, 2) + ");");
					writer.writeEmptyLine();
				}
			}

			// Collect the data to actually write
			std::vector<std::pair<uint64, size_t>> functionList;	// First = hash of the generated function, second = start index of function's parameter data
			std::vector<uint8> parameterData;
			{
				functionList.reserve(mBuiltDictionary.mEntries.size());
				for (const std::pair<uint64, LookupEntry>& pair : mBuiltDictionary.mEntries)
				{
					const LookupEntry& lookupEntry = pair.second;
					if (nullptr != lookupEntry.mExecFunc)
					{
						functionList.emplace_back(pair.first, pair.second.mParameterStart);
					}
				}

				parameterData.resize(mBuiltDictionary.mParameterData.size() * 4);
				uint8* outPtr = &parameterData[0];
				for (const LookupEntry::ParameterInfo& parameterInfo : mBuiltDictionary.mParameterData)
				{
					*(uint16*)(&outPtr[0]) = (uint16)parameterInfo.mOffset;
					outPtr[2] = (uint8)parameterInfo.mOpcodeIndex;
					outPtr[3] = (uint8)parameterInfo.mSemantics;
					outPtr += 4;
				}
			}

			// Now write all that data
			//  -> This gets output as a string (and also using compression), as that proved to allow for MUCH faster compilation by the Microsoft compiler for some reason
			{
				std::vector<uint8> compressedData;
				ZlibDeflate::encode(compressedData, &parameterData[0], parameterData.size(), 9);
				writeBinaryBlob(writer, "parameterData", &compressedData[0], compressedData.size());
				writer.writeLine("dict.loadParameterInfo(reinterpret_cast<const uint8*>(parameterData), " + rmx::hexString(compressedData.size(), 4) + ");");
				writer.writeEmptyLine();
			}
			{
				writer.writeLine("const Nativizer::CompactFunctionEntry functionList[] =");
				writer.beginBlock();
				for (size_t k = 0; k < functionList.size(); ++k)
				{
					const auto& pair = functionList[k];
					const std::string hashString = rmx::hexString(pair.first, 16, "");
					writer.writeLine("{ 0x" + hashString + ", &exec_" + hashString + ", " + rmx::hexString(pair.second, 8) + " }" + (k+1 < functionList.size() ? "," : ""));
				}
				writer.endBlock("};");
				writer.writeLine("dict.loadFunctions(functionList, " + rmx::hexString(functionList.size(), 4) + ");");
			}
			writer.endBlock();
		}
	}

	void Nativizer::buildFunction(CppWriter& writer, const ScriptFunction& function)
	{
		static std::vector<OpcodeProcessor::OpcodeData> opcodeData;
		OpcodeProcessor::buildOpcodeData(opcodeData, function);

		const size_t numOpcodes = function.mOpcodes.size();
		for (size_t i = 0; i < numOpcodes; )
		{
			if (opcodeData[i].mRemainingSequenceLength > 0)
			{
				const size_t numOpcodesConsumed = processOpcodes(writer, &function.mOpcodes[i], opcodeData[i].mRemainingSequenceLength, function);
				i += numOpcodesConsumed;
			}
			else
			{
				++i;
			}
		}
	}

	size_t Nativizer::processOpcodes(CppWriter& writer, const Opcode* opcodes, size_t numOpcodes, const ScriptFunction& function)
	{
		// Check how many opcodes cam be nativized into a single function
		numOpcodes = collectNativizableOpcodes(opcodes, numOpcodes);
		if (numOpcodes < MIN_OPCODES)
		{
			// Nativization is not worth it for so few opcodes, so just skip them
			return numOpcodes + 1;
		}

		static NativizerInternal nativizerInternal;
		nativizerInternal.reset();

		// Get subtype infos from the opcodes, and build hashes
		const uint64 hash = nativizerInternal.buildSubtypeInfos(opcodes, numOpcodes, *mMemoryAccessHandler);
		if (mBuiltDictionary.mEntries.count(hash) != 0)
			return numOpcodes;

		nativizerInternal.buildAssignmentsFromOpcodes(opcodes);

		// Postprocessing
		nativizerInternal.performPostProcessing();

		// Generate code for the assignments
		nativizerInternal.generateCppCode(writer, function, opcodes[0], hash);

		// Register
		for (size_t index = 1; index < nativizerInternal.mOpcodeInfos.size() - 1; ++index)
		{
			const uint64 partialHash = nativizerInternal.mOpcodeInfos[index].mHash;
			mBuiltDictionary.mEntries[partialHash];		// Creates the entry with initial value, or leaves it like it is if it was already existing
		}

		{
			LookupEntry& entry = mBuiltDictionary.mEntries[hash];
			entry.mExecFunc = (ExecFunc)1;	// Treating this as a bool, we only care if it's a nullptr or not

			std::vector<NativizerInternal::ParameterInfo>& params = nativizerInternal.mParameters.mParameters;
			if (params.empty())
			{
				// Refer to the dummy entry for functions without parameters
				entry.mParameterStart = 0;
			}
			else
			{
				const size_t startIndex = mBuiltDictionary.mParameterData.size();
				entry.mParameterStart = startIndex;
				mBuiltDictionary.mParameterData.resize(startIndex + params.size() + 1);

				LookupEntry::ParameterInfo* parameterPtr = &mBuiltDictionary.mParameterData[startIndex];
				for (size_t k = 0; k < params.size(); ++k)
				{
					parameterPtr->mOffset = (uint16)params[k].mOffset;
					parameterPtr->mOpcodeIndex = (uint8)params[k].mOpcodeIndex;
					parameterPtr->mSemantics = params[k].mSemantics;
					++parameterPtr;
				}

				// Add a terminating entry as well
				parameterPtr->mOffset = (uint16)nativizerInternal.mParameters.mTotalSize;
				parameterPtr->mOpcodeIndex = 0xff;
				parameterPtr->mSemantics = (LookupEntry::ParameterInfo::Semantics)0xff;
			}
		}

		return numOpcodes;
	}

	size_t Nativizer::collectNativizableOpcodes(const Opcode* opcodes, size_t numOpcodes)
	{
		if (numOpcodes > MAX_OPCODES)
			numOpcodes = MAX_OPCODES;

		for (size_t index = 0; index < numOpcodes; ++index)
		{
			const Opcode::Type opcodeType = opcodes[index].mType;
			RMX_ASSERT(opcodeType != Opcode::Type::MAKE_BOOL, "MAKE_BOOL should not occur any more");

			const bool isSupported = (opcodeType == Opcode::Type::MOVE_STACK ||
									  (opcodeType >= Opcode::Type::PUSH_CONSTANT && opcodeType <= Opcode::Type::COMPARE_GE));
			if (!isSupported)
			{
				// Stop here
				return index;
			}
		}
		return numOpcodes;
	}

}
