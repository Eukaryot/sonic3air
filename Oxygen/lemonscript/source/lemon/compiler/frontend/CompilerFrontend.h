/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/frontend/TokenProcessing.h"


namespace lemon
{
	class Module;
	class GlobalsLookup;
	class ScriptFunction;
	class LocalVariable;
	class ConstantArray;
	class Node;
	class BlockNode;
	class UndefinedNode;
	class FunctionNode;
	class PragmaNode;
	class TokenList;
	struct LineNumberTranslation;
	struct NodesIterator;


	class CompilerFrontend
	{
	public:
		CompilerFrontend(Module& module, GlobalsLookup& globalsLookup, CompileOptions& compileOptions, const LineNumberTranslation& lineNumberTranslation, TokenProcessing& tokenProcessing, std::vector<FunctionNode*>& functionNodes);

		void runCompilerFrontend(BlockNode& outRootNode, const std::vector<std::string_view>& lines);

	private:
		struct ScopeContext
		{
			struct StackItem
			{
				size_t mNumLocalVariables = 0;
				size_t mNumLocalConstants = 0;
				size_t mNumLocalConstantArrays = 0;
			};

			std::vector<LocalVariable*> mLocalVariables;
			std::vector<Constant> mLocalConstants;
			std::vector<ConstantArray*> mLocalConstantArrays;
			std::vector<StackItem> mScopeStack;			// Number of local variables for each scope on the stack

			ScopeContext()
			{
				mScopeStack.reserve(4);
			}

			inline void beginScope()
			{
				StackItem& item = vectorAdd(mScopeStack);
				item.mNumLocalVariables = mLocalVariables.size();
				item.mNumLocalConstants = mLocalConstants.size();
				item.mNumLocalConstantArrays = mLocalConstantArrays.size();
			}

			inline void endScope()
			{
				const StackItem& item = mScopeStack.back();
				mLocalVariables.resize(item.mNumLocalVariables);
				mLocalConstants.resize(item.mNumLocalConstants);
				mLocalConstantArrays.resize(item.mNumLocalConstantArrays);
				mScopeStack.pop_back();
			}
		};

	private:
		// Node building
		void buildNodesFromCodeLines(BlockNode& blockNode, const std::vector<std::string_view>& lines);

		// Node processing
		void processGlobalDefinitions(BlockNode& rootNode);
		ScriptFunction& processFunctionHeader(Node& node, const TokenList& tokens);
		void processSingleFunction(FunctionNode& functionNode);
		void processUndefinedNodesInBlock(BlockNode& blockNode, ScriptFunction& function, ScopeContext& scopeContext);
		Node* processUndefinedNode(UndefinedNode& undefinedNode, ScriptFunction& function, ScopeContext& scopeContext, NodesIterator& nodesIterator);
		Node* gatherNextStatement(NodesIterator& nodesIterator, ScriptFunction& function, ScopeContext& scopeContext);
		void processTokens(TokenList& tokens, ScriptFunction& function, ScopeContext& scopeContext, uint32 lineNumber, const DataTypeDefinition* resultType = nullptr);
		void processConstantDefinition(TokenList& tokens, NodesIterator& nodesIterator, ScopeContext* scopeContext);
		Node* processIfBlock(TokenList& tokens, ScriptFunction& function, ScopeContext& scopeContext, NodesIterator& nodesIterator, uint32 lineNumber);

		// Misc
		bool processGlobalPragma(const std::string& content);
		AnyBaseValue readConstantExpression(TokenList& tokens, size_t& pos, size_t endPos, const DataTypeDefinition* dataType, uint32 lineNumber);

	private:
		Module& mModule;
		GlobalsLookup& mGlobalsLookup;
		const LineNumberTranslation& mLineNumberTranslation;
		CompileOptions& mCompileOptions;
		TokenProcessing& mTokenProcessing;
		std::vector<FunctionNode*>& mFunctionNodes;
		std::vector<const PragmaNode*> mCurrentPragmas;
	};

}
