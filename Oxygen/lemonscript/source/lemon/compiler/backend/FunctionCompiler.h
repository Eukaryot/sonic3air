/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"


namespace lemon
{
	class ScriptFunction;
	class Node;
	class BlockNode;
	class StatementToken;
	class BinaryOperationToken;
	class GlobalsLookup;
	struct CompileOptions;

	class FunctionCompiler
	{
	friend struct OpcodeBuilder;

	public:
		FunctionCompiler(ScriptFunction& function, const CompileOptions& compileOptions, const GlobalsLookup& globalsLookup);

		void processParameters();
		void buildOpcodesForFunction(const BlockNode& blockNode);

	private:
		struct NodeContext
		{
			bool mIsLoopBlock = false;
			std::vector<uint32> mBreakLocations;
			std::vector<uint32> mContinueLocations;
		};

	private:
		Opcode& addOpcode(Opcode::Type type, int64 parameter = 0);
		Opcode& addOpcode(Opcode::Type type, BaseType dataType, int64 parameter = 0);
		Opcode& addOpcode(Opcode::Type type, const DataTypeDefinition* dataType, int64 parameter = 0);
		void addCastOpcodeIfNecessary(const DataTypeDefinition* sourceType, const DataTypeDefinition* targetType);

		void buildOpcodesFromNodes(const BlockNode& blockNode, NodeContext& context);
		void buildOpcodesForNode(const Node& node, NodeContext& context);

		void compileTokenTreeToOpcodes(const StatementToken& token, bool consumeResult = false, bool isLValue = false);
		void compileBinaryAssignmentToOpcodes(const BinaryOperationToken& bot, Opcode::Type opcodeType);
		void compileBinaryOperationToOpcodes(const BinaryOperationToken& bot, Opcode::Type opcodeType);

		void scopeBegin(int numVariables);
		void scopeEnd(int numVariables);

		void optimizeOpcodes();
		void cleanupNOPs();
		void assignOpcodeFlags();

	private:
		ScriptFunction& mFunction;
		const CompileOptions& mCompileOptions;
		const GlobalsLookup& mGlobalsLookup;
		std::vector<Opcode>& mOpcodes;
		uint32 mLineNumber = 0;		// For error output
	};

}
