#define RMX_LIB

#include "lemon/compiler/Compiler.h"
#include "lemon/program/FunctionWrapper.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/program/Module.h"
#include "lemon/program/Program.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/StandardLibrary.h"
#include "lemon/runtime/provider/NativizedOpcodeProvider.h"

#ifdef PLATFORM_WINDOWS
	#include <direct.h>   // For _chdir
#endif

namespace lemon
{
	// Forward declaration of the nativized code lookup builder function
	extern void createNativizedCodeLookup(Nativizer::LookupDictionary& dict);
}

using namespace lemon;



void moveOutOfBinDir(const std::string& path)
{
#ifdef PLATFORM_WINDOWS
	std::vector<std::string> parts;
	for (size_t pos = 0; pos < path.length(); ++pos)
	{
		const size_t start = pos;

		// Find next separator
		while (pos < path.length() && !(path[pos] == '\\' || path[pos] == '/'))
			++pos;

		// Get part as string
		parts.emplace_back(path.substr(start, pos-start));
	}

	for (size_t index = 0; index < parts.size(); ++index)
	{
		if (parts[index] == "bin" || parts[index] == "_vstudio")
		{
			std::string wd;
			for (size_t i = 0; i < index; ++i)
				wd += parts[i] + '/';
			_chdir(wd.c_str());
			break;
		}
	}
#endif
}


void logValue(int64 value)
{
	std::cout << rmx::hexString(value, 8) << std::endl;
}

void logValueStr(int64 key)
{
	Runtime* runtime = Runtime::getActiveRuntime();
	RMX_CHECK(nullptr != runtime, "No lemon script runtime active", return);

	const FlyweightString* storedString = runtime->resolveStringByKey((uint64)key);
	RMX_CHECK(nullptr != storedString, "Unable to resolve format string", return);

	std::cout << storedString->getString() << std::endl;
}

void debugLog(uint64 stringHash)
{
	Runtime* runtime = Runtime::getActiveRuntime();
	RMX_CHECK(nullptr != runtime, "No lemon script runtime active", return);

	const FlyweightString* storedString = runtime->resolveStringByKey((uint64)stringHash);
	RMX_CHECK(nullptr != storedString, "Unable to resolve format string", return);

	std::cout << storedString->getString() << std::endl;
}

void debugLog2(AnyTypeWrapper param)
{
	if (param.mType == &PredefinedDataTypes::UINT_8)
	{
		std::cout << rmx::hexString((uint8)param.mValue, 2) << std::endl;
	}
	else if (param.mType == &PredefinedDataTypes::UINT_64)
	{
		std::cout << rmx::hexString(param.mValue, 8) << std::endl;
	}
	else if (param.mType == &PredefinedDataTypes::FLOAT)
	{
		std::cout << *reinterpret_cast<float*>(&param.mValue) << std::endl;
	}
	else if (param.mType == &PredefinedDataTypes::DOUBLE)
	{
		std::cout << *reinterpret_cast<double*>(&param.mValue) << std::endl;
	}
	else if (param.mType == &PredefinedDataTypes::STRING)
	{
		debugLog(param.mValue);
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

uint32 valueD0 = 0;
uint32 valueA0 = 0;

uint32 getterD0()
{
	return valueD0;
}

void setterD0(int64 value)
{
	valueD0 = (uint32)value;
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


class TestMemAccess : public MemoryAccessHandler
{
public:
	virtual uint8 read8(uint64 address) override
	{
		auto it = mMemory.find(address);
		return (it == mMemory.end()) ? 0 : it->second;
	}

	virtual uint16 read16(uint64 address) override
	{
		return (uint16)read8(address) + ((uint16)read8(address + 1) << 8);
	}

	virtual uint32 read32(uint64 address) override
	{
		return (uint32)read16(address) + ((uint32)read16(address + 2) << 16);
	}

	virtual uint64 read64(uint64 address) override
	{
		return (uint64)read32(address) + ((uint64)read32(address + 4) << 32);
	}

	virtual void write8(uint64 address, uint8 value) override
	{
		mMemory[address] = value;
	}

	virtual void write16(uint64 address, uint16 value) override
	{
		write8(address, (uint8)value);
		write8(address + 1, (uint8)(value >> 8));
	}

	virtual void write32(uint64 address, uint32 value) override
	{
		write16(address, (uint16)value);
		write16(address + 2, (uint16)(value >> 16));
	}

	virtual void write64(uint64 address, uint64 value) override
	{
		write32(address, (uint32)value);
		write32(address + 4, (uint32)(value >> 32));
	}

private:
	std::map<uint64, uint8> mMemory;
};


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


void doNothing()	// This function serves only as a point where to place breakpoints when debugging / testing
{
#if defined(PLATFORM_WINDOWS) && !defined(_WIN64)
	_asm nop;
#endif
}


int main(int argc, char** argv)
{
	INIT_RMX;

	// Move up until we're out of bin directory
	moveOutOfBinDir(argv[0]);

	Module module("test_module");
	UserDefinedVariable& varD0 = module.addUserDefinedVariable("D0", &PredefinedDataTypes::UINT_32);
	varD0.mGetter = getterD0;
	varD0.mSetter = setterD0;
	lemon::ExternalVariable& varA0 = module.addExternalVariable("A0", &lemon::PredefinedDataTypes::UINT_32);
	varA0.mPointer = &valueA0;
	UserDefinedVariable& var = module.addUserDefinedVariable("Log", &PredefinedDataTypes::INT_64);
	var.mSetter = logValue;
	UserDefinedVariable& var2 = module.addUserDefinedVariable("LogStr", &PredefinedDataTypes::INT_64);
	var2.mSetter = logValueStr;

	module.addNativeFunction("debugLog", lemon::wrap(&debugLog));
	module.addNativeFunction("debugLog2", lemon::wrap(&debugLog2));
	module.addNativeFunction("logFloat", lemon::wrap(&logFloat));
	module.addNativeFunction("maximum", wrap(&testFunctionA), Function::Flag::COMPILE_TIME_CONSTANT);
	module.addNativeFunction("maximum", wrap(&testFunctionB), Function::Flag::COMPILE_TIME_CONSTANT);

	SomeClass instance;
	module.addNativeFunction("sayHello", wrap(instance, &SomeClass::sayHello));
	module.addNativeFunction("incTen", wrap(instance, &SomeClass::incTen));

	StandardLibrary::registerBindings(module);

	GlobalsLookup globalsLookup;
	globalsLookup.addDefinitionsFromModule(module);

	{
		std::cout << "=== Compilation ===\r\n";
		lemon::CompileOptions options;
		Compiler compiler(module, globalsLookup, options);
		const bool compileSuccess = compiler.loadScript(L"script/main.lemon");
		if (!compileSuccess)
		{
			for (const Compiler::ErrorMessage& error : compiler.getErrors())
			{
				RMX_ERROR("Compile error in line " << error.mError.mLineNumber << ":\n" << error.mMessage, );
			}
			return 0;
		}
		std::cout << "\r\n";
	}

#if 0
	Module newModule;
	{
		std::vector<uint8> buffer;
		VectorBinarySerializer serializer(false, buffer);
		module.serialize(serializer);

		UserDefinedVariable& varD0 = newModule.addUserDefinedVariable("D0", &PredefinedDataTypes::UINT_32);
		varD0.mGetter = getterD0;
		varD0.mSetter = setterD0;
		UserDefinedVariable& var = newModule.addUserDefinedVariable("Log", &PredefinedDataTypes::INT_64);
		var.mSetter = logValue;

		newModule.addNativeFunction("max", wrap(&testFunctionA));
		newModule.addNativeFunction("max", wrap(&testFunctionB));

		VectorBinarySerializer serializer2(true, buffer);
		newModule.serialize(serializer2);

		doNothing();
	}
#endif

	TestMemAccess memoryAccess;

	Program program;
	program.addModule(module);

#if 0
	// Run nativization
	program.runNativization(module, L"source/NativizedCode.inc", memoryAccess);
#endif

	try
	{
	#if 0
		// Use nativization
		static NativizedOpcodeProvider instance(&createNativizedCodeLookup);
		program.mNativizedOpcodeProvider = instance.isValid() ? &instance : nullptr;
	#endif

		const Function* func = program.getFunctionBySignature(rmx::getMurmur2_64(String("main")) + Function::getVoidSignatureHash());
		RMX_CHECK(nullptr != func, "Function not found", RMX_REACT_THROW);

		std::cout << "=== Execution ===\r\n";
		Runtime runtime;
		runtime.setProgram(program);
		runtime.setMemoryAccessHandler(&memoryAccess);
		runtime.callFunction(*func);

		Runtime::ExecuteResult result;
		bool running = true;
		while (running)
		{
			runtime.executeSteps(result, 10);
			switch (result.mResult)
			{
				case Runtime::ExecuteResult::CALL:
				{
					if (nullptr == runtime.handleResultCall(result))
					{
						throw std::runtime_error("Call failed, probably due to an invalid function");
					}
					break;
				}

				case Runtime::ExecuteResult::RETURN:
				{
					if (runtime.getMainControlFlow().getCallStack().count == 0)
						running = false;
					break;
				}

				case Runtime::ExecuteResult::EXTERNAL_CALL:
				case Runtime::ExecuteResult::EXTERNAL_JUMP:
				{
					doNothing();
					break;
				}

				case Runtime::ExecuteResult::HALT:
				{
					running = false;
					break;
				}
			}
		}

		RMX_CHECK(runtime.getMainControlFlow().getValueStackSize() == 0, "Runtime value stack must be empty at the end", );
		doNothing();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		RMX_ERROR("Error during execution: " << e.what(), );
	}

	return 0;
}
