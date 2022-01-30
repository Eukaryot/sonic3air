/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/translator/Nativizer.h"
#include "lemon/translator/SourceCodeWriter.h"
#include "lemon/program/Function.h"
#include "lemon/program/Module.h"
#include "lemon/program/OpcodeHelper.h"
#include "lemon/program/Program.h"
#include "lemon/runtime/OpcodeProcessor.h"
#include "lemon/runtime/Runtime.h"


namespace lemon
{
	namespace
	{

		const std::string& getDataTypeString(BaseType dataType, bool ignoreSigned = false)
		{
			static const std::string TYPESTRING_uint8  = "uint8";
			static const std::string TYPESTRING_uint16 = "uint16";
			static const std::string TYPESTRING_uint32 = "uint32";
			static const std::string TYPESTRING_uint64 = "uint64";
			static const std::string TYPESTRING_int8   = "int8";
			static const std::string TYPESTRING_int16  = "int16";
			static const std::string TYPESTRING_int32  = "int32";
			static const std::string TYPESTRING_int64  = "int64";
			static const std::string TYPESTRING_empty  = "";

			if (ignoreSigned)
			{
				switch ((uint8)dataType & 0x03)
				{
					case 0x00:  return TYPESTRING_uint8;
					case 0x01:  return TYPESTRING_uint16;
					case 0x02:  return TYPESTRING_uint32;
					case 0x03:  return TYPESTRING_uint64;
				}
			}
			else
			{
				switch ((uint8)dataType & 0x0b)
				{
					case 0x00:  return TYPESTRING_uint8;
					case 0x01:  return TYPESTRING_uint16;
					case 0x02:  return TYPESTRING_uint32;
					case 0x03:  return TYPESTRING_uint64;
					case 0x08:  return TYPESTRING_int8;
					case 0x09:  return TYPESTRING_int16;
					case 0x0a:  return TYPESTRING_int32;
					case 0x0b:  return TYPESTRING_int64;
				}
			}
			return TYPESTRING_empty;
		}

		size_t getDataTypeBits(BaseType dataType)
		{
			switch ((uint8)dataType & 0x03)
			{
				case 0x00:  return 8;
				case 0x01:  return 16;
				case 0x02:  return 32;
				case 0x03:  return 64;
			}
			return 0;
		}


		struct ParameterInfo
		{
			using Semantics = Nativizer::LookupEntry::ParameterInfo::Semantics;

			size_t mOpcodeIndex = 0;
			size_t mOffset = 0;
			size_t mSize = 0;
			Semantics mSemantics = Semantics::INTEGER;
			BaseType mDataType = BaseType::INT_CONST;

			inline ParameterInfo(size_t opcodeIndex, size_t size, Semantics semantics, BaseType dataType = BaseType::INT_CONST) :
				mOpcodeIndex(opcodeIndex), mOffset(0), mSize(size), mSemantics(semantics), mDataType(dataType)
			{}
		};

		struct Parameters
		{
			std::vector<ParameterInfo> mParameters;
			size_t mTotalSize = 0;

			void clear()
			{
				mParameters.clear();
				mTotalSize = 0;
			}

			size_t add(size_t opcodeIndex, size_t size, ParameterInfo::Semantics semantics, BaseType dataType = BaseType::INT_CONST)
			{
				const size_t offset = mTotalSize;
				mParameters.emplace_back(opcodeIndex, size, semantics, dataType);
				mParameters.back().mOffset = offset;
				mTotalSize += size;
				return offset;
			}
		};

		struct OpcodeInfo
		{
			Nativizer::OpcodeSubtypeInfo mSubtypeInfo;
			ParameterInfo* mParameter = nullptr;
			uint64 mHash = 0;
		};

		struct Assignment
		{
			struct Node
			{
				enum class Type
				{
					INVALID,
					CONSTANT,			// Read a constant value (not really used at the moment; this could be used for specialized nativization that includes hard-coded constants)
					PARAMETER,			// Read from a parameter
					VALUE_STACK,		// Read from or write to the stack
					VARIABLE,			// Read from or write to a variable
					MEMORY,				// Read from or write to a memory location
					MEMORY_FIXED,		// Read from a fixed memory location via a direct access pointer
					OPERATION_UNARY,	// Execute a unary operation
					OPERATION_BINARY,	// Execute a binary operation
					TEMP_VAR			// Read from or write to a temporary local variable ("var0", "var1", etc. in nativized code)
				};

				Type mType = Type::INVALID;
				BaseType mDataType = BaseType::INT_CONST;
				uint64 mValue = 0;
				uint64 mParameterOffset = 0;
				Node* mChild[2] = { nullptr, nullptr };

				inline Node() {}
				inline Node(Type type, BaseType dataType) : mType(type), mDataType(dataType) {}
				inline Node(Type type, BaseType dataType, uint64 value) : mType(type), mDataType(dataType), mValue(value) {}
				inline Node(Type type, BaseType dataType, uint64 value, uint64 parameterOffset) : mType(type), mDataType(dataType), mValue(value), mParameterOffset(parameterOffset) {}
			};

			Node* mDest = nullptr;
			Node* mSource = nullptr;
			BaseType mDataType = BaseType::INT_CONST;
			bool mExplicitCast = false;
			size_t mOpcodeIndex = 0;
			size_t mAssignmentIndex = 0;

			void outputParameter(std::string& line, int64 value, BaseType dataType = BaseType::INT_CONST, bool isPointer = false) const
			{
				const std::string& dataTypeString = getDataTypeString(dataType, false);
				if (isPointer)
				{
					line += "*context.getParameter<" + dataTypeString + "*>(";
				}
				else
				{
					line += "context.getParameter<" + dataTypeString + ">(";
				}
				if (value != 0)
				{
					line += std::to_string(value);
				}
				line += ")";
			}

			void outputNode(std::string& line, const Node& node, bool isWrite, bool* closeParenthesis = nullptr) const
			{
				switch (node.mType)
				{
					case Node::Type::CONSTANT:
					{
						switch (node.mDataType)
						{
							case BaseType::INT_8:	line += std::to_string((int8)(node.mValue));			break;
							case BaseType::INT_16:	line += std::to_string((int16)(node.mValue));			break;
							case BaseType::INT_32:	line += std::to_string((int32)(node.mValue));			break;
							case BaseType::INT_64:	line += std::to_string((int64)(node.mValue)) + "ll";	break;
							case BaseType::UINT_8:	line += std::to_string((uint8)(node.mValue));			break;
							case BaseType::UINT_16:	line += std::to_string((uint16)(node.mValue));			break;
							case BaseType::UINT_32:	line += std::to_string((uint32)(node.mValue));			break;
							case BaseType::UINT_64:	line += std::to_string((uint64)(node.mValue)) + "ull";	break;
							default:				line += std::to_string((uint32)(node.mValue));			break;
						}
						break;
					}

					case Node::Type::PARAMETER:
					{
						// Always read as int64, as that's what default opcode execution does as well
						outputParameter(line, node.mParameterOffset, BaseType::INT_64);
						break;
					}

					case Node::Type::VALUE_STACK:
					{
						const std::string& dataTypeString = getDataTypeString(node.mDataType);
						if (isWrite)
						{
							line += "context.writeValueStack<" + dataTypeString + ">(";
							line += std::to_string((int)node.mValue);
							line += ", (" + dataTypeString + ")";
							*closeParenthesis = true;
						}
						else
						{
							line += "context.readValueStack<" + dataTypeString + ">(";
							line += std::to_string((int)node.mValue);
							line += ")";
						}
						break;
					}

					case Node::Type::VARIABLE:
					{
						const uint32 variableId = (uint32)node.mValue;
						const Variable::Type type = (Variable::Type)(variableId >> 28);
						switch (type)
						{
							case Variable::Type::LOCAL:
							{
								const std::string& dataTypeString = getDataTypeString(node.mDataType);
								if (isWrite)
								{
									line += "context.writeLocalVariable<" + dataTypeString + ">(";
									outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
									line += ", ";
									*closeParenthesis = true;
								}
								else
								{
									line += "context.readLocalVariable<" + dataTypeString + ">(";
									outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
									line += ")";
								}
								break;
							}

							case Variable::Type::USER:
							{
								line += "static_cast<GlobalVariable&>(context.mControlFlow->mProgram->getGlobalVariableById(";
								outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
								if (isWrite)
								{
									line += ")).setValue(";
									*closeParenthesis = true;
								}
								else
								{
									line += ")).getValue()";
								}
								break;
							}

							case Variable::Type::GLOBAL:
							case Variable::Type::EXTERNAL:
							{
								outputParameter(line, node.mParameterOffset, node.mDataType, true);
								break;
							}
						}

						break;
					}

					case Node::Type::MEMORY:
					{
						const std::string& dataTypeString = getDataTypeString(node.mDataType, true);
						if (isWrite)
						{
							line += "OpcodeExecUtils::writeMemory<" + dataTypeString + ">(*context.mControlFlow, ";
							outputNode(line, *node.mChild[0], false);
							line += ", ";
							*closeParenthesis = true;
						}
						else
						{
							line += "OpcodeExecUtils::readMemory<" + dataTypeString + ">(*context.mControlFlow, ";
							outputNode(line, *node.mChild[0], false);
							line += ")";
						}
						break;
					}

					case Node::Type::MEMORY_FIXED:
					{
						const bool swapBytes = (node.mValue & 0x01) != 0;
						const size_t bytes = DataTypeHelper::getSizeOfBaseType(node.mDataType);
						if (swapBytes && bytes >= 2)
						{
							line += *String(0, "swapBytes%d(", bytes * 8);
							outputParameter(line, node.mParameterOffset, node.mDataType, true);
							line += ")";
						}
						else
						{
							outputParameter(line, node.mParameterOffset, node.mDataType, true);
						}
						break;
					}

					case Node::Type::OPERATION_BINARY:
					{
						const char* operatorString = "";
						const char* functionCall = nullptr;
						bool ignoreSigned = false;
						bool booleanResult = false;		// TODO: This is probably not really needed
						bool isShift = false;
						switch ((Opcode::Type)node.mValue)
						{
							case Opcode::Type::ARITHM_ADD:	operatorString = "+";   ignoreSigned = true;   booleanResult = false;	break;
							case Opcode::Type::ARITHM_SUB:	operatorString = "-";   ignoreSigned = true;   booleanResult = false;	break;
							case Opcode::Type::ARITHM_MUL:	operatorString = "*";   ignoreSigned = false;  booleanResult = false;	break;
							case Opcode::Type::ARITHM_MOD:	operatorString = "%";   ignoreSigned = false;  booleanResult = false;	break;
							case Opcode::Type::ARITHM_AND:	operatorString = "&";   ignoreSigned = true;   booleanResult = false;	break;
							case Opcode::Type::ARITHM_OR:	operatorString = "|";   ignoreSigned = true;   booleanResult = false;	break;
							case Opcode::Type::ARITHM_XOR:	operatorString = "^";   ignoreSigned = true;   booleanResult = false;	break;
							case Opcode::Type::ARITHM_SHL:	operatorString = "<<";  ignoreSigned = true;   booleanResult = false;	isShift = true;  break;
							case Opcode::Type::ARITHM_SHR:	operatorString = ">>";  ignoreSigned = false;  booleanResult = false;	isShift = true;  break;
							case Opcode::Type::COMPARE_EQ:	operatorString = "==";  ignoreSigned = true;   booleanResult = true;	break;
							case Opcode::Type::COMPARE_NEQ:	operatorString = "!=";  ignoreSigned = true;   booleanResult = true;	break;
							case Opcode::Type::COMPARE_LT:	operatorString = "<";   ignoreSigned = false;  booleanResult = true;	break;
							case Opcode::Type::COMPARE_LE:	operatorString = "<=";  ignoreSigned = false;  booleanResult = true;	break;
							case Opcode::Type::COMPARE_GT:	operatorString = ">";   ignoreSigned = false;  booleanResult = true;	break;
							case Opcode::Type::COMPARE_GE:	operatorString = ">=";  ignoreSigned = false;  booleanResult = true;	break;
							case Opcode::Type::ARITHM_DIV:	functionCall = "OpcodeExecUtils::safeDivide";   ignoreSigned = false;  booleanResult = false;	break;
							default:  break;
						}

						const std::string& dataTypeString = getDataTypeString(node.mDataType, ignoreSigned);
						if (nullptr == functionCall)
						{
							line += "((" + dataTypeString + ")(";
							outputNode(line, *node.mChild[0], false);
							line += std::string(") ") + operatorString + " (" + dataTypeString + ")(";
							if (isShift)
								line += "(";
							outputNode(line, *node.mChild[1], false);
							if (isShift)
								line += ") & " + rmx::hexString(getDataTypeBits(node.mDataType) - 1, 2);
							line += "))";
						}
						else
						{
							line += functionCall;
							line += "<" + dataTypeString + ">((" + dataTypeString + ")";
							outputNode(line, *node.mChild[0], false);
							line += ", (" + dataTypeString + ")";
							outputNode(line, *node.mChild[1], false);
							line += ")";
						}
						break;
					}

					case Node::Type::OPERATION_UNARY:
					{
						bool ignoreSigned = false;
						switch ((Opcode::Type)node.mValue)
						{
							case Opcode::Type::ARITHM_NEG:	  line += "-";  ignoreSigned = false;  break;
							case Opcode::Type::ARITHM_NOT:	  line += "!";  ignoreSigned = true;   break;
							case Opcode::Type::ARITHM_BITNOT: line += "~";  ignoreSigned = true;   break;
							default:  break;
						}

						if (!ignoreSigned)
							line += "(signed)";		// TODO: This is more of a hack....
						outputNode(line, *node.mChild[0], false);
						break;
					}

					case Node::Type::TEMP_VAR:
					{
						if (isWrite)
						{
							const std::string& dataTypeString = getDataTypeString(node.mDataType);
							line += *String(0, "const %s var%d", dataTypeString.c_str(), (int)node.mValue);
						}
						else
						{
							line += *String(0, "var%d", (int)node.mValue);
						}
						break;
					}

					default:
						RMX_ERROR("Not handled", );
						break;
				}
			}

			void outputLine(std::string& line) const
			{
				bool closeParenthesis = false;
				outputNode(line, *mDest, true, &closeParenthesis);

				if (!closeParenthesis)
					line += " = ";
				if (mExplicitCast)
					line += "(" + getDataTypeString(mDataType, true) + ")";

				outputNode(line, *mSource, false);
				if (closeParenthesis)
					line += ")";
				line += ";";
			}
		};

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
				outInfo.mSubtypeData |= ((uint32)firstOpcode.mDataType & 0xf7) << 16;		// Data type, ignoring signed/unsigned
				outInfo.mSubtypeData |= (firstOpcode.mParameter >> 28);						// Variable type
				break;

			case Opcode::Type::READ_MEMORY:
				outInfo.mSubtypeData |= ((uint32)firstOpcode.mDataType & 0xf7) << 16;		// Data type, ignoring signed/unsigned
				outInfo.mSubtypeData |= firstOpcode.mParameter;								// Flag for variant that does not consume its input
				break;

			case Opcode::Type::WRITE_MEMORY:
				outInfo.mSubtypeData |= ((uint32)firstOpcode.mDataType & 0xf7) << 16;		// Data type, ignoring signed/unsigned
				outInfo.mSubtypeData |= firstOpcode.mParameter;								// Flag for exchanged inputs variant
				break;

			case Opcode::Type::CAST_VALUE:
				outInfo.mSubtypeData |= (uint32)OpcodeHelper::getCastExecType(firstOpcode);	// Type of cast
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
				outInfo.mSubtypeData |= ((uint32)firstOpcode.mDataType & 0xf7) << 16;		// Data type, ignoring signed/unsigned
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
		if (numOpcodes > MAX_OPCODES)
			numOpcodes = MAX_OPCODES;

		OpcodeInfo opcodeInfo[MAX_OPCODES];
		size_t numOpcodeInfos = 0;

		// Check how many opcodes are supported -- TODO: This will have to get removed when all are supported
		{
			for (size_t index = 0; index < numOpcodes; ++index)
			{
				const Opcode& opcode = opcodes[index];
				RMX_ASSERT(opcode.mType != Opcode::Type::MAKE_BOOL, "MAKE_BOOL should not occur any more");
				RMX_ASSERT(opcode.mType != Opcode::Type::DUPLICATE, "DUPLICATE should not occur any more");
				RMX_ASSERT(opcode.mType != Opcode::Type::EXCHANGE, "EXCHANGE should not occur any more");

				const bool isSupported = (opcode.mType == Opcode::Type::MOVE_STACK ||
										  //opcode.mType == Opcode::Type::MOVE_VAR_STACK ||
										  opcode.mType == Opcode::Type::PUSH_CONSTANT ||
										  (opcode.mType >= Opcode::Type::GET_VARIABLE_VALUE && opcode.mType <= Opcode::Type::COMPARE_GE));
				if (!isSupported)
				{
					numOpcodes = index;
					break;
				}
			}

			if (numOpcodes < MIN_OPCODES)
				return numOpcodes + 1;
		}

		// Get subtype infos from the opcodes, and build hashes
		uint64 hash = getStartHash();
		{
			for (size_t opcodeIndex = 0; opcodeIndex < numOpcodes; )
			{
				OpcodeInfo& info = opcodeInfo[numOpcodeInfos];
				info.mSubtypeInfo.mFirstOpcodeIndex = opcodeIndex;
				getOpcodeSubtypeInfo(info.mSubtypeInfo, &opcodes[opcodeIndex], numOpcodes - opcodeIndex, *mMemoryAccessHandler);
				opcodeIndex += info.mSubtypeInfo.mConsumedOpcodes;
				hash = addOpcodeSubtypeInfoToHash(hash, info.mSubtypeInfo);
				info.mHash = hash;
				++numOpcodeInfos;
			}

			if (mBuiltDictionary.mEntries.count(hash) != 0)
				return numOpcodes;
		}

		static Parameters parameters;
		parameters.clear();

		static std::vector<Assignment> assignments;
		assignments.clear();

		static std::vector<Assignment::Node> nodes;
		RMX_ASSERT(nodes.size() <= 0x100, "Oops, a reallocation happened");
		nodes.clear();
		nodes.reserve(0x100);

		int stackPosition = 0;	// Relative to initial stack position

		for (size_t infoIndex = 0; infoIndex < numOpcodeInfos; ++infoIndex)
		{
			const OpcodeInfo& info = opcodeInfo[infoIndex];
			const size_t opcodeIndex = info.mSubtypeInfo.mFirstOpcodeIndex;
			const Opcode& opcode = opcodes[opcodeIndex];
			const size_t oldNumAssignments = assignments.size();

			switch (info.mSubtypeInfo.mSpecialType)
			{
				case OpcodeSubtypeInfo::SpecialType::FIXED_MEMORY_READ:
				{
					const Opcode& readMemoryOpcode = opcodes[opcodeIndex+1];
					const BaseType dataType = readMemoryOpcode.mDataType;
					const uint64 swapBytesFlag = (info.mSubtypeInfo.mSubtypeData & 0x0001);
					const bool consumeInput = (info.mSubtypeInfo.mSubtypeData & 0x0002) == 0;

					if (!consumeInput)
					{
						// First add an assigment to push the address to the stack
						const size_t parameterOffset = parameters.add(opcodeIndex, 8, ParameterInfo::Semantics::INTEGER);
						Assignment& assignment = vectorAdd(assignments);
						assignment.mDest   = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition);
						assignment.mSource = &nodes.emplace_back(Assignment::Node::Type::PARAMETER, opcode.mDataType, 0, parameterOffset);
						assignment.mDataType = opcode.mDataType;
						++stackPosition;
					}

					const size_t parameterOffset = parameters.add(opcodeIndex, 8, ParameterInfo::Semantics::FIXED_MEMORY_ADDRESS);
					Assignment& assignment = vectorAdd(assignments);
					assignment.mDest   = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, dataType, stackPosition);
					assignment.mSource = &nodes.emplace_back(Assignment::Node::Type::MEMORY_FIXED, dataType, swapBytesFlag, parameterOffset);
					assignment.mDataType = dataType;
					++stackPosition;
					break;
				}

				default:
				{
					// Process a normal single opcode
					switch (opcode.mType)
					{
						case Opcode::Type::MOVE_STACK:
						{
							stackPosition += (int16)opcode.mParameter;
							break;
						}

						case Opcode::Type::PUSH_CONSTANT:
						{
							// Normal processing of the PUSH_CONSTANT opcode
							Assignment& assignment = vectorAdd(assignments);
							assignment.mDest = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition);
							if (info.mSubtypeInfo.mSubtypeData & 0x8000)
							{
								// Specialized version hard-codes the constant value
								assignment.mSource = &nodes.emplace_back(Assignment::Node::Type::CONSTANT, opcode.mDataType, opcode.mParameter);
							}
							else
							{
								// Generic version reads the constant value as a parameter
								const size_t parameterOffset = parameters.add(opcodeIndex, 8, ParameterInfo::Semantics::INTEGER);
								assignment.mSource = &nodes.emplace_back(Assignment::Node::Type::PARAMETER, opcode.mDataType, 0, parameterOffset);
							}
							assignment.mDataType = opcode.mDataType;
							++stackPosition;
							break;
						}

						case Opcode::Type::GET_VARIABLE_VALUE:
						case Opcode::Type::SET_VARIABLE_VALUE:
						{
							const Variable::Type type = (Variable::Type)((uint32)(opcode.mParameter) >> 28);
							size_t parameterOffset;
							switch (type)
							{
								case Variable::Type::EXTERNAL:
								{
									parameterOffset = parameters.add(opcodeIndex, 8, ParameterInfo::Semantics::EXTERNAL_VARIABLE, opcode.mDataType);
									break;
								}
								case Variable::Type::GLOBAL:
								{
									parameterOffset = parameters.add(opcodeIndex, 8, ParameterInfo::Semantics::GLOBAL_VARIABLE);
									break;
								}
								default:
								{
									parameterOffset = parameters.add(opcodeIndex, 4, ParameterInfo::Semantics::INTEGER);
									break;
								}
							}

							if (opcode.mType == Opcode::Type::GET_VARIABLE_VALUE)
							{
								Assignment& assignment = vectorAdd(assignments);
								assignment.mDest	 = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition);
								assignment.mSource	 = &nodes.emplace_back(Assignment::Node::Type::VARIABLE, opcode.mDataType, (uint32)opcode.mParameter, parameterOffset);
								assignment.mDataType = opcode.mDataType;
								assignment.mExplicitCast = true;
								++stackPosition;
							}
							else
							{
								Assignment& assignment = vectorAdd(assignments);
								assignment.mDest	 = &nodes.emplace_back(Assignment::Node::Type::VARIABLE, opcode.mDataType, (uint32)opcode.mParameter, parameterOffset);
								assignment.mSource	 = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
								assignment.mDataType = opcode.mDataType;
								assignment.mExplicitCast = true;
							}
							break;
						}

						case Opcode::Type::READ_MEMORY:
						{
							const bool consumeInput = (opcode.mParameter == 0);
							Assignment& assignment = vectorAdd(assignments);
							assignment.mDest			  = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - (consumeInput ? 1 : 0));
							assignment.mSource			  = &nodes.emplace_back(Assignment::Node::Type::MEMORY, opcode.mDataType);
							assignment.mSource->mChild[0] = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, BaseType::UINT_32, stackPosition - 1);
							assignment.mDataType = opcode.mDataType;
							if (!consumeInput)
								++stackPosition;
							break;
						}

						case Opcode::Type::WRITE_MEMORY:
						{
							const bool exchangedInputs = (opcode.mParameter != 0);
							{
								// Main assignment
								Assignment& assignment = vectorAdd(assignments);
								assignment.mDest			= &nodes.emplace_back(Assignment::Node::Type::MEMORY, opcode.mDataType);
								assignment.mDest->mChild[0]	= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, BaseType::UINT_32, stackPosition - (exchangedInputs ? 2 : 1));
								assignment.mSource			= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - (exchangedInputs ? 1 : 2));
								assignment.mDataType = opcode.mDataType;
								assignment.mExplicitCast = true;
							}
							if (exchangedInputs)
							{
								// Add another assignment to copy the value to the top-of-stack, where it might be expected by the next assignments
								Assignment& assignment = vectorAdd(assignments);
								assignment.mDest	= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 2);
								assignment.mSource	= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
								assignment.mDataType = opcode.mDataType;
								assignment.mExplicitCast = true;
							}
							--stackPosition;
							break;
						}

						case Opcode::Type::CAST_VALUE:
						{
							const BaseType dataType = OpcodeHelper::getCastExecType(opcode);
							Assignment& assignment = vectorAdd(assignments);
							assignment.mDest	 = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, dataType, stackPosition - 1);
							assignment.mSource	 = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, dataType, stackPosition - 1);
							assignment.mDataType = dataType;
							assignment.mExplicitCast = true;
							break;
						}

						case Opcode::Type::ARITHM_ADD:
						case Opcode::Type::ARITHM_SUB:
						case Opcode::Type::ARITHM_MUL:
						case Opcode::Type::ARITHM_DIV:
						case Opcode::Type::ARITHM_MOD:
						case Opcode::Type::ARITHM_AND:
						case Opcode::Type::ARITHM_OR:
						case Opcode::Type::ARITHM_XOR:
						case Opcode::Type::ARITHM_SHL:
						case Opcode::Type::ARITHM_SHR:
						case Opcode::Type::COMPARE_EQ:
						case Opcode::Type::COMPARE_NEQ:
						case Opcode::Type::COMPARE_LT:
						case Opcode::Type::COMPARE_LE:
						case Opcode::Type::COMPARE_GT:
						case Opcode::Type::COMPARE_GE:
						{
							Assignment& assignment = vectorAdd(assignments);
							assignment.mDest				= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 2);
							assignment.mSource				= &nodes.emplace_back(Assignment::Node::Type::OPERATION_BINARY, opcode.mDataType, (uint64)opcode.mType);
							assignment.mSource->mChild[0]	= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 2);
							assignment.mSource->mChild[1]	= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							assignment.mDataType = opcode.mDataType;
							--stackPosition;
							break;
						}

						case Opcode::Type::ARITHM_NEG:
						case Opcode::Type::ARITHM_NOT:
						case Opcode::Type::ARITHM_BITNOT:
						{
							Assignment& assignment = vectorAdd(assignments);
							assignment.mDest				= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							assignment.mSource				= &nodes.emplace_back(Assignment::Node::Type::OPERATION_UNARY, opcode.mDataType, (uint64)opcode.mType);
							assignment.mSource->mChild[0]	= &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							assignment.mDataType			= opcode.mDataType;
							assignment.mExplicitCast = true;
							break;
						}

						default:
							break;
					}
					break;
				}

			}

			for (size_t i = oldNumAssignments; i < assignments.size(); ++i)
			{
				assignments[i].mOpcodeIndex = opcodeIndex;
				assignments[i].mAssignmentIndex = i;
			}
		}

		// Assign parameter offsets
		//  -> This is a separate step, now that the parameter list is filled; i.e. there won't be any more reallocations and the pointers stay valid
		for (ParameterInfo& param : parameters.mParameters)
		{
			opcodeInfo[param.mOpcodeIndex].mParameter = &param;
		}

		// Postprocessing
		//  -> Build temp vars where a value is both read (from any source) and written (to stack)
		{
			struct Read
			{
				Assignment* mAssignment = nullptr;
				Assignment::Node* mNode = nullptr;
			};
			struct TempVar
			{
				Assignment* mWrite = nullptr;
				std::vector<Read> mReads;
				bool mPreserve = false;			// If set, this temp var must not be removed
				bool mOutputToStack = false;	// If set, this temp var must also write to the stack, in addition to be used in the reads
			};
			static std::vector<TempVar> tempVars;
			tempVars.clear();
			tempVars.reserve(0x20);

			// This lookup is meant to mirror the stack (with index MAX_OPCODES representing the initial stack position)
			//  -> It's used to track which assigment nodes consume the value written by which other assignment
			//  -> This way, we can build pairs of nodes that can be linked together:
			//      where possible, the writing node gets integrated directly as input for the reading node, without the need of having a temp var in between
			static TempVar* tempVarLookup[MAX_OPCODES * 2];
			for (size_t k = 0; k < MAX_OPCODES * 2; ++k)
			{
				tempVarLookup[k] = nullptr;
			}
			int lowestWrittenStackPosition = stackPosition;

			// First collect temp vars
			for (size_t assignmentIndex = 0; assignmentIndex < assignments.size(); ++assignmentIndex)
			{
				Assignment& assignment = assignments[assignmentIndex];

				// Iterate recursively through the node and its inner child nodes
				static std::vector<Assignment::Node*> nodeStack;
				nodeStack.clear();
				nodeStack.push_back(assignment.mDest);		// Push first to the stack, so it gets handled last
				nodeStack.push_back(assignment.mSource);
				while (!nodeStack.empty())
				{
					Assignment::Node& node = *nodeStack.back();
					nodeStack.pop_back();

					switch (node.mType)
					{
						case Assignment::Node::Type::VALUE_STACK:
						{
							// Ignore the assignment's write access (that one needs separate handling afterwards, see below)
							if (&node != assignment.mDest)
							{
								// Get the temp var at the respective stack position
								const int readStackPosition = (int)node.mValue;
								TempVar* tempVar = tempVarLookup[MAX_OPCODES + readStackPosition];
								if (nullptr != tempVar)
								{
									Read& read = vectorAdd(tempVar->mReads);
									read.mAssignment = &assignment;
									read.mNode = &node;
									tempVar->mPreserve = true;		// This temp var has a write and at leats one read, so it must not be removed by cleanup later on
								}
							}
							break;
						}

						case Assignment::Node::Type::MEMORY:
						case Assignment::Node::Type::OPERATION_UNARY:
						{
							// Go deeper into the child node
							nodeStack.push_back(node.mChild[0]);
							break;
						}

						case Assignment::Node::Type::OPERATION_BINARY:
						{
							// Go deeper into both child nodes
							nodeStack.push_back(node.mChild[0]);
							nodeStack.push_back(node.mChild[1]);
							break;
						}

						default:
							break;
					}
				}

				// Add assignment's write as a temp var
				//  -> If it turns out that this temp var is not consumed by a read, it will get removed again later (see below)
				if (assignment.mDest->mType == Assignment::Node::Type::VALUE_STACK)
				{
					const int writeStackPosition = (int)assignment.mDest->mValue;
					lowestWrittenStackPosition = std::min(lowestWrittenStackPosition, writeStackPosition);

					TempVar& tempVar = vectorAdd(tempVars);
					tempVarLookup[MAX_OPCODES + writeStackPosition] = &tempVar;
					tempVar.mWrite = &assignment;
					tempVar.mReads.clear();
					tempVar.mPreserve = false;
					tempVar.mOutputToStack = false;
				}
			}

			// Now evaluate which of the temp vars (or assigments if you will) are required to be written to the stack
			//  -> This is everything below the final stack position
			for (int pos = lowestWrittenStackPosition; pos < stackPosition; ++pos)
			{
				TempVar* tempVar = tempVarLookup[MAX_OPCODES + pos];
				if (nullptr != tempVar)
				{
					if (!tempVar->mReads.empty())
					{
						// Temp var has reads, which means it gets read by an assigment, but does not get consumed by it
						//  -> We need to output an additional stack write for it
						tempVar->mOutputToStack = true;		// Note that this implies "mPreserve", as it only gets set when there's also reads
					}
				}
			}

			// Remove all temp vars for stack writes that are not marked as preserved
			//  -> That's all that did not get consumed
			for (int i = (int)tempVars.size() - 1; i >= 0; --i)
			{
				TempVar& tempVar = tempVars[i];
				if (!tempVar.mPreserve)
				{
					const int writeStackPosition = (int)tempVar.mWrite->mDest->mValue;
					if (writeStackPosition >= stackPosition)
					{
						// Temp var refers to an assignment that can be dropped altogether
						//  -> E.g. WRITE_MEMORY with exchanged inputs can create such assignments; but in many cases, their result goes unused
						tempVar.mWrite->mDest = nullptr;
					}
					tempVars.erase(tempVars.begin() + i);
				}
			}

			// Now go through the remaining temp vars and properly apply them
			int nextTempVarNumber = 0;
			for (TempVar& tempVar : tempVars)
			{
				Assignment& write = *tempVar.mWrite;
				const int writeStackPosition = (int)write.mDest->mValue;

				// Replace the nodes referenced in the remaining temp vars, so that they actually use the temp var
				write.mDest->mType = Assignment::Node::Type::TEMP_VAR;
				write.mDest->mValue = nextTempVarNumber;
				for (const Read& read : tempVar.mReads)
				{
					RMX_ASSERT(writeStackPosition == read.mNode->mValue, "Difference in stack positions of read and write of a temp var");
					read.mNode->mType = Assignment::Node::Type::TEMP_VAR;
					read.mNode->mValue = nextTempVarNumber;
				}

				// And if the temp var needs to be written to the stack, add an additional assigment to do right that
				if (tempVar.mOutputToStack)
				{
					Assignment& assignment = vectorAdd(assignments);
					assignment.mDest	 = &nodes.emplace_back(Assignment::Node::Type::VALUE_STACK, write.mDataType, writeStackPosition);
					assignment.mSource	 = &nodes.emplace_back(Assignment::Node::Type::TEMP_VAR, write.mDataType, nextTempVarNumber);
					assignment.mDataType = write.mDataType;

					// Register as a read, otherwise the optimization below could try to integrate this temp var
					Read& read = vectorAdd(tempVar.mReads);
					read.mAssignment = &assignment;
					read.mNode = assignment.mSource;
				}

				++nextTempVarNumber;
			}

			// As an additional optimization, integrate some temp vars directly into where they are used
			// TODO: This needs to preserve casts
			for (size_t i = 0; i < tempVars.size(); ++i)
			{
				TempVar& tempVar = tempVars[i];
				if (tempVars[i].mReads.size() != 1)
					continue;

				if (tempVar.mWrite->mSource->mType != Assignment::Node::Type::PARAMETER)
				{
					// TODO: Do not allow integration when there's variable or memory writes in between (except for parameters)
					continue;
				}

				// Replace read node
				*tempVar.mReads[0].mNode = *tempVar.mWrite->mSource;
				tempVar.mReads[0].mAssignment = tempVar.mWrite;

				// Invalidate old write assignment
				tempVar.mWrite->mDest = nullptr;
			}

			// Remove all assignments that just copy a value around (without an actual cast)
			for (size_t i = 0; i < tempVars.size(); ++i)
			{
				TempVar& tempVar = tempVars[i];
				if (nullptr != tempVar.mWrite->mDest && tempVar.mWrite->mSource->mType == Assignment::Node::Type::TEMP_VAR)
				{
					const int sourceTempVarNumber = (int)tempVar.mWrite->mSource->mValue;
					const TempVar& sourceTempVar = tempVars[sourceTempVarNumber];
					if (nullptr != sourceTempVar.mWrite->mDest && sourceTempVar.mWrite->mDest->mDataType == tempVar.mWrite->mDest->mDataType)
					{
						// Replace in all reads
						for (Read& read : tempVar.mReads)
						{
							read.mNode->mValue = sourceTempVarNumber;
						}

						// Invalidate old write assignment
						tempVar.mWrite->mDest = nullptr;
					}
				}
			}
		}

		// Generate code for the assignments
		{
			std::string line = "// First occurrence: " + function.getName();
			if (opcodes[0].mLineNumber != 0)
				line = line + ", line " + std::to_string(opcodes[0].mLineNumber - function.mSourceBaseLineOffset + 1);
			writer.writeLine(line);

			writer.writeLine("static void exec_" + rmx::hexString(hash, 16, "") + "(const RuntimeOpcodeContext context)");
			writer.beginBlock();

			// Write lines
			for (const Assignment& assignment : assignments)
			{
				if (nullptr != assignment.mDest)	// Ignore the invalidated assignments
				{
					line.clear();
					assignment.outputLine(line);
					writer.writeLine(line);
				}
			}

			if (stackPosition != 0)
			{
				writer.writeLine("context.moveValueStack(" + std::to_string(stackPosition) + ");");
			}

			writer.endBlock();
			writer.writeEmptyLine();
		}

		// Register
		for (size_t index = 1; index < numOpcodeInfos-1; ++index)
		{
			const uint64 partialHash = opcodeInfo[index].mHash;
			mBuiltDictionary.mEntries[partialHash];		// Creates the entry with initial value, or leaves it like it is if it was already existing
		}

		{
			LookupEntry& entry = mBuiltDictionary.mEntries[hash];
			entry.mExecFunc = (ExecFunc)1;	// Treating this as a bool, we only care if it's a nullptr or not

			std::vector<ParameterInfo>& params = parameters.mParameters;
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
				parameterPtr->mOffset = (uint16)parameters.mTotalSize;
				parameterPtr->mOpcodeIndex = 0xff;
				parameterPtr->mSemantics = (LookupEntry::ParameterInfo::Semantics)0xff;
			}
		}

		return numOpcodes;
	}

}
