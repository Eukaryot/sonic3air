/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#define RMX_LIB

#include "TestBindings.h"

#include "lemon/compiler/Compiler.h"
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


void doNothing()	// This function serves only as a point where to place breakpoints when debugging / testing
{
#if defined(PLATFORM_WINDOWS) && !defined(_WIN64)
	_asm nop;
#endif
}


struct RuntimeExecuteConnector : public lemon::Runtime::ExecuteConnector
{
	Runtime& mRuntime;
	bool mStopped = false;

	inline RuntimeExecuteConnector(Runtime& runtime) : mRuntime(runtime) {}

	bool handleCall(const lemon::Function* func, uint64 callTarget) override
	{
		if (nullptr == func)
		{
			throw std::runtime_error("Call failed, probably due to an invalid function");
		}
		return true;
	}

	bool handleReturn() override
	{
		if (mRuntime.getMainControlFlow().getCallStack().count == 0)
		{
			mStopped = true;
			return false;
		}
		return true;
	}

	bool handleExternalCall(uint64 address) override
	{
		return true;
	}

	bool handleExternalJump(uint64 address) override
	{
		return true;
	}
};


int main(int argc, char** argv)
{
	INIT_RMX;

	// Move up until we're out of bin directory
	moveOutOfBinDir(argv[0]);

	Module module("test_module");
	GlobalsLookup globalsLookup;
	module.startCompiling(globalsLookup);

	StandardLibrary::registerBindings(module);
	TestBindings::registerBindings(module);
	globalsLookup.addDefinitionsFromModule(module);

	{
		std::cout << "=== Compilation ===\r\n";
		lemon::CompileOptions options;
		options.mOutputOpcodesAsText = L"function_opcodes.txt";
		Compiler compiler(module, globalsLookup, options);
		const bool compileSuccess = compiler.loadScript(L"script/mainscript.lemon");
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
	program.runNativization(module, L"source/test/NativizedCode.inc", memoryAccess);
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

		RuntimeExecuteConnector connector(runtime);
		while (!connector.mStopped)
		{
			runtime.executeSteps(connector, 10, 0);

			if (connector.mResult == Runtime::ExecuteResult::Result::HALT)
			{
				connector.mStopped = true;
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
