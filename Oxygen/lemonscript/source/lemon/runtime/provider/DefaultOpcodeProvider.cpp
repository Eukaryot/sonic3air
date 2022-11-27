/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/provider/DefaultOpcodeProvider.h"
#include "lemon/runtime/RuntimeFunction.h"
#include "lemon/runtime/RuntimeOpcodeContext.h"
#include "lemon/runtime/OpcodeExecUtils.h"
#include "lemon/program/OpcodeHelper.h"
#include "lemon/program/Program.h"


namespace lemon
{
	#define SELECT_EXEC_FUNC_BY_DATATYPE(_function_) \
	{ \
		switch (opcode.mDataType) \
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

	#define SELECT_EXEC_FUNC_BY_DATATYPE_INT(_function_) \
	{ \
		switch (opcode.mDataType) \
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

	#define SELECT_EXEC_FUNC_BY_DATATYPE_SIGNED(_function_) \
	{ \
		const BaseType baseType = BaseTypeHelper::isIntegerType(opcode.mDataType) ? BaseTypeHelper::makeIntegerSigned(opcode.mDataType) : opcode.mDataType; \
		switch (baseType) \
		{ \
			case BaseType::INT_8:		runtimeOpcode.mExecFunc = &_function_<int8>;	break; \
			case BaseType::INT_16:		runtimeOpcode.mExecFunc = &_function_<int16>;	break; \
			case BaseType::INT_32:		runtimeOpcode.mExecFunc = &_function_<int32>;	break; \
			case BaseType::INT_64:		runtimeOpcode.mExecFunc = &_function_<int64>;	break; \
			case BaseType::INT_CONST:	runtimeOpcode.mExecFunc = &_function_<int64>;	break; \
			case BaseType::FLOAT:		runtimeOpcode.mExecFunc = &_function_<float>;	break; \
			case BaseType::DOUBLE:		runtimeOpcode.mExecFunc = &_function_<double>;	break; \
			default: \
				throw std::runtime_error("Invalid opcode data type"); \
		} \
	}


	class OpcodeExec
	{
	public:
		static void exec_NOP(const RuntimeOpcodeContext context)
		{
		}

		static void exec_MOVE_STACK_positive(const RuntimeOpcodeContext context)
		{
			const int count = (int)context.getParameter<int16>();
			for (int i = 0; i < count; ++i)
				context.writeValueStack(i, 0);
			context.moveValueStack(count);
		}

		static void exec_MOVE_STACK_negative(const RuntimeOpcodeContext context)
		{
			context.moveValueStack(context.getParameter<int16>());
		}

		static void exec_MOVE_STACK_m1(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
		}

		static void exec_MOVE_VAR_STACK_positive(const RuntimeOpcodeContext context)
		{
			const int count = (int)context.getParameter<int16>();
			int64* variables = &context.mControlFlow->mLocalVariablesBuffer[context.mControlFlow->mLocalVariablesSize];
			memset(variables, 0, count * sizeof(int64));
			context.mControlFlow->mLocalVariablesSize += count;
		}

		static void exec_MOVE_VAR_STACK_negative(const RuntimeOpcodeContext context)
		{
			const int count = (int)context.getParameter<int16>();
			context.mControlFlow->mLocalVariablesSize += count;
		}

		static void exec_PUSH_CONSTANT(const RuntimeOpcodeContext context)
		{
			*context.mControlFlow->mValueStackPtr = context.getParameter<int64>();
			++context.mControlFlow->mValueStackPtr;
		}

		static void exec_GET_VARIABLE_VALUE_LOCAL(const RuntimeOpcodeContext context)
		{
			const uint32 variableId = context.getParameter<uint32>();
			*context.mControlFlow->mValueStackPtr = context.readLocalVariable<int64>(variableId);
			++context.mControlFlow->mValueStackPtr;
		}

		static void exec_GET_VARIABLE_VALUE_USER(const RuntimeOpcodeContext context)
		{
			const uint32 variableId = context.getParameter<uint32>();
			const GlobalVariable& variable = static_cast<GlobalVariable&>(context.mControlFlow->getProgram().getGlobalVariableByID(variableId));
			*context.mControlFlow->mValueStackPtr = variable.getValue();
			++context.mControlFlow->mValueStackPtr;
		}

		template<typename T>
		static void exec_GET_VARIABLE_VALUE_EXTERNAL(const RuntimeOpcodeContext context)
		{
			*context.mControlFlow->mValueStackPtr = *context.getParameter<T*>();
			++context.mControlFlow->mValueStackPtr;
		}

		static void exec_SET_VARIABLE_VALUE_LOCAL(const RuntimeOpcodeContext context)
		{
			const int64 value = *(context.mControlFlow->mValueStackPtr-1);
			const uint32 variableId = context.getParameter<uint32>();
			context.writeLocalVariable<int64>(variableId, value);
		}

		static void exec_SET_VARIABLE_VALUE_USER(const RuntimeOpcodeContext context)
		{
			const int64 value = *(context.mControlFlow->mValueStackPtr-1);
			const uint32 variableId = context.getParameter<uint32>();
			GlobalVariable& variable = static_cast<GlobalVariable&>(context.mControlFlow->getProgram().getGlobalVariableByID(variableId));
			variable.setValue(value);
		}

		template<typename T>
		static void exec_SET_VARIABLE_VALUE_EXTERNAL(const RuntimeOpcodeContext context)
		{
			const int64 value = *(context.mControlFlow->mValueStackPtr-1);
			*context.getParameter<T*>() = (T)value;
		}

		template<typename T>
		static void exec_READ_MEMORY(const RuntimeOpcodeContext context)
		{
			const uint64 address = *(context.mControlFlow->mValueStackPtr-1);
			*(context.mControlFlow->mValueStackPtr-1) = OpcodeExecUtils::readMemory<T>(*context.mControlFlow, address);
		}

		template<typename T>
		static void exec_READ_MEMORY_NOCONSUME(const RuntimeOpcodeContext context)
		{
			const uint64 address = *(context.mControlFlow->mValueStackPtr-1);
			*context.mControlFlow->mValueStackPtr = OpcodeExecUtils::readMemory<T>(*context.mControlFlow, address);
			++context.mControlFlow->mValueStackPtr;
		}

		template<typename T>
		static void exec_WRITE_MEMORY(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			const uint64 address = *context.mControlFlow->mValueStackPtr;
			OpcodeExecUtils::writeMemory<T>(*context.mControlFlow, address, (T)(*(context.mControlFlow->mValueStackPtr-1)));
		}

		template<typename T>
		static void exec_WRITE_MEMORY_EXCHANGED(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			const uint64 address = *(context.mControlFlow->mValueStackPtr - 1);
			const T value = (T)(*context.mControlFlow->mValueStackPtr);
			OpcodeExecUtils::writeMemory<T>(*context.mControlFlow, address, value);
			*(context.mControlFlow->mValueStackPtr - 1) = value;	// Replace top-of-stack (still the address) with the value
		}

		template<typename S, typename T>
		static void exec_CAST_VALUE(const RuntimeOpcodeContext context)
		{
			const S value = context.readValueStack<S>(-1);
			context.writeValueStack<T>(-1, static_cast<T>(value));
		}

		static void exec_MAKE_BOOL(const RuntimeOpcodeContext context)
		{
			*(context.mControlFlow->mValueStackPtr-1) = (*(context.mControlFlow->mValueStackPtr-1) != 0) ? 1 : 0;
		}

		template<typename T>
		static void exec_ARITHM_BINARY_ADD(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) + context.readValueStack<T>(0));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_SUB(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) - context.readValueStack<T>(0));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_MUL(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) * context.readValueStack<T>(0));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_DIV(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, OpcodeExecUtils::safeDivide(context.readValueStack<T>(-1), context.readValueStack<T>(0)));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_MOD(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, OpcodeExecUtils::safeModulo(context.readValueStack<T>(-1), context.readValueStack<T>(0)));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_AND(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) & context.readValueStack<T>(0));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_OR(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) | context.readValueStack<T>(0));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_XOR(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) ^ context.readValueStack<T>(0));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_SHL(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) << (context.readValueStack<T>(0) & (sizeof(T) * 8 - 1)));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_SHR(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<T>(-1, context.readValueStack<T>(-1) >> (context.readValueStack<T>(0) & (sizeof(T) * 8 - 1)));
		}

		template<typename T>
		static void exec_ARITHM_BINARY_CMP_EQ(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) == context.readValueStack<T>(0)) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_BINARY_CMP_NEQ(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) != context.readValueStack<T>(0)) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_BINARY_CMP_LT(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) < context.readValueStack<T>(0)) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_BINARY_CMP_LE(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) <= context.readValueStack<T>(0)) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_BINARY_CMP_GT(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) > context.readValueStack<T>(0)) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_BINARY_CMP_GE(const RuntimeOpcodeContext context)
		{
			--context.mControlFlow->mValueStackPtr;
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) >= context.readValueStack<T>(0)) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_UNARY_NEG(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, -context.readValueStack<T>(-1));
		}

		template<typename T>
		static void exec_ARITHM_UNARY_NOT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<uint64>(-1, (context.readValueStack<T>(-1) == 0) ? 1 : 0);
		}

		template<typename T>
		static void exec_ARITHM_UNARY_BITNOT(const RuntimeOpcodeContext context)
		{
			context.writeValueStack<T>(-1, ~context.readValueStack<T>(-1));
		}

		static void exec_NOT_HANDLED(const RuntimeOpcodeContext context)
		{
			throw std::runtime_error("Unhandled opcode");
		}
	};



	void DefaultOpcodeProvider::buildRuntimeOpcodeStatic(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int& outNumOpcodesConsumed, const Runtime& runtime)
	{
		const Opcode& opcode = opcodes[0];
		outNumOpcodesConsumed = 1;

		RuntimeOpcode& runtimeOpcode = buffer.addOpcode(8);
		runtimeOpcode.setParameter(opcode.mParameter);		// Default usage, parameter might be used differently depending on the opcode type
		runtimeOpcode.mExecFunc = &OpcodeExec::exec_NOT_HANDLED;
		runtimeOpcode.mOpcodeType = opcode.mType;

		switch (opcode.mType)
		{
			case Opcode::Type::NOP:
				runtimeOpcode.mExecFunc = &OpcodeExec::exec_NOP;
				break;

			case Opcode::Type::MOVE_STACK:
				runtimeOpcode.mExecFunc = (opcode.mParameter >= 0) ? OpcodeExec::exec_MOVE_STACK_positive :
										  (opcode.mParameter == -1) ? &OpcodeExec::exec_MOVE_STACK_m1 : &OpcodeExec::exec_MOVE_STACK_negative;
				break;

			case Opcode::Type::MOVE_VAR_STACK:
				runtimeOpcode.mExecFunc = (opcode.mParameter >= 0) ? &OpcodeExec::exec_MOVE_VAR_STACK_positive : &OpcodeExec::exec_MOVE_VAR_STACK_negative;
				break;

			case Opcode::Type::PUSH_CONSTANT:
				runtimeOpcode.mExecFunc = &OpcodeExec::exec_PUSH_CONSTANT;
				break;

			case Opcode::Type::GET_VARIABLE_VALUE:
			{
				const uint32 variableId = (uint32)opcode.mParameter;
				const Variable::Type type = (Variable::Type)(variableId >> 28);
				switch (type)
				{
					case Variable::Type::LOCAL:		runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_LOCAL;	break;
					case Variable::Type::USER:		runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_USER;	break;

					case Variable::Type::GLOBAL:
					{
						int64* value = const_cast<Runtime&>(runtime).accessGlobalVariableValue(runtime.getProgram().getGlobalVariableByID(variableId));
						runtimeOpcode.setParameter(value);

						switch (DataTypeHelper::getSizeOfBaseType(opcode.mDataType))
						{
							case 1:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint8>;   break;
							case 2:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint16>;  break;
							case 4:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint32>;  break;
							case 8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint64>;  break;
						}
						break;
					}

					case Variable::Type::EXTERNAL:
					{
						const ExternalVariable& variable = static_cast<ExternalVariable&>(runtime.getProgram().getGlobalVariableByID(variableId));
						runtimeOpcode.setParameter(variable.mPointer);

						switch (variable.getDataType()->getBytes())
						{
							case 1:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint8>;   break;
							case 2:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint16>;  break;
							case 4:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint32>;  break;
							case 8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_GET_VARIABLE_VALUE_EXTERNAL<uint64>;  break;
						}
						break;
					}
				}
				break;
			}

			case Opcode::Type::SET_VARIABLE_VALUE:
			{
				const uint32 variableId = (uint32)opcodes[0].mParameter;
				const Variable::Type type = (Variable::Type)(variableId >> 28);
				switch (type)
				{
					case Variable::Type::LOCAL:		runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_LOCAL;	 break;
					case Variable::Type::USER:		runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_USER;	 break;

					case Variable::Type::GLOBAL:
					{
						int64* value = const_cast<Runtime&>(runtime).accessGlobalVariableValue(runtime.getProgram().getGlobalVariableByID(variableId));
						runtimeOpcode.setParameter(value);

						switch (DataTypeHelper::getSizeOfBaseType(opcode.mDataType))
						{
							case 1:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint8>;   break;
							case 2:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint16>;  break;
							case 4:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint32>;  break;
							case 8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint64>;  break;
						}
						break;
					}

					case Variable::Type::EXTERNAL:
					{
						const ExternalVariable& variable = static_cast<ExternalVariable&>(runtime.getProgram().getGlobalVariableByID(variableId));
						runtimeOpcode.setParameter(variable.mPointer);

						switch (variable.getDataType()->getBytes())
						{
							case 1:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint8>;   break;
							case 2:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint16>;  break;
							case 4:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint32>;  break;
							case 8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_SET_VARIABLE_VALUE_EXTERNAL<uint64>;  break;
						}
						break;
					}
				}
				break;
			}

			case Opcode::Type::READ_MEMORY:
			{
				if (opcode.mParameter == 0)
				{
					SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_READ_MEMORY);
				}
				else
				{
					SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_READ_MEMORY_NOCONSUME);
				}
				break;
			}

			case Opcode::Type::WRITE_MEMORY:
			{
				if (opcode.mParameter == 0)
				{
					SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_WRITE_MEMORY);
				}
				else
				{
					SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_WRITE_MEMORY_EXCHANGED);
				}
				break;
			}

			case Opcode::Type::CAST_VALUE:
			{
				const BaseCastType baseCastType = static_cast<BaseCastType>(opcode.mParameter);
				switch (baseCastType)
				{
					// Cast down (signed or unsigned makes no difference here)
					case BaseCastType::INT_16_TO_8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint16, uint8>;   break;
					case BaseCastType::INT_32_TO_8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint32, uint8>;   break;
					case BaseCastType::INT_64_TO_8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint64, uint8>;   break;
					case BaseCastType::INT_32_TO_16: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint32, uint16>;  break;
					case BaseCastType::INT_64_TO_16: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint64, uint16>;  break;
					case BaseCastType::INT_64_TO_32: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint64, uint32>;  break;

					// Cast up (value is unsigned -> adding zeroes)
					case BaseCastType::UINT_8_TO_16:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint8, uint16>;  break;
					case BaseCastType::UINT_8_TO_32:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint8, uint32>;  break;
					case BaseCastType::UINT_8_TO_64:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint8, uint64>;  break;
					case BaseCastType::UINT_16_TO_32: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint16, uint32>; break;
					case BaseCastType::UINT_16_TO_64: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint16, uint64>; break;
					case BaseCastType::UINT_32_TO_64: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint32, uint64>; break;

					// Cast up (value is signed -> adding highest bit)
					case BaseCastType::SINT_8_TO_16:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int8, int16>;    break;
					case BaseCastType::SINT_8_TO_32:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int8, int32>;    break;
					case BaseCastType::SINT_8_TO_64:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int8, int64>;    break;
					case BaseCastType::SINT_16_TO_32: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int16, int32>;   break;
					case BaseCastType::SINT_16_TO_64: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int16, int64>;   break;
					case BaseCastType::SINT_32_TO_64: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int32, int64>;   break;

					// Integer cast to float
					case BaseCastType::UINT_8_TO_FLOAT:   runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint8,  float>;   break;
					case BaseCastType::UINT_16_TO_FLOAT:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint16, float>;   break;
					case BaseCastType::UINT_32_TO_FLOAT:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint32, float>;   break;
					case BaseCastType::UINT_64_TO_FLOAT:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint64, float>;   break;
					case BaseCastType::SINT_8_TO_FLOAT:   runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int8,   float>;   break;
					case BaseCastType::SINT_16_TO_FLOAT:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int16,  float>;   break;
					case BaseCastType::SINT_32_TO_FLOAT:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int32,  float>;   break;
					case BaseCastType::SINT_64_TO_FLOAT:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int64,  float>;   break;

					case BaseCastType::UINT_8_TO_DOUBLE:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint8,  double>;  break;
					case BaseCastType::UINT_16_TO_DOUBLE: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint16, double>;  break;
					case BaseCastType::UINT_32_TO_DOUBLE: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint32, double>;  break;
					case BaseCastType::UINT_64_TO_DOUBLE: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<uint64, double>;  break;
					case BaseCastType::SINT_8_TO_DOUBLE:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int8,   double>;  break;
					case BaseCastType::SINT_16_TO_DOUBLE: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int16,  double>;  break;
					case BaseCastType::SINT_32_TO_DOUBLE: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int32,  double>;  break;
					case BaseCastType::SINT_64_TO_DOUBLE: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<int64,  double>;  break;

					// Float cast to integer
					case BaseCastType::FLOAT_TO_UINT_8:   runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, uint8>;    break;
					case BaseCastType::FLOAT_TO_UINT_16:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, uint16>;   break;
					case BaseCastType::FLOAT_TO_UINT_32:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, uint32>;   break;
					case BaseCastType::FLOAT_TO_UINT_64:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, uint64>;   break;
					case BaseCastType::FLOAT_TO_SINT_8:   runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, int8>;     break;
					case BaseCastType::FLOAT_TO_SINT_16:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, int16>;    break;
					case BaseCastType::FLOAT_TO_SINT_32:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, int32>;    break;
					case BaseCastType::FLOAT_TO_SINT_64:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, int64>;    break;

					case BaseCastType::DOUBLE_TO_UINT_8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, uint8>;   break;
					case BaseCastType::DOUBLE_TO_UINT_16: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, uint16>;  break;
					case BaseCastType::DOUBLE_TO_UINT_32: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, uint32>;  break;
					case BaseCastType::DOUBLE_TO_UINT_64: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, uint64>;  break;
					case BaseCastType::DOUBLE_TO_SINT_8:  runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, int8>;    break;
					case BaseCastType::DOUBLE_TO_SINT_16: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, int16>;   break;
					case BaseCastType::DOUBLE_TO_SINT_32: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, int32>;   break;
					case BaseCastType::DOUBLE_TO_SINT_64: runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, int64>;   break;

					// Float cast
					case BaseCastType::FLOAT_TO_DOUBLE:   runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<float, double>;   break;
					case BaseCastType::DOUBLE_TO_FLOAT:   runtimeOpcode.mExecFunc = &OpcodeExec::exec_CAST_VALUE<double, float>;   break;

					default:
						throw std::runtime_error("Unrecognized cast type");
				}
				break;
			}

			case Opcode::Type::MAKE_BOOL:
			{
				runtimeOpcode.mExecFunc = &OpcodeExec::exec_MAKE_BOOL;
				break;
			}

			case Opcode::Type::ARITHM_ADD:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_ADD);	break;
			case Opcode::Type::ARITHM_SUB:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_SUB);	break;
			case Opcode::Type::ARITHM_MUL:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_MUL);	break;
			case Opcode::Type::ARITHM_DIV:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_DIV);	break;
			case Opcode::Type::ARITHM_MOD:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_MOD);	break;

			case Opcode::Type::ARITHM_AND:		SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_ARITHM_BINARY_AND);	break;
			case Opcode::Type::ARITHM_OR:		SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_ARITHM_BINARY_OR);	break;
			case Opcode::Type::ARITHM_XOR:		SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_ARITHM_BINARY_XOR);	break;
			case Opcode::Type::ARITHM_SHL:		SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_ARITHM_BINARY_SHL);	break;
			case Opcode::Type::ARITHM_SHR:		SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_ARITHM_BINARY_SHR);	break;

			case Opcode::Type::COMPARE_EQ:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_CMP_EQ);	break;
			case Opcode::Type::COMPARE_NEQ:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_CMP_NEQ);	break;
			case Opcode::Type::COMPARE_LT:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_CMP_LT);	break;
			case Opcode::Type::COMPARE_LE:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_CMP_LE);	break;
			case Opcode::Type::COMPARE_GT:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_CMP_GT);	break;
			case Opcode::Type::COMPARE_GE:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_BINARY_CMP_GE);	break;

			case Opcode::Type::ARITHM_NEG:		SELECT_EXEC_FUNC_BY_DATATYPE_SIGNED(OpcodeExec::exec_ARITHM_UNARY_NEG);	break;
			case Opcode::Type::ARITHM_NOT:		SELECT_EXEC_FUNC_BY_DATATYPE(OpcodeExec::exec_ARITHM_UNARY_NOT);		break;
			case Opcode::Type::ARITHM_BITNOT:	SELECT_EXEC_FUNC_BY_DATATYPE_INT(OpcodeExec::exec_ARITHM_UNARY_BITNOT);	break;

			case Opcode::Type::JUMP:
			case Opcode::Type::JUMP_CONDITIONAL:
			case Opcode::Type::CALL:
			case Opcode::Type::RETURN:
			case Opcode::Type::EXTERNAL_CALL:
			case Opcode::Type::EXTERNAL_JUMP:
			{
				if ((uint32)opcode.mDataType != 0)
					runtimeOpcode.mFlags |= RuntimeOpcode::FLAG_CALL_IS_BASE_CALL;
				runtimeOpcode.mSuccessiveHandledOpcodes = 0;
				return;
			}

			default:
				// Other opcode types are handled outside already
				break;
		}

		runtimeOpcode.mSuccessiveHandledOpcodes = (runtimeOpcode.mExecFunc == &OpcodeExec::exec_NOT_HANDLED) ? 0 : 1;
	}

	bool DefaultOpcodeProvider::buildRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int& outNumOpcodesConsumed, const Runtime& runtime)
	{
		buildRuntimeOpcodeStatic(buffer, opcodes, numOpcodesAvailable, outNumOpcodesConsumed, runtime);
		return true;
	}

	#undef SELECT_EXEC_FUNC_BY_DATATYPE
	#undef SELECT_EXEC_FUNC_BY_DATATYPE_INT
	#undef SELECT_EXEC_FUNC_BY_DATATYPE_SIGNED
}
