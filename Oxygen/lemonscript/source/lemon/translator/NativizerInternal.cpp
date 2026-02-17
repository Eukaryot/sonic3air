/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/translator/NativizerInternal.h"
#include "lemon/translator/SourceCodeWriter.h"
#include "lemon/program/function/Function.h"
#include "lemon/program/OpcodeHelper.h"


// Optimization switch for the nativizer code output:
//  - Level 0 creates basic nativization by translating opcodes into lines of C++ code
//  - Level 1 avoids the value stack where possible, by using local C++ variables ("temp vars") in the nativized code
//  - Level 2 resolves these local C++ variables where the lines that assign them can be merged directly into the lines that read them
#define NATIVIZER_OPTIMIZATION_LEVEL 2


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
			static const std::string TYPESTRING_float  = "float";
			static const std::string TYPESTRING_double = "double";
			static const std::string TYPESTRING_empty  = "";

			if (BaseTypeHelper::isIntegerType(dataType))
			{
				const bool isSigned = ignoreSigned ? false : BaseTypeHelper::isIntegerSigned(dataType);
				switch (BaseTypeHelper::getIntegerSizeFlags(dataType))
				{
					case 0x00:  return isSigned ? TYPESTRING_int8  : TYPESTRING_uint8;
					case 0x01:  return isSigned ? TYPESTRING_int16 : TYPESTRING_uint16;
					case 0x02:  return isSigned ? TYPESTRING_int32 : TYPESTRING_uint32;
					case 0x03:  return isSigned ? TYPESTRING_int64 : TYPESTRING_uint64;
				}
			}
			else if (BaseTypeHelper::isFloatingPointType(dataType))
			{
				switch (dataType)
				{
					case BaseType::FLOAT:  return TYPESTRING_float;
					case BaseType::DOUBLE: return TYPESTRING_double;
					default:  break;
				}
			}
			RMX_ERROR("Unsupported base type " << (int)dataType, );
			return TYPESTRING_empty;
		}

		size_t getIntegerDataTypeBits(BaseType dataType)
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
	}



	void NativizerInternal::Assignment::outputParameter(std::string& line, int64 value, BaseType dataType, bool isPointer) const
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

	void NativizerInternal::Assignment::outputDestNode(std::string& line, const Node& node, bool& closeParenthesis) const
	{
		switch (node.mType)
		{
			case Node::Type::VALUE_STACK:
			{
				const std::string& dataTypeString = getDataTypeString(node.mDataType);
				line += "context.writeValueStack<" + dataTypeString + ">(";
				line += std::to_string((int)node.mValue);
				line += ", ";
				closeParenthesis = true;
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
						line += "context.writeLocalVariable<" + dataTypeString + ">(";
						outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
						line += ", ";
						closeParenthesis = true;
						break;
					}

					case Variable::Type::USER:
					{
						line += "context.mControlFlow->getProgram().getGlobalVariableByID(";
						outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
						line += ").as<GlobalVariable>().setValue(";
						closeParenthesis = true;
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
				line += "OpcodeExecUtils::writeMemory<" + dataTypeString + ">(*context.mControlFlow, ";
				outputSourceNode(line, *node.mChild[0]);
				line += ", ";
				closeParenthesis = true;
				break;
			}

			case Node::Type::TEMP_VAR:
			{
				const std::string& dataTypeString = getDataTypeString(node.mDataType);
				line += *String(0, "const AnyBaseValue var%d((%s)", (int)node.mValue, dataTypeString.c_str());
				closeParenthesis = true;
				break;
			}

			default:
				RMX_ERROR("Not handled", );
				break;
		}
	}

	void NativizerInternal::Assignment::outputSourceNode(std::string& line, const Node& node) const
	{
		switch (node.mType)
		{
			case Node::Type::CONSTANT:
			{
				const AnyBaseValue constant(node.mValue);
				switch (node.mDataType)
				{
					case BaseType::INT_8:	line += std::to_string(constant.get<int8>());			break;
					case BaseType::INT_16:	line += std::to_string(constant.get<int16>());			break;
					case BaseType::INT_32:	line += std::to_string(constant.get<int32>());			break;
					case BaseType::INT_64:	line += std::to_string(constant.get<int64>()) + "ll";	break;
					case BaseType::UINT_8:	line += std::to_string(constant.get<uint8>());			break;
					case BaseType::UINT_16:	line += std::to_string(constant.get<uint16>());			break;
					case BaseType::UINT_32:	line += std::to_string(constant.get<uint32>());			break;
					case BaseType::UINT_64:	line += std::to_string(constant.get<uint64>()) + "ull";	break;
					case BaseType::FLOAT:	line += std::to_string(constant.get<float>()) + 'f';	break;
					case BaseType::DOUBLE:	line += std::to_string(constant.get<double>());			break;
					default:				line += std::to_string(constant.get<uint32>());			break;
				}
				break;
			}

			case Node::Type::PARAMETER:
			{
				outputParameter(line, node.mParameterOffset, node.mDataType);
				break;
			}

			case Node::Type::VALUE_STACK:
			{
				const std::string& dataTypeString = getDataTypeString(node.mDataType);
				line += "context.readValueStack<" + dataTypeString + ">(";
				line += std::to_string((int)node.mValue);
				line += ")";
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
						line += "context.readLocalVariable<" + dataTypeString + ">(";
						outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
						line += ")";
						break;
					}

					case Variable::Type::USER:
					{
						line += "context.mControlFlow->getProgram().getGlobalVariableByID(";
						outputParameter(line, node.mParameterOffset, BaseType::UINT_32);
						line += ").as<GlobalVariable>().getValue()";
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
				line += "OpcodeExecUtils::readMemory<" + dataTypeString + ">(*context.mControlFlow, ";
				outputSourceNode(line, *node.mChild[0]);
				line += ")";
				break;
			}

			case Node::Type::MEMORY_FIXED:
			{
				const bool swapBytes = (node.mValue & 0x01) != 0;
				const size_t bytes = BaseTypeHelper::getSizeOfBaseType(node.mDataType);
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
					case Opcode::Type::ARITHM_DIV:	functionCall = "OpcodeExecUtils::safeDivide";   ignoreSigned = false;  booleanResult = false;	break;
					case Opcode::Type::ARITHM_MOD:	functionCall = "OpcodeExecUtils::safeModulo";   ignoreSigned = false;  booleanResult = false;	break;
					case Opcode::Type::ARITHM_AND:	operatorString = "&";   ignoreSigned = true;   booleanResult = false;	break;
					case Opcode::Type::ARITHM_OR:	operatorString = "|";   ignoreSigned = true;   booleanResult = false;	break;
					case Opcode::Type::ARITHM_XOR:	operatorString = "^";   ignoreSigned = true;   booleanResult = false;	break;
					case Opcode::Type::ARITHM_SHL:	operatorString = "<<";  ignoreSigned = true;   booleanResult = false;  isShift = true;  break;
					case Opcode::Type::ARITHM_SHR:	operatorString = ">>";  ignoreSigned = false;  booleanResult = false;  isShift = true;  break;
					case Opcode::Type::COMPARE_EQ:	operatorString = "==";  ignoreSigned = true;   booleanResult = true;	break;
					case Opcode::Type::COMPARE_NEQ:	operatorString = "!=";  ignoreSigned = true;   booleanResult = true;	break;
					case Opcode::Type::COMPARE_LT:	operatorString = "<";   ignoreSigned = false;  booleanResult = true;	break;
					case Opcode::Type::COMPARE_LE:	operatorString = "<=";  ignoreSigned = false;  booleanResult = true;	break;
					case Opcode::Type::COMPARE_GT:	operatorString = ">";   ignoreSigned = false;  booleanResult = true;	break;
					case Opcode::Type::COMPARE_GE:	operatorString = ">=";  ignoreSigned = false;  booleanResult = true;	break;
					default:
						break;
				}

				const std::string& dataTypeString = getDataTypeString(node.mDataType, ignoreSigned);
				if (nullptr == functionCall)
				{
					line += "(";

					// Left side
					if (node.mChild[0]->mDataType != node.mDataType)
					{
						line += "(" + dataTypeString + ")";
						outputSourceNode(line, *node.mChild[0]);
					}
					else
					{
						outputSourceNode(line, *node.mChild[0]);
					}

					// Operator
					line += std::string(" ") + operatorString + " ";

					// Right side
					if (node.mChild[1]->mDataType != node.mDataType)
					{
						line += "(" + dataTypeString + ")(";
						if (isShift)
						{
							outputSourceNode(line, *node.mChild[1]);
							line += " & " + rmx::hexString(getIntegerDataTypeBits(node.mDataType) - 1, 2);		// Assuming the right side of a shift is always an integer
						}
						else
						{
							outputSourceNode(line, *node.mChild[1]);
						}
						line += ")";
					}
					else
					{
						if (isShift)
						{
							line += "(";
							outputSourceNode(line, *node.mChild[1]);
							line += " & " + rmx::hexString(getIntegerDataTypeBits(node.mDataType) - 1, 2) + ")";		// Assuming the right side of a shift is always an integer
						}
						else
						{
							outputSourceNode(line, *node.mChild[1]);
						}
					}

					line += ")";
				}
				else
				{
					line += functionCall;
					line += "<" + dataTypeString + ">((" + dataTypeString + ")";
					outputSourceNode(line, *node.mChild[0]);
					line += ", (" + dataTypeString + ")";
					outputSourceNode(line, *node.mChild[1]);
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
				outputSourceNode(line, *node.mChild[0]);
				break;
			}

			case Node::Type::TEMP_VAR:
			{
				const std::string& dataTypeString = getDataTypeString(node.mDataType);
				line += *String(0, "var%d.get<%s>()", (int)node.mValue, dataTypeString.c_str());
				break;
			}

			default:
				RMX_ERROR("Not handled", );
				break;
		}
	}

	void NativizerInternal::Assignment::outputLine(std::string& line) const
	{
		// Output this assignment into a text line
		bool closeParenthesis = false;
		outputDestNode(line, *mDest, closeParenthesis);

		if (!closeParenthesis)
			line += " = ";
		if (mDest->mDataType != mSource->mDataType)
			line += "(" + getDataTypeString(mDest->mDataType, true) + ")";

		outputSourceNode(line, *mSource);

		if (closeParenthesis)
			line += ")";
		line += ";";
	}



	void NativizerInternal::reset()
	{
		mOpcodeInfos.clear();
		mParameters.clear();
		mAssignments.clear();

		RMX_ASSERT(mNodes.size() <= 0x100, "Oops, a reallocation happened");
		mNodes.clear();
		mNodes.reserve(0x100);
	}

	uint64 NativizerInternal::buildSubtypeInfos(const Opcode* opcodes, size_t numOpcodes, MemoryAccessHandler& memoryAccessHandler)
	{
		uint64 hash = Nativizer::getStartHash();
		for (size_t opcodeIndex = 0; opcodeIndex < numOpcodes; )
		{
			OpcodeInfo& info = vectorAdd(mOpcodeInfos);
			info.mSubtypeInfo.mFirstOpcodeIndex = opcodeIndex;
			Nativizer::getOpcodeSubtypeInfo(info.mSubtypeInfo, &opcodes[opcodeIndex], numOpcodes - opcodeIndex, memoryAccessHandler);
			opcodeIndex += info.mSubtypeInfo.mConsumedOpcodes;
			hash = Nativizer::addOpcodeSubtypeInfoToHash(hash, info.mSubtypeInfo);
			info.mHash = hash;
		}
		return hash;
	}

	void NativizerInternal::buildAssignmentsFromOpcodes(const Opcode* opcodes)
	{
		int stackPosition = 0;
		for (const OpcodeInfo& info : mOpcodeInfos)
		{
			const size_t opcodeIndex = info.mSubtypeInfo.mFirstOpcodeIndex;
			const Opcode& opcode = opcodes[opcodeIndex];
			const size_t oldNumAssignments = mAssignments.size();

			switch (info.mSubtypeInfo.mSpecialType)
			{
				case Nativizer::OpcodeSubtypeInfo::SpecialType::FIXED_MEMORY_READ:
				{
					const Opcode& readMemoryOpcode = opcodes[opcodeIndex+1];
					const BaseType dataType = readMemoryOpcode.mDataType;
					const uint64 swapBytesFlag = (info.mSubtypeInfo.mSubtypeData & 0x0001);
					const bool consumeInput = (info.mSubtypeInfo.mSubtypeData & 0x0002) == 0;

					if (!consumeInput)
					{
						// First add an assignment to push the address to the stack
						const size_t parameterOffset = mParameters.add(opcodeIndex, 8, ParameterInfo::Semantics::INTEGER);
						Assignment& assignment = vectorAdd(mAssignments);
						assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition);
						assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::PARAMETER, opcode.mDataType, 0, parameterOffset);
						++stackPosition;
					}

					const size_t parameterOffset = mParameters.add(opcodeIndex, 8, ParameterInfo::Semantics::FIXED_MEMORY_ADDRESS);
					Assignment& assignment = vectorAdd(mAssignments);
					assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, dataType, stackPosition);
					assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::MEMORY_FIXED, dataType, swapBytesFlag, parameterOffset);
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
							Assignment& assignment = vectorAdd(mAssignments);
							assignment.mDest = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition);
							if (info.mSubtypeInfo.mSubtypeData & 0x8000)
							{
								// Specialized version hard-codes the constant value
								assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::CONSTANT, opcode.mDataType, opcode.mParameter);
							}
							else
							{
								// Generic version reads the constant value as a parameter
								//  -> Integer constants are always read as int64
								const size_t parameterOffset = mParameters.add(opcodeIndex, 8, ParameterInfo::Semantics::INTEGER);
								const BaseType constantDataType = BaseTypeHelper::isIntegerType(opcode.mDataType) ? BaseType::INT_64 : opcode.mDataType;
								assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::PARAMETER, constantDataType, 0, parameterOffset);
							}
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
								case Variable::Type::LOCAL:
								{
									parameterOffset = mParameters.add(opcodeIndex, 4, ParameterInfo::Semantics::LOCAL_VARIABLE);
									break;
								}
								case Variable::Type::GLOBAL:
								{
									parameterOffset = mParameters.add(opcodeIndex, 8, ParameterInfo::Semantics::GLOBAL_VARIABLE);
									break;
								}
								case Variable::Type::EXTERNAL:
								{
									parameterOffset = mParameters.add(opcodeIndex, 8, ParameterInfo::Semantics::EXTERNAL_VARIABLE, opcode.mDataType);
									break;
								}
								default:
								{
									parameterOffset = mParameters.add(opcodeIndex, 4, ParameterInfo::Semantics::INTEGER);
									break;
								}
							}

							if (opcode.mType == Opcode::Type::GET_VARIABLE_VALUE)
							{
								Assignment& assignment = vectorAdd(mAssignments);
								assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition);
								assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::VARIABLE, opcode.mDataType, (uint32)opcode.mParameter, parameterOffset);
								++stackPosition;
							}
							else
							{
								Assignment& assignment = vectorAdd(mAssignments);
								assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VARIABLE, opcode.mDataType, (uint32)opcode.mParameter, parameterOffset);
								assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							}
							break;
						}

						case Opcode::Type::READ_MEMORY:
						{
							const bool consumeInput = (opcode.mParameter == 0);
							Assignment& assignment = vectorAdd(mAssignments);
							assignment.mDest			  = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - (consumeInput ? 1 : 0));
							assignment.mSource			  = &mNodes.emplace_back(Assignment::Node::Type::MEMORY, opcode.mDataType);
							assignment.mSource->mChild[0] = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, BaseType::UINT_32, stackPosition - 1);
							if (!consumeInput)
								++stackPosition;
							break;
						}

						case Opcode::Type::WRITE_MEMORY:
						{
							{
								// Main assignment
								Assignment& assignment = vectorAdd(mAssignments);
								assignment.mDest			= &mNodes.emplace_back(Assignment::Node::Type::MEMORY, opcode.mDataType);
								assignment.mDest->mChild[0]	= &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, BaseType::UINT_32, stackPosition - 2);
								assignment.mSource			= &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							}
							{
								// Add another assignment to copy the value to the top-of-stack, where it might be expected by the next assignments
								Assignment& assignment = vectorAdd(mAssignments);
								assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 2);
								assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							}
							--stackPosition;
							break;
						}

						case Opcode::Type::CAST_VALUE:
						{
							const BaseType targetType = OpcodeHelper::getCastTargetType(opcode);
							const BaseType sourceType = OpcodeHelper::getCastSourceType(opcode);
							Assignment& assignment = vectorAdd(mAssignments);
							assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, targetType, stackPosition - 1);
							assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, sourceType, stackPosition - 1);
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
							const BaseType returnType = (opcode.mType >= Opcode::Type::COMPARE_EQ) ? BaseType::BOOL : opcode.mDataType;
							Assignment& assignment = vectorAdd(mAssignments);
							assignment.mDest			  = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, returnType, stackPosition - 2);
							assignment.mSource			  = &mNodes.emplace_back(Assignment::Node::Type::OPERATION_BINARY, opcode.mDataType, (uint64)opcode.mType);
							assignment.mSource->mChild[0] = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 2);
							assignment.mSource->mChild[1] = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							--stackPosition;
							break;
						}

						case Opcode::Type::ARITHM_NEG:
						case Opcode::Type::ARITHM_NOT:
						case Opcode::Type::ARITHM_BITNOT:
						{
							Assignment& assignment = vectorAdd(mAssignments);
							assignment.mDest			  = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							assignment.mSource			  = &mNodes.emplace_back(Assignment::Node::Type::OPERATION_UNARY, opcode.mDataType, (uint64)opcode.mType);
							assignment.mSource->mChild[0] = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, opcode.mDataType, stackPosition - 1);
							break;
						}

						default:
							break;
					}
					break;
				}
			}

			for (size_t i = oldNumAssignments; i < mAssignments.size(); ++i)
			{
				mAssignments[i].mOpcodeIndex = opcodeIndex;
			}
		}

		mFinalStackPosition = stackPosition;

		// Assign parameter offsets
		//  -> This is a separate step, now that the parameter list is filled; i.e. there won't be any more reallocations and the pointers stay valid
		for (ParameterInfo& param : mParameters.mParameters)
		{
			mOpcodeInfos[param.mOpcodeIndex].mParameter = &param;
		}
	}

	void NativizerInternal::performPostProcessing()
	{
		// Postprocessing
		//  -> Note that this whole function is only an optimization and can be skipped entirely
		//  -> It builds temp vars where a value is both read (from any source) and written (to stack) inside our nativized function

	#if NATIVIZER_OPTIMIZATION_LEVEL >= 1

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

		// This lookup is meant to mirror the stack
		//  -> It's used to track which assignment nodes consume the value written by which other assignment
		//  -> This way, we can build pairs of nodes that can be linked together:
		//      where possible, the writing node gets integrated directly as input for the reading node, without the need of having a temp var in between
		struct StackAssignment
		{
			TempVar*& operator[](int offset) { return mLookup[Nativizer::MAX_OPCODES + offset]; }
			TempVar* mLookup[Nativizer::MAX_OPCODES * 2] = { nullptr };		// Index MAX_OPCODES represents the initial stack position
		};
		StackAssignment tempVarByStackPosition;
		int lowestWrittenStackPosition = mFinalStackPosition;

		// First collect temp vars
		for (Assignment& assignment : mAssignments)
		{
			// Iterate recursively through the node and its inner child nodes
			static std::vector<Assignment::Node*> nodeStack;
			nodeStack.clear();

			const bool isValueStackWrite = (assignment.mDest->mType == Assignment::Node::Type::VALUE_STACK);

			// Push to stack in an order so that dest gets handled last
			//  -> In case this assignments writes to the value stack, then ignore dest for now, as it requires its own special handling after the stack processing
			if (!isValueStackWrite)
				nodeStack.push_back(assignment.mDest);
			nodeStack.push_back(assignment.mSource);

			while (!nodeStack.empty())
			{
				Assignment::Node& node = *nodeStack.back();
				nodeStack.pop_back();

				switch (node.mType)
				{
					case Assignment::Node::Type::VALUE_STACK:
					{
						// Get the temp var at the respective stack position
						const int readStackPosition = (int)node.mValue;
						TempVar* tempVar = tempVarByStackPosition[readStackPosition];
						if (nullptr != tempVar)
						{
							Read& read = vectorAdd(tempVar->mReads);
							read.mAssignment = &assignment;
							read.mNode = &node;
							tempVar->mPreserve = true;		// This temp var has a write and at least one read, so it must not be removed by cleanup later on
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
			if (isValueStackWrite)
			{
				const int writeStackPosition = (int)assignment.mDest->mValue;
				lowestWrittenStackPosition = std::min(lowestWrittenStackPosition, writeStackPosition);

				TempVar& tempVar = vectorAdd(tempVars);
				tempVarByStackPosition[writeStackPosition] = &tempVar;
				tempVar.mWrite = &assignment;
				tempVar.mReads.clear();
				tempVar.mPreserve = false;
				tempVar.mOutputToStack = false;
			}
		}

		// Now evaluate which of the temp vars (or assignments if you will) are required to be written to the stack
		//  -> This is everything below the final stack position
		for (int pos = lowestWrittenStackPosition; pos < mFinalStackPosition; ++pos)
		{
			TempVar* tempVar = tempVarByStackPosition[pos];
			if (nullptr != tempVar && !tempVar->mReads.empty())
			{
				// Temp var has reads, which means it gets read by an assignment, but does not get consumed by it
				//  -> We need to output an additional stack write for it
				tempVar->mOutputToStack = true;		// Note that this implies "mPreserve", as it only gets set when there's also reads
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
				if (writeStackPosition >= mFinalStackPosition)
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

			// And if the temp var needs to be written to the stack, add an additional assignment to do right that
			if (tempVar.mOutputToStack)
			{
				Assignment& assignment = vectorAdd(mAssignments);
				assignment.mDest   = &mNodes.emplace_back(Assignment::Node::Type::VALUE_STACK, write.mSource->mDataType, writeStackPosition);
				assignment.mSource = &mNodes.emplace_back(Assignment::Node::Type::TEMP_VAR, write.mSource->mDataType, nextTempVarNumber);

				// Register as a read, otherwise the optimization below could try to integrate this temp var
				Read& read = vectorAdd(tempVar.mReads);
				read.mAssignment = &assignment;
				read.mNode = assignment.mSource;
			}

			++nextTempVarNumber;
		}

		#if NATIVIZER_OPTIMIZATION_LEVEL >= 2
		{
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
				Read& read = tempVar.mReads[0];
				*read.mNode = *tempVar.mWrite->mSource;
				read.mAssignment = tempVar.mWrite;

				// Invalidate old write assignment
				tempVar.mWrite->mDest = nullptr;
			}
		}
		#endif

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

	#endif
	}

	void NativizerInternal::generateCppCode(CppWriter& writer, const ScriptFunction& function, const Opcode& firstOpcode, uint64 hash)
	{
		std::string line = "// First occurrence: ";
		line.append(function.getName().getString());
		if (firstOpcode.mLineNumber != 0)
			line = line + ", line " + std::to_string(firstOpcode.mLineNumber - function.mSourceBaseLineOffset + 1);
		writer.writeLine(line);

		writer.writeLine("static void exec_" + rmx::hexString(hash, 16, "") + "(const RuntimeOpcodeContext context)");
		writer.beginBlock();

		// Write lines
		for (const Assignment& assignment : mAssignments)
		{
			if (nullptr != assignment.mDest)	// Ignore the invalidated assignments
			{
				line.clear();
				assignment.outputLine(line);
				writer.writeLine(line);
			}
		}

		if (mFinalStackPosition != 0)
		{
			writer.writeLine("context.moveValueStack(" + std::to_string(mFinalStackPosition) + ");");
		}

		writer.endBlock();
		writer.writeEmptyLine();
	}

}
