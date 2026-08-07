/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "TestBindings.h"

#include "lemon/runtime/ControlFlow.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/program/function/FunctionWrapper.h"
#include "lemon/program/Module.h"
#include "lemon/program/ModuleBindingsBuilder.h"
#include "lemon/program/Program.h"
#include "lemon/program/Variable.h"


namespace lemon
{
	struct ObjectHandleWrapper
	{
		uint32 mContent;

		static inline const CustomDataType* mObjectHandleDataType = nullptr;
	};

	ObjectHandleWrapper makeObjectHandle(uint32 value)
	{
		return ObjectHandleWrapper { value };
	}

	ObjectHandleWrapper increaseObjectHandle(ObjectHandleWrapper value)
	{
		return ObjectHandleWrapper { value.mContent + 1 };
	}


	const ReferenceDataType* mVarRefDataType_u8 = nullptr;


	namespace traits
	{
		template<> const DataTypeDefinition* getDataType<ObjectHandleWrapper>()  { return ObjectHandleWrapper::mObjectHandleDataType; }
		template<> const DataTypeDefinition* getDataType<TReferenceWrapper<uint8>>()  { return mVarRefDataType_u8; }
	}

	namespace internal
	{
		template<>
		struct StackHandler<ObjectHandleWrapper>
		{
			static void pushStack(ObjectHandleWrapper value, const NativeFunction::Context context)
			{
				context.mControlFlow.pushValueStack(value.mContent);
			}

			static ObjectHandleWrapper popStack(const NativeFunction::Context context)
			{
				return ObjectHandleWrapper { context.mControlFlow.popValueStack<uint32>() };
			}
		};
	}


	namespace functions
	{
		uint32 valueD0 = 0;
		uint32 valueA0 = 0;

		void getterD0(ControlFlow& controlFlow)
		{
			controlFlow.pushValueStack(valueD0);
		}

		void setterD0(ControlFlow& controlFlow)
		{
			valueD0 = controlFlow.readValueStack<uint32>(-1);	// Read the value, but don't remove it from stack
		}

		int64* accessA0()
		{
			return (int64*)&valueA0;
		}

		void logValue(ControlFlow& controlFlow)
		{
			const int64 value = controlFlow.readValueStack<int64>(-1);		// Read the value, but don't remove it from stack
			std::cout << rmx::hexString(value, 8) << std::endl;
		}

		void logValueStr(ControlFlow& controlFlow)
		{
			const uint64 key = controlFlow.readValueStack<uint64>(-1);		// Read the value, but don't remove it from stack

			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_CHECK(nullptr != runtime, "No lemon script runtime active", return);

			const FlyweightString* storedString = runtime->resolveStringByKey(key);
			RMX_CHECK(nullptr != storedString, "Unable to resolve format string", return);

			std::cout << storedString->getString() << std::endl;
		}

		void debugLog(AnyTypeWrapper param)
		{
			if (param.mType == &PredefinedDataTypes::UINT_8 || param.mType == &PredefinedDataTypes::INT_8)
			{
				std::cout << rmx::hexString(param.mValue.get<uint8>(), 2) << std::endl;
			}
			else if (param.mType == &PredefinedDataTypes::UINT_16 || param.mType == &PredefinedDataTypes::INT_16)
			{
				std::cout << rmx::hexString(param.mValue.get<uint16>(), 4) << std::endl;
			}
			else if (param.mType == &PredefinedDataTypes::UINT_32 || param.mType == &PredefinedDataTypes::INT_32)
			{
				std::cout << rmx::hexString(param.mValue.get<uint32>(), 8) << std::endl;
			}
			else if (param.mType == &PredefinedDataTypes::UINT_64 || param.mType == &PredefinedDataTypes::INT_64 || param.mType == &PredefinedDataTypes::CONST_INT)
			{
				std::cout << rmx::hexString(param.mValue.get<uint64>(), 8) << std::endl;
			}
			else if (param.mType == &PredefinedDataTypes::FLOAT)
			{
				std::cout << param.mValue.get<float>() << std::endl;
			}
			else if (param.mType == &PredefinedDataTypes::DOUBLE)
			{
				std::cout << param.mValue.get<double>() << std::endl;
			}
			else if (param.mType == &PredefinedDataTypes::STRING)
			{
				Runtime* runtime = Runtime::getActiveRuntime();
				RMX_CHECK(nullptr != runtime, "No lemon script runtime active", return);
				const FlyweightString* storedString = runtime->resolveStringByKey(param.mValue.get<uint64>());
				RMX_CHECK(nullptr != storedString, "Unable to resolve format string", return);
				std::cout << storedString->getString() << std::endl;
			}
			else if (param.mType == ObjectHandleWrapper::mObjectHandleDataType)
			{
				std::cout << "[ObjectHandle: " << param.mValue.get<uint32>() << "]" << std::endl;
			}
			else
			{
				std::cout << "Oops, type support not implemented yet" << std::endl;
			}
		}

		void logFloat(float value)
		{
			std::cout << value << std::endl;
		}

		int8 testFunctionA(int8 a, int8 b)
		{
			std::cout << "Test function A called" << std::endl;
			return std::max(a, b);
		}

		uint8 testFunctionB(uint8 a, uint8 b)
		{
			std::cout << "Test function B called" << std::endl;
			return std::max(a, b);
		}

		void increase_u8(const NativeFunction::Context* context, TReferenceWrapper<uint8> ref)
		{
			uint8 value = (uint8)context->mControlFlow.readVariableGeneric(ref.mVariableID);
			++value;
			context->mControlFlow.writeVariableGeneric(ref.mVariableID, (int64)value);
		}

		class SomeClass
		{
		public:
			void sayHello()
			{
				std::cout << "Hello World\n";
			}

			uint8 incTen(uint8 input)
			{
				return input + 10;
			}
		};
	}


	void TestBindings::registerBindings(lemon::Module& module)
	{
		lemon::ModuleBindingsBuilder builder(module);
		const BitFlagSet<Function::Flag> defaultFlags(Function::Flag::ALLOW_INLINE_EXECUTION);
		const BitFlagSet<Function::Flag> compileTimeConstant(Function::Flag::ALLOW_INLINE_EXECUTION, Function::Flag::COMPILE_TIME_CONSTANT);

		// Variables
		UserDefinedVariable& varD0 = module.addUserDefinedVariable("D0", &lemon::PredefinedDataTypes::UINT_32);
		varD0.mGetter = functions::getterD0;
		varD0.mSetter = functions::setterD0;
		lemon::ExternalVariable& varA0 = module.addExternalVariable("A0", &lemon::PredefinedDataTypes::UINT_32, std::bind(functions::accessA0));

		// Log setters
		UserDefinedVariable& var = module.addUserDefinedVariable("Log", &lemon::PredefinedDataTypes::INT_64);
		var.mSetter = functions::logValue;
		UserDefinedVariable& var2 = module.addUserDefinedVariable("LogStr", &lemon::PredefinedDataTypes::INT_64);
		var2.mSetter = functions::logValueStr;

		// Functions (bound to global C++ functions)
		builder.addNativeFunction("debugLog", lemon::wrap(&functions::debugLog), defaultFlags);
		builder.addNativeFunction("logFloat", lemon::wrap(&functions::logFloat), defaultFlags);
		builder.addNativeFunction("maximum", lemon::wrap(&functions::testFunctionA), compileTimeConstant);
		builder.addNativeFunction("maximum", lemon::wrap(&functions::testFunctionB), compileTimeConstant);

		mVarRefDataType_u8 = &module.addReferenceDataType(lemon::PredefinedDataTypes::UINT_8);
		builder.addNativeFunction("increase_u8", lemon::wrap(&functions::increase_u8), defaultFlags);

		// Functions (bound to C++ class methods)
		functions::SomeClass instance;
		module.addNativeFunction("sayHello", lemon::wrap(instance, &functions::SomeClass::sayHello));
		module.addNativeFunction("incTen", lemon::wrap(instance, &functions::SomeClass::incTen));

		// Object handle & methods
		ObjectHandleWrapper::mObjectHandleDataType = &module.addCustomDataType("ObjectHandle", lemon::BaseType::UINT_32);
		module.addNativeFunction("makeObjectHandle", lemon::wrap(&makeObjectHandle));
		module.addNativeFunction("increaseObjectHandle", lemon::wrap(&increaseObjectHandle));
		module.addNativeMethod("ObjectHandle", "increase", lemon::wrap(&increaseObjectHandle));
	}
}
