/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/runtime/RuntimeOpcode.h"
#include <unordered_map>


namespace lemon
{
	class CppWriter;
	class MemoryAccessHandler;
	class Module;
	class Program;
	class ScriptFunction;
	struct Opcode;

	class Nativizer
	{
	public:
		static const constexpr size_t MIN_OPCODES = 2;
		static const constexpr size_t MAX_OPCODES = 32;

		struct OpcodeSubtypeInfo
		{
			enum class SpecialType : uint8
			{
				NONE,
				FIXED_MEMORY_READ	= 0x80	// Intentially using higher values than the opcode types
			};

			size_t mFirstOpcodeIndex = 0;
			size_t mConsumedOpcodes = 0;
			Opcode::Type mType = Opcode::Type::NOP;
			SpecialType mSpecialType = SpecialType::NONE;
			uint32 mSubtypeData = 0;
		};

		struct LookupEntry
		{
			struct ParameterInfo
			{
				enum class Semantics
				{
					INTEGER,
					GLOBAL_VARIABLE,
					EXTERNAL_VARIABLE,
					FIXED_MEMORY_ADDRESS
				};

				uint16 mOffset = 0;
				uint8 mOpcodeIndex = 0;
				Semantics mSemantics = Semantics::INTEGER;
			};

			ExecFunc mExecFunc = nullptr;
			size_t mParameterStart = 0;

			inline LookupEntry() {}
			inline explicit LookupEntry(ExecFunc execFunc, size_t parameterStart) : mExecFunc(execFunc), mParameterStart(parameterStart) {}
		};

		struct CompactFunctionEntry
		{
			uint64 mHash;
			void(*mFunctionPointer)(const RuntimeOpcodeContext);
			size_t mParameterStart;
		};

		struct LookupDictionary
		{
			void addEmptyEntries(const uint64* hashes, size_t numHashes);
			void loadFunctions(const CompactFunctionEntry* entries, size_t numEntries);
			void loadParameterInfo(const uint8* data, size_t count);

			std::unordered_map<uint64, LookupEntry> mEntries;
			std::vector<LookupEntry::ParameterInfo> mParameterData;
		};

	public:
		static void getOpcodeSubtypeInfo(OpcodeSubtypeInfo& outInfo, const Opcode* opcodes, size_t numOpcodesAvailable, MemoryAccessHandler& memoryAccessHandler);
		static uint64 getStartHash();
		static uint64 addOpcodeSubtypeInfoToHash(uint64 hash, const OpcodeSubtypeInfo& info);

	public:
		void build(String& output, const Module& module, const Program& program, MemoryAccessHandler& memoryAccessHandler);

	private:
		void buildFunction(CppWriter& writer, const ScriptFunction& function);
		size_t processOpcodes(CppWriter& writer, const Opcode* opcodes, size_t numOpcodes, const ScriptFunction& function);
		size_t collectNativizableOpcodes(const Opcode* opcodes, size_t numOpcodes);

	private:
		const Module* mModule = nullptr;
		const Program* mProgram = nullptr;
		MemoryAccessHandler* mMemoryAccessHandler = nullptr;
		LookupDictionary mBuiltDictionary;
	};

}
