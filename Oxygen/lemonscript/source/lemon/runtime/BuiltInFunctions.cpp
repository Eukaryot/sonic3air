/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/BuiltInFunctions.h"
#include "lemon/program/function/FunctionWrapper.h"
#include "lemon/program/Module.h"
#include "lemon/program/Program.h"
#include "lemon/utility/FastStringStream.h"


namespace lemon
{
	namespace builtins
	{
		// Local helper functions
		namespace
		{
			StringRef readStringVariable(const NativeFunction::Context& context, uint32 variableId)
			{
				const uint64 stringHash = context.mControlFlow.readVariableGeneric(variableId);
				const FlyweightString* str = context.mControlFlow.getRuntime().resolveStringByKey(stringHash);
				return (nullptr != str) ? StringRef(*str) : StringRef();
			}

			void writeStringVariable(const NativeFunction::Context& context, uint32 variableId, std::string_view value)
			{
				const uint64 hash = context.mControlFlow.getRuntime().addString(value);
				context.mControlFlow.writeVariableGeneric(variableId, hash);
			}
		}


		template<typename T>
		T constant_array_access(uint32 id, uint32 index)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			const std::vector<ConstantArray*>& constantArrays = runtime->getProgram().getConstantArrays();
			RMX_CHECK(id < constantArrays.size(), "Invalid constant array ID " << id << " (must be below " << constantArrays.size() << ")", return 0);
			return constantArrays[id]->getElement(index).get<T>();
		}

		template<>
		StringRef constant_array_access(uint32 id, uint32 index)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			const std::vector<ConstantArray*>& constantArrays = runtime->getProgram().getConstantArrays();
			RMX_CHECK(id < constantArrays.size(), "Invalid constant array ID " << id << " (must be below " << constantArrays.size() << ")", return StringRef());
			return StringRef(constantArrays[id]->getElement(index).get<uint64>());
		}

		size_t getArraySize(Variable& var)
		{
			RMX_ASSERT(var.getDataType()->isA<ArrayDataType>(), "Array data type is not an array");
			const ArrayDataType& arrayDataType = var.getDataType()->as<ArrayDataType>();
			return arrayDataType.mArraySize;
		}

		template<typename T>
		bool isValidArrayIndex(Variable& var, uint32 index)
		{
			RMX_ASSERT(var.getDataType()->isA<ArrayDataType>(), "Array data type is not an array");
			const ArrayDataType& arrayDataType = var.getDataType()->as<ArrayDataType>();
			RMX_ASSERT(arrayDataType.mElementType.getBytes() == sizeof(T), "Type mismatch for array");
			return (index < arrayDataType.mArraySize);
		}

		template<typename T>
		T array_bracket_getter(const NativeFunction::Context* context, uint32 variableId, uint32 index)
		{
			const Variable::Type type = (Variable::Type)(variableId >> 28);
			switch (type)
			{
				case Variable::Type::LOCAL:
				{
					LocalVariable& var = context->mControlFlow.getCurrentFunction()->getLocalVariableByID(variableId);
					if (!isValidArrayIndex<T>(var, index))
					{
						return 0;
					}

					const T* data = context->mControlFlow.accessLocalVariable<T>(var.getLocalMemoryOffset());
					return data[index];
				}

				case Variable::Type::GLOBAL:
				{
					GlobalVariable& var = context->mControlFlow.getProgram().getGlobalVariableByID(variableId).as<GlobalVariable>();
					if (!isValidArrayIndex<T>(var, index))
					{
						return 0;
					}

					const T* data = reinterpret_cast<T*>(context->mControlFlow.getRuntime().accessGlobalVariableValue(var));
					return data[index];
				}

				default:
				{
					RMX_ASSERT(false, "Unsupported type of variable");
					return 0;
				}
			}
		}

		template<>
		StringRef array_bracket_getter(const NativeFunction::Context* context, uint32 variableId, uint32 index)
		{
			return StringRef(array_bracket_getter<uint64>(context, variableId, index));
		}

		template<typename T>
		void array_bracket_setter(const NativeFunction::Context* context, uint32 variableId, uint32 index, T value)
		{
			const Variable::Type type = (Variable::Type)(variableId >> 28);
			switch (type)
			{
				case Variable::Type::LOCAL:
				{
					LocalVariable& var = context->mControlFlow.getCurrentFunction()->getLocalVariableByID(variableId);
					if (!isValidArrayIndex<T>(var, index))
					{
						return;
					}

					T* data = context->mControlFlow.accessLocalVariable<T>(var.getLocalMemoryOffset());
					data[index] = value;
					break;
				}

				case Variable::Type::GLOBAL:
				{
					GlobalVariable& var = context->mControlFlow.getProgram().getGlobalVariableByID(variableId).as<GlobalVariable>();
					if (!isValidArrayIndex<T>(var, index))
					{
						return;
					}

					T* data = reinterpret_cast<T*>(context->mControlFlow.getRuntime().accessGlobalVariableValue(var));
					data[index] = value;
					break;
				}

				default:
				{
					RMX_ASSERT(false, "Unsupported type of variable");
					break;
				}
			}
		}

		template<>
		void array_bracket_setter(const NativeFunction::Context* context, uint32 variableId, uint32 index, StringRef value)
		{
			array_bracket_setter(context, variableId, index, value.getHash());
		}

		uint32 array_length(const NativeFunction::Context* context, ArrayBaseWrapper array)
		{
			const Variable::Type type = (Variable::Type)(array.mVariableID >> 28);
			switch (type)
			{
				case Variable::Type::LOCAL:
				{
					LocalVariable& var = context->mControlFlow.getCurrentFunction()->getLocalVariableByID(array.mVariableID);
					return getArraySize(var);
				}

				case Variable::Type::GLOBAL:
				{
					GlobalVariable& var = context->mControlFlow.getProgram().getGlobalVariableByID(array.mVariableID).as<GlobalVariable>();
					return getArraySize(var);
				}

				default:
				{
					RMX_ASSERT(false, "Unsupported type of variable");
					return 0;
				}
			}
		}

		StringRef string_operator_plus(StringRef str1, StringRef str2)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			RMX_CHECK(str1.isValid(), "Unable to resolve string", return StringRef());
			RMX_CHECK(str2.isValid(), "Unable to resolve string", return StringRef());

			static detail::FastStringStream result;
			result.clear();
			result.addString(str1.getStringRef());
			result.addString(str2.getStringRef());
			return StringRef(runtime->addString(std::string_view(result.mBuffer, result.mLength)));
		}

		StringRef string_operator_plus_int64(StringRef str, int64 value)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			RMX_CHECK(str.isValid(), "Unable to resolve string", return StringRef());

			static detail::FastStringStream result;
			result.clear();
			result.addString(str.getStringRef());
			result.addDecimal(value, 0);
			return StringRef(runtime->addString(std::string_view(result.mBuffer, result.mLength)));
		}

		StringRef string_operator_plus_int64_inv(int64 value, StringRef str)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			RMX_CHECK(str.isValid(), "Unable to resolve string", return StringRef());

			static detail::FastStringStream result;
			result.clear();
			result.addDecimal(value, 0);
			result.addString(str.getStringRef());
			return StringRef(runtime->addString(std::string_view(result.mBuffer, result.mLength)));
		}

		bool string_operator_less(StringRef str1, StringRef str2)
		{
			RMX_CHECK(str1.isValid(), "Unable to resolve string", return false);
			RMX_CHECK(str2.isValid(), "Unable to resolve string", return false);
			return (str1.getString() < str2.getString());
		}

		bool string_operator_less_or_equal(StringRef str1, StringRef str2)
		{
			RMX_CHECK(str1.isValid(), "Unable to resolve string", return false);
			RMX_CHECK(str2.isValid(), "Unable to resolve string", return false);
			return (str1.getString() <= str2.getString());
		}

		bool string_operator_greater(StringRef str1, StringRef str2)
		{
			RMX_CHECK(str1.isValid(), "Unable to resolve string", return false);
			RMX_CHECK(str2.isValid(), "Unable to resolve string", return false);
			return (str1.getString() > str2.getString());
		}

		bool string_operator_greater_or_equal(StringRef str1, StringRef str2)
		{
			RMX_CHECK(str1.isValid(), "Unable to resolve string", return false);
			RMX_CHECK(str2.isValid(), "Unable to resolve string", return false);
			return (str1.getString() >= str2.getString());
		}

	/*
		// Old implementation without variable access
		uint32 builtin_string_bracket_getter(StringRef str, uint32 index)
		{
			if ((size_t)index >= str.getString().length())
				return 0;
			return (uint32)str.getString()[index];
		}
	*/
		uint32 builtin_string_bracket_getter(const NativeFunction::Context* context, uint32 variableId, uint32 index)
		{
			const StringRef stringRef = readStringVariable(*context, variableId);
			if ((size_t)index >= stringRef.getString().length())
				return 0;
			return (uint32)stringRef.getString()[index];
		}

		void builtin_string_bracket_setter(const NativeFunction::Context* context, uint32 variableId, uint32 index, uint32 value)
		{
			const StringRef stringRef = readStringVariable(*context, variableId);
			if ((size_t)index > stringRef.getString().length())
				return;

			const char character = (char)value;
			std::string newString(stringRef.getString());
			if ((size_t)index < stringRef.getString().length())
			{
				newString[index] = character;
			}
			else
			{
				newString.push_back(character);
			}
			writeStringVariable(*context, variableId, newString);
		}
	}



	BuiltInFunctions::FunctionName BuiltInFunctions::CONSTANT_ARRAY_ACCESS("#builtin_constant_array_access");
	BuiltInFunctions::FunctionName BuiltInFunctions::ARRAY_BRACKET_GETTER("#builtin_array_bracket_getter");
	BuiltInFunctions::FunctionName BuiltInFunctions::ARRAY_BRACKET_SETTER("#builtin_array_bracket_setter");
	BuiltInFunctions::FunctionName BuiltInFunctions::ARRAY_LENGTH("#builtin_array_length");

	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_PLUS("#builtin_string_operator_plus");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_PLUS_INT64("#builtin_string_operator_plus_int64");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_PLUS_INT64_INV("#builtin_string_operator_plus_int64_inv");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_LESS("#builtin_string_operator_less");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_LESS_OR_EQUAL("#builtin_string_operator_less_equal");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_GREATER("#builtin_string_operator_greater");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_GREATER_OR_EQUAL("#builtin_string_operator_greater_equal");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_BRACKET_GETTER("#builtin_string_bracket_getter");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_BRACKET_SETTER("#builtin_string_bracket_setter");


	void BuiltInFunctions::registerBuiltInFunctions(lemon::Module& module)
	{
		// Register built-in functions, which are directly referenced by the compiler
		const BitFlagSet<Function::Flag> defaultFlags(Function::Flag::ALLOW_INLINE_EXECUTION);

		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<int8>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<uint8>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<int16>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<uint16>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<int32>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<uint32>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<int64>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<uint64>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<float>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<double>), defaultFlags);
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<StringRef>), defaultFlags);

		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<int8>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<uint8>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<int16>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<uint16>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<int32>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<uint32>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<int64>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<uint64>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<float>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<double>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_getter<StringRef>), defaultFlags);

		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<int8>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<uint8>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<int16>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<uint16>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<int32>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<uint32>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<int64>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<uint64>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<float>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<double>), defaultFlags);
		module.addNativeFunction(ARRAY_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::array_bracket_setter<StringRef>), defaultFlags);

		module.addNativeFunction(ARRAY_LENGTH.makeFlyweightString(), lemon::wrap(&builtins::array_length), defaultFlags);

		module.addNativeFunction(STRING_OPERATOR_PLUS.makeFlyweightString(), lemon::wrap(&builtins::string_operator_plus), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_PLUS_INT64.makeFlyweightString(), lemon::wrap(&builtins::string_operator_plus_int64), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_PLUS_INT64_INV.makeFlyweightString(), lemon::wrap(&builtins::string_operator_plus_int64_inv), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_LESS.makeFlyweightString(), lemon::wrap(&builtins::string_operator_less), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_LESS_OR_EQUAL.makeFlyweightString(), lemon::wrap(&builtins::string_operator_less_or_equal), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_GREATER.makeFlyweightString(), lemon::wrap(&builtins::string_operator_greater), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_GREATER_OR_EQUAL.makeFlyweightString(), lemon::wrap(&builtins::string_operator_greater_or_equal), defaultFlags);
		module.addNativeFunction(STRING_BRACKET_GETTER.makeFlyweightString(), lemon::wrap(&builtins::builtin_string_bracket_getter), defaultFlags);
		module.addNativeFunction(STRING_BRACKET_SETTER.makeFlyweightString(), lemon::wrap(&builtins::builtin_string_bracket_setter), defaultFlags);
	}
}
