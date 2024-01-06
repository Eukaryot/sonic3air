/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/provider/OptimizedOpcodeProvider.h"
#include "lemon/runtime/RuntimeFunction.h"
#include "lemon/runtime/RuntimeOpcodeContext.h"
#include "lemon/runtime/OpcodeExecUtils.h"
#include "lemon/program/Program.h"


namespace lemon
{

	#define SELECT_EXEC_FUNC_BY_DATATYPE(_function_, _datatype_) \
	{ \
		switch (_datatype_) \
		{ \
			case BaseType::INT_8:		runtimeOpcode.mExecFunc = &_function_<int8>;	break; \
			case BaseType::INT_16:		runtimeOpcode.mExecFunc = &_function_<int16>;	break; \
			case BaseType::INT_32:		runtimeOpcode.mExecFunc = &_function_<int32>;	break; \
			case BaseType::INT_64:		runtimeOpcode.mExecFunc = &_function_<int64>;	break; \
			case BaseType::UINT_8:		runtimeOpcode.mExecFunc = &_function_<uint8>;	break; \
			case BaseType::UINT_16:		runtimeOpcode.mExecFunc = &_function_<uint16>;	break; \
			case BaseType::UINT_32:		runtimeOpcode.mExecFunc = &_function_<uint32>;	break; \
			case BaseType::UINT_64:		runtimeOpcode.mExecFunc = &_function_<uint64>;	break; \
			case BaseType::INT_CONST:	runtimeOpcode.mExecFunc = &_function_<uint64>;	break; \
			case BaseType::FLOAT:		runtimeOpcode.mExecFunc = &_function_<float>;	break; \
			case BaseType::DOUBLE:		runtimeOpcode.mExecFunc = &_function_<double>;	break; \
			default: \
				throw std::runtime_error("Invalid opcode data type"); \
		} \
	}

	#define SELECT_EXEC_FUNC_BY_DATATYPE_INT(_function_, _datatype_) \
	{ \
		switch (_datatype_) \
		{ \
			case BaseType::INT_8:		runtimeOpcode.mExecFunc = &_function_<int8>;	break; \
			case BaseType::INT_16:		runtimeOpcode.mExecFunc = &_function_<int16>;	break; \
			case BaseType::INT_32:		runtimeOpcode.mExecFunc = &_function_<int32>;	break; \
			case BaseType::INT_64:		runtimeOpcode.mExecFunc = &_function_<int64>;	break; \
			case BaseType::UINT_8:		runtimeOpcode.mExecFunc = &_function_<uint8>;	break; \
			case BaseType::UINT_16:		runtimeOpcode.mExecFunc = &_function_<uint16>;	break; \
			case BaseType::UINT_32:		runtimeOpcode.mExecFunc = &_function_<uint32>;	break; \
			case BaseType::UINT_64:		runtimeOpcode.mExecFunc = &_function_<uint64>;	break; \
			case BaseType::INT_CONST:	runtimeOpcode.mExecFunc = &_function_<uint64>;	break; \
			default: \
				throw std::runtime_error("Invalid opcode data type"); \
		} \
	}


	class OptimizedOpcodeExec
	{
	public:
		static void exec_OPT_SET_VARIABLE_VALUE_LOCAL_DISCARD(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			const int64 value = *context.mControlFlow->mValueStackPtr;
			const uint32 variableId = context.getParameter<uint32>();
			context.writeLocalVariable<int64>(variableId, value);
		}

		static void exec_OPT_SET_VARIABLE_VALUE_USER_DISCARD(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			const int64 value = *context.mControlFlow->mValueStackPtr;
			const uint32 variableId = context.getParameter<uint32>();
			GlobalVariable& variable = static_cast<GlobalVariable&>(context.mControlFlow->getProgram().getGlobalVariableByID(variableId));
			variable.setValue(value);
		}

		template<typename T>
		static void exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			const int64 value = *context.mControlFlow->mValueStackPtr;
			T* pointer = context.mOpcode->getParameter<T*>();
			*pointer = (T)value;
		}

		template<typename T>
		static void exec_OPT_WRITE_MEMORY_DISCARD(const RuntimeOpcodeContext context)
		{
			context.mControlFlow->mValueStackPtr -= 2;
			const uint64 address = *(context.mControlFlow->mValueStackPtr+1);
			OpcodeExecUtils::writeMemory<T>(*context.mControlFlow, address, (T)(*(context.mControlFlow->mValueStackPtr)));
		}

		template<typename T>
		static void exec_OPT_WRITE_MEMORY_EXCHANGED_DISCARD(const RuntimeOpcodeContext context)
		{
			context.mControlFlow->mValueStackPtr -= 2;
			const uint64 address = *(context.mControlFlow->mValueStackPtr);
			OpcodeExecUtils::writeMemory<T>(*context.mControlFlow, address, (T)(*(context.mControlFlow->mValueStackPtr+1)));
		}

		template<typename T>
		static void exec_OPT_READ_MEMORY_FIXED_ADDR(const RuntimeOpcodeContext context)
		{
			const uint64 address = context.getParameter<uint64>();
			*context.mControlFlow->mValueStackPtr = OpcodeExecUtils::readMemory<T>(*context.mControlFlow, address);
			++context.mControlFlow->mValueStackPtr;
		}

		template<typename T>
		static void exec_OPT_READ_MEMORY_FIXED_ADDR_DIRECT(const RuntimeOpcodeContext context)
		{
			const uint8* pointer = context.getParameter<uint8*>();
			*context.mControlFlow->mValueStackPtr = *(T*)pointer;
			++context.mControlFlow->mValueStackPtr;
		}

		template<typename T>
		static void exec_OPT_READ_MEMORY_FIXED_ADDR_DIRECT_SWAP(const RuntimeOpcodeContext context)
		{
			const uint8* pointer = context.getParameter<uint8*>();
			*context.mControlFlow->mValueStackPtr = rmx::swapBytes(*(T*)pointer);
			++context.mControlFlow->mValueStackPtr;
		}

		template<typename T>
		static void exec_OPT_WRITE_MEMORY_FIXED_ADDR(const RuntimeOpcodeContext context)
		{
			const uint64 address = context.getParameter<uint64>();
			OpcodeExecUtils::writeMemory<T>(*context.mControlFlow, address, (T)(*(context.mControlFlow->mValueStackPtr-1)));
		}

		template<typename T>
		static void exec_OPT_WRITE_MEMORY_FIXED_ADDR_DIRECT(const RuntimeOpcodeContext context)
		{
			uint8* pointer = context.getParameter<uint8*>();
			*(T*)pointer = (T)(*(context.mControlFlow->mValueStackPtr-1));
		}

		template<typename T>
		static void exec_OPT_WRITE_MEMORY_FIXED_ADDR_DIRECT_SWAP(const RuntimeOpcodeContext context)
		{
			uint8* pointer = context.getParameter<uint8*>();
			*(T*)pointer = rmx::swapBytes((T)(*(context.mControlFlow->mValueStackPtr-1)));
		}

		template<typename T>
		static void exec_OPT_ADD_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) + context.mOpcode->getParameter<T>());
		}

		template<typename T>
		static void exec_OPT_SUB_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) - context.mOpcode->getParameter<T>());
		}

		template<typename T>
		static void exec_OPT_MUL_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) * context.mOpcode->getParameter<T>());
		}

		template<typename T>
		static void exec_OPT_DIV_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, OpcodeExecUtils::safeDivide(context.readValueStack<T>(-1), context.mOpcode->getParameter<T>()));
		}

		template<typename T>
		static void exec_OPT_MOD_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, OpcodeExecUtils::safeModulo(context.readValueStack<T>(-1), context.mOpcode->getParameter<T>()));
		}

		template<typename T>
		static void exec_OPT_AND_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) & context.mOpcode->getParameter<T>());
		}

		template<typename T>
		static void exec_OPT_OR_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) | context.mOpcode->getParameter<T>());
		}

		template<typename T>
		static void exec_OPT_XOR_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) ^ context.mOpcode->getParameter<T>());
		}

		template<typename T>
		static void exec_OPT_SHL_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) << (context.mOpcode->getParameter<T>() & (sizeof(T) * 8 - 1)));
		}

		template<typename T>
		static void exec_OPT_SHR_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) >> (context.mOpcode->getParameter<T>() & (sizeof(T) * 8 - 1)));
		}

		template<typename T>
		static void exec_OPT_CMP_EQ_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) == context.mOpcode->getParameter<T>()) ? 1 : 0);
		}

		template<typename T>
		static void exec_OPT_CMP_NEQ_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) != context.mOpcode->getParameter<T>()) ? 1 : 0);
		}

		template<typename T>
		static void exec_OPT_CMP_LT_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) < context.mOpcode->getParameter<T>()) ? 1 : 0);
		}

		template<typename T>
		static void exec_OPT_CMP_LE_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) <= context.mOpcode->getParameter<T>()) ? 1 : 0);
		}

		template<typename T>
		static void exec_OPT_CMP_GT_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) > context.mOpcode->getParameter<T>()) ? 1 : 0);
		}

		template<typename T>
		static void exec_OPT_CMP_GE_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) >= context.mOpcode->getParameter<T>()) ? 1 : 0);
		}

		template<typename T>
		static void exec_OPT_EXTERNAL_ADD_CONSTANT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(0, *context.mOpcode->getParameter<T*>() + context.mOpcode->getParameter<T>(8));
			++context.mControlFlow->mValueStackPtr;
		}
	};


	bool OptimizedOpcodeProvider::buildRuntimeOpcodeStatic(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime)
	{
		if (numOpcodesAvailable >= 2)
		{
			// Merge: Binary operation with an external variable and a constant value
			if (opcodes[0].mType == Opcode::Type::GET_VARIABLE_VALUE && (Variable::Type)((uint32)(opcodes[0].mParameter) >> 28) == Variable::Type::EXTERNAL)
			{
				if (opcodes[1].mType == Opcode::Type::PUSH_CONSTANT)
				{
					if (opcodes[2].mType == Opcode::Type::ARITHM_ADD && opcodes[2].mDataType == opcodes[0].mDataType)
					{
						RuntimeOpcode& runtimeOpcode = buffer.addOpcode(16);
						SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_EXTERNAL_ADD_CONSTANT, opcodes[0].mDataType);

						const uint32 variableId = (uint32)opcodes[0].mParameter;
						const ExternalVariable& variable = static_cast<ExternalVariable&>(runtime.getProgram().getGlobalVariableByID(variableId));
						runtimeOpcode.setParameter(variable.mAccessor());
						runtimeOpcode.setParameter(opcodes[1].mParameter, 8);
						outNumOpcodesConsumed = 3;
						return true;
					}
				}
			}

			// Merge: Binary operation with a constant value
			if (opcodes[0].mType == Opcode::Type::PUSH_CONSTANT)
			{
				if ((opcodes[1].mType >= Opcode::Type::ARITHM_ADD && opcodes[1].mType <= Opcode::Type::ARITHM_SHR) ||
					(opcodes[1].mType >= Opcode::Type::COMPARE_EQ && opcodes[1].mType <= Opcode::Type::COMPARE_GE))
				{
					RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
					switch (opcodes[1].mType)
					{
						case Opcode::Type::ARITHM_ADD:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_ADD_CONSTANT,	 opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_SUB:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_SUB_CONSTANT,	 opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_MUL:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_MUL_CONSTANT,	 opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_DIV:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_DIV_CONSTANT,	 opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_MOD:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_MOD_CONSTANT,	 opcodes[1].mDataType);	break;

						case Opcode::Type::ARITHM_AND:	SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_AND_CONSTANT, opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_OR:	SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_OR_CONSTANT,	 opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_XOR:	SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_XOR_CONSTANT, opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_SHL:	SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_SHL_CONSTANT, opcodes[1].mDataType);	break;
						case Opcode::Type::ARITHM_SHR:	SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_SHR_CONSTANT, opcodes[1].mDataType);	break;

						case Opcode::Type::COMPARE_EQ:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_CMP_EQ_CONSTANT,  opcodes[1].mDataType);	break;
						case Opcode::Type::COMPARE_NEQ:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_CMP_NEQ_CONSTANT, opcodes[1].mDataType);	break;
						case Opcode::Type::COMPARE_LT:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_CMP_LT_CONSTANT,  opcodes[1].mDataType);	break;
						case Opcode::Type::COMPARE_LE:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_CMP_LE_CONSTANT,  opcodes[1].mDataType);	break;
						case Opcode::Type::COMPARE_GT:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_CMP_GT_CONSTANT,  opcodes[1].mDataType);	break;
						case Opcode::Type::COMPARE_GE:	SELECT_EXEC_FUNC_BY_DATATYPE(OptimizedOpcodeExec::exec_OPT_CMP_GE_CONSTANT,  opcodes[1].mDataType);	break;
						default:
							return false;
					}

					runtimeOpcode.setParameter(opcodes[0].mParameter);
					outNumOpcodesConsumed = 2;
					return true;
				}
			}

			// Merge: Set variable value and discard its result
			if (opcodes[0].mType == Opcode::Type::SET_VARIABLE_VALUE)
			{
				if (opcodes[1].mType == Opcode::Type::MOVE_STACK && opcodes[1].mParameter == -1)
				{
					const uint32 variableId = (uint32)opcodes[0].mParameter;
					const Variable::Type type = (Variable::Type)(variableId >> 28);

					RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
					runtimeOpcode.setParameter(variableId);

					switch (type)
					{
						case Variable::Type::LOCAL:		runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_LOCAL_DISCARD;	break;
						case Variable::Type::USER:		runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_USER_DISCARD;	break;

						case Variable::Type::GLOBAL:
						{
							int64* value = const_cast<Runtime&>(runtime).accessGlobalVariableValue(runtime.getProgram().getGlobalVariableByID(variableId));
							runtimeOpcode.setParameter(value);

							switch (DataTypeHelper::getSizeOfBaseType(opcodes[0].mDataType))
							{
								case 1:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint8>;   break;
								case 2:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint16>;  break;
								case 4:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint32>;  break;
								case 8:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint64>;  break;
							}
							break;
						}

						case Variable::Type::EXTERNAL:
						{
							const ExternalVariable& variable = static_cast<ExternalVariable&>(runtime.getProgram().getGlobalVariableByID(variableId));
							runtimeOpcode.setParameter(variable.mAccessor());

							switch (variable.getDataType()->getBytes())
							{
								case 1:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint8>;   break;
								case 2:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint16>;  break;
								case 4:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint32>;  break;
								case 8:  runtimeOpcode.mExecFunc = &OptimizedOpcodeExec::exec_OPT_SET_VARIABLE_VALUE_EXTERNAL_DISCARD<uint64>;  break;
							}
							break;
						}
					}
					outNumOpcodesConsumed = 2;
					return true;
				}
			}

			// Merge: Write memory and discard its result
			if (opcodes[0].mType == Opcode::Type::WRITE_MEMORY && opcodes[0].mParameter == 0)
			{
				if (opcodes[1].mType == Opcode::Type::MOVE_STACK && opcodes[1].mParameter == -1)
				{
					RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
					if (opcodes[0].mParameter == 0)
					{
						SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_WRITE_MEMORY_DISCARD, opcodes[0].mDataType);
					}
					else
					{
						SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_WRITE_MEMORY_EXCHANGED_DISCARD, opcodes[0].mDataType);
					}
					outNumOpcodesConsumed = 2;
					return true;
				}
			}

			// Merge: Read memory at a fixed address
			if (opcodes[0].mType == Opcode::Type::PUSH_CONSTANT)
			{
				if (opcodes[1].mType == Opcode::Type::READ_MEMORY && opcodes[1].mParameter == 0)	// TODO: Also support the "no consume" variant (parameter == 1)
				{
					uint64 address = opcodes[0].mParameter;
					MemoryAccessHandler::SpecializationResult result;
					runtime.getMemoryAccessHandler()->getDirectAccessSpecialization(result, address, DataTypeHelper::getSizeOfBaseType(opcodes[1].mDataType), false);
					if (result.mResult == MemoryAccessHandler::SpecializationResult::Result::HAS_SPECIALIZATION)
					{
						RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
						if (result.mSwapBytes)
						{
							SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_READ_MEMORY_FIXED_ADDR_DIRECT_SWAP, opcodes[1].mDataType);
						}
						else
						{
							SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_READ_MEMORY_FIXED_ADDR_DIRECT, opcodes[1].mDataType);
						}
						runtimeOpcode.setParameter(result.mDirectAccessPointer);
					}
					else
					{
						RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
						SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_READ_MEMORY_FIXED_ADDR, opcodes[1].mDataType);
						runtimeOpcode.setParameter(address);
					}
					outNumOpcodesConsumed = 2;
					return true;
				}
			}

			// Merge: Write memory at a fixed address
			if (opcodes[0].mType == Opcode::Type::PUSH_CONSTANT)
			{
				if (opcodes[1].mType == Opcode::Type::WRITE_MEMORY && opcodes[1].mParameter == 0)
				{
					uint64 address = opcodes[0].mParameter;
					MemoryAccessHandler::SpecializationResult result;
					runtime.getMemoryAccessHandler()->getDirectAccessSpecialization(result, address, DataTypeHelper::getSizeOfBaseType(opcodes[1].mDataType), true);
					if (result.mResult == MemoryAccessHandler::SpecializationResult::Result::HAS_SPECIALIZATION)
					{
						RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
						if (result.mSwapBytes)
						{
							SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_WRITE_MEMORY_FIXED_ADDR_DIRECT_SWAP, opcodes[1].mDataType);
						}
						else
						{
							SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_WRITE_MEMORY_FIXED_ADDR_DIRECT, opcodes[1].mDataType);
						}
						runtimeOpcode.setParameter(result.mDirectAccessPointer);
					}
					else
					{
						RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
						SELECT_EXEC_FUNC_BY_DATATYPE_INT(OptimizedOpcodeExec::exec_OPT_WRITE_MEMORY_FIXED_ADDR, opcodes[1].mDataType);
						runtimeOpcode.setParameter(address);
					}
					outNumOpcodesConsumed = 2;
					return true;
				}
			}
		}

		return false;
	}

	bool OptimizedOpcodeProvider::buildRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime)
	{
		return buildRuntimeOpcodeStatic(buffer, opcodes, numOpcodesAvailable, firstOpcodeIndex, outNumOpcodesConsumed, runtime);
	}

	#undef SELECT_EXEC_FUNC_BY_DATATYPE
	#undef SELECT_EXEC_FUNC_BY_DATATYPE_INT
}
