/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"
#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/Errors.h"
#include "lemon/compiler/LineNumberTranslation.h"
#include "lemon/compiler/Preprocessor.h"
#include "lemon/compiler/TokenProcessing.h"


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
	class TokenList;

	class API_EXPORT Compiler
	{
	public:
		struct CompileOptions
		{
			const DataTypeDefinition* mExternalAddressType = &PredefinedDataTypes::UINT_64;
			std::wstring mOutputCombinedSource;
			std::wstring mOutputNativizedSource;
			std::wstring mOutputTranslatedSource;
		};

		struct ErrorMessage
		{
			std::string mMessage;
			std::wstring mFilename;
			CompilerError mError;
		};

	public:
		Compiler(Module& module, GlobalsLookup& globalsLookup, const CompileOptions& compileOptions);
		~Compiler();

		bool loadScript(const std::wstring& path);

		bool loadCodeLines(std::vector<std::string_view>& outLines, const std::wstring& path);
		bool compileLines(const std::vector<std::string_view>& lines);

		inline const std::vector<ErrorMessage>& getErrors() const  { return mErrors; }

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

		struct NodesIterator;

	private:
		bool loadScriptInternal(const std::wstring& basepath, const std::wstring& filename, std::vector<std::string_view>& outLines, std::unordered_set<uint64>& includedPathHashes);

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

		// Misc
		bool processGlobalPragma(const std::string& content);

	private:
		Module& mModule;
		GlobalsLookup& mGlobalsLookup;
		CompileOptions mCompileOptions;
		GlobalCompilerConfig mGlobalCompilerConfig;

		TokenProcessing mTokenProcessing;
		Preprocessor mPreprocessor;

		struct ScriptFile
		{
			std::wstring mBasePath;
			std::wstring mFilename;
			String mContent;
			size_t mFirstLine = 0;
		};
		std::vector<ScriptFile*> mScriptFiles;
		ObjectPool<ScriptFile,64> mScriptFilesPool;

		std::vector<FunctionNode*> mFunctionNodes;
		LineNumberTranslation mLineNumberTranslation;
		std::wstring mCurrentScriptFilename;
		std::vector<ErrorMessage> mErrors;
	};

}
