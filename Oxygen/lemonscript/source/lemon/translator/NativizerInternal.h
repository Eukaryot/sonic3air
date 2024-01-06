/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/translator/Nativizer.h"


namespace lemon
{

	class NativizerInternal
	{
	public:
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
			size_t mOpcodeIndex = 0;

			void outputParameter(std::string& line, int64 value, BaseType dataType = BaseType::INT_CONST, bool isPointer = false) const;
			void outputDestNode(std::string& line, const Node& node, bool& closeParenthesis) const;
			void outputSourceNode(std::string& line, const Node& node) const;
			void outputLine(std::string& line) const;
		};

	public:
		void reset();
		uint64 buildSubtypeInfos(const Opcode* opcodes, size_t numOpcodes, MemoryAccessHandler& memoryAccessHandler);
		void buildAssignmentsFromOpcodes(const Opcode* opcodes);
		void performPostProcessing();
		void generateCppCode(CppWriter& writer, const ScriptFunction& function, const Opcode& firstOpcode, uint64 hash);

	public:
		std::vector<OpcodeInfo> mOpcodeInfos;
		Parameters mParameters;
		std::vector<Assignment> mAssignments;
		std::vector<Assignment::Node> mNodes;
		int mFinalStackPosition = 0;
	};

}
