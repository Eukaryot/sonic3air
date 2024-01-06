/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/BuiltInFunctions.h"
#include "lemon/runtime/FastStringStream.h"
#include "lemon/program/FunctionWrapper.h"
#include "lemon/program/Module.h"
#include "lemon/program/Program.h"


namespace lemon
{
	namespace builtins
	{
		template<typename T>
		T constant_array_access(uint32 id, uint32 index)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			const std::vector<ConstantArray*>& constantArrays = runtime->getProgram().getConstantArrays();
			RMX_CHECK(id < constantArrays.size(), "Invalid constant array ID " << id << " (must be below " << constantArrays.size() << ")", return 0);
			return (T)constantArrays[id]->getElement(index);
		}

		template<>
		StringRef constant_array_access(uint32 id, uint32 index)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			const std::vector<ConstantArray*>& constantArrays = runtime->getProgram().getConstantArrays();
			RMX_CHECK(id < constantArrays.size(), "Invalid constant array ID " << id << " (must be below " << constantArrays.size() << ")", return StringRef());
			return StringRef(constantArrays[id]->getElement(index));
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
	}



	BuiltInFunctions::FunctionName BuiltInFunctions::CONSTANT_ARRAY_ACCESS("#builtin_constant_array_access");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_PLUS("#builtin_string_operator_plus");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_PLUS_INT64("#builtin_string_operator_plus_int64");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_PLUS_INT64_INV("#builtin_string_operator_plus_int64_inv");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_LESS("#builtin_string_operator_less");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_LESS_OR_EQUAL("#builtin_string_operator_less_equal");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_GREATER("#builtin_string_operator_greater");
	BuiltInFunctions::FunctionName BuiltInFunctions::STRING_OPERATOR_GREATER_OR_EQUAL("#builtin_string_operator_greater_equal");


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
		module.addNativeFunction(CONSTANT_ARRAY_ACCESS.makeFlyweightString(), lemon::wrap(&builtins::constant_array_access<StringRef>), defaultFlags);

		module.addNativeFunction(STRING_OPERATOR_PLUS.makeFlyweightString(), lemon::wrap(&builtins::string_operator_plus), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_PLUS_INT64.makeFlyweightString(), lemon::wrap(&builtins::string_operator_plus_int64), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_PLUS_INT64_INV.makeFlyweightString(), lemon::wrap(&builtins::string_operator_plus_int64_inv), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_LESS.makeFlyweightString(), lemon::wrap(&builtins::string_operator_less), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_LESS_OR_EQUAL.makeFlyweightString(), lemon::wrap(&builtins::string_operator_less_or_equal), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_GREATER.makeFlyweightString(), lemon::wrap(&builtins::string_operator_greater), defaultFlags);
		module.addNativeFunction(STRING_OPERATOR_GREATER_OR_EQUAL.makeFlyweightString(), lemon::wrap(&builtins::string_operator_greater_or_equal), defaultFlags);
	}
}
