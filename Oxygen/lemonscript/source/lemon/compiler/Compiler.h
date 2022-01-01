/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"
#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/PreprocessorDefinition.h"


namespace lemon
{
	class Module;
	class GlobalsLookup;
	class Preprocessor;
	class ScriptFunction;
	class LocalVariable;
	class Node;
	class BlockNode;
	class UndefinedNode;
	class FunctionNode;
	class TokenList;

	class API_EXPORT Compiler
	{
	public:
		struct LineNumberTranslation
		{
			struct Interval
			{
				uint32 mStartLineNumber = 0;	// End line number is the start of the next item minus one
				std::wstring mFilename;
				uint32 mLineOffsetInFile = 0;
			};
			std::vector<Interval> mIntervals;

			std::pair<uint32, std::wstring> translateLineNumber(uint32 lineNumber) const;
			void push(uint32 currentLineNumber, const std::wstring& filename, uint32 lineOffsetInFile);
		};

		struct CompileOptions
		{
			PreprocessorDefinitionMap mPreprocessorDefinitions;
			const DataTypeDefinition* mExternalAddressType = &PredefinedDataTypes::UINT_64;
			std::wstring mOutputCombinedSource;
			std::wstring mOutputNativizedSource;
			std::wstring mOutputTranslatedSource;
		};

		struct ErrorMessage
		{
			std::string mMessage;
			std::wstring mFilename;
			uint32 mLineNumber = 0;
		};

	public:
		Compiler(Module& module, GlobalsLookup& globalsLookup);
		Compiler(Module& module, GlobalsLookup& globalsLookup, const CompileOptions& compileOptions);
		~Compiler();

		bool loadScript(const std::wstring& path);

		bool loadCodeLines(std::vector<std::string_view>& outLines, const std::wstring& path);
		bool compileLines(const std::vector<std::string_view>& lines);
		void compileLinesToNode(BlockNode& outNode, const std::vector<std::string_view>& lines);

		inline const std::vector<ErrorMessage>& getErrors() const  { return mErrors; }

	private:
		struct ScopeContext
		{
			std::vector<LocalVariable*> mLocalVariables;
			std::vector<size_t> mScopeStack;			// Number of local variables for each scope on the stack

			ScopeContext()
			{
				mScopeStack.reserve(4);
			}

			inline void beginScope()
			{
				mScopeStack.emplace_back(mLocalVariables.size());
			}

			inline void endScope()
			{
				mLocalVariables.resize(mScopeStack.back());
				mScopeStack.pop_back();
			}
		};

		struct NodesIterator;

	private:
		bool loadScriptInternal(const std::wstring& basepath, const std::wstring& filename, std::vector<std::string_view>& outLines, int inclusionDepth);

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

		// Misc
		bool processGlobalPragma(const std::string& content);

	private:
		Module& mModule;
		GlobalsLookup& mGlobalsLookup;
		CompileOptions mCompileOptions;
		GlobalCompilerConfig mGlobalCompilerConfig;
		Preprocessor& mPreprocessor;		// Must stay alive as it holds the modified code lines

		struct ScriptFile
		{
			std::wstring mBasePath;
			std::wstring mFilename;
			String mContent;
			size_t mFirstLine = 0;
		};
		std::vector<ScriptFile*> mScriptFiles;
		ObjectPool<ScriptFile,64> mScriptFilesPool;

		std::string mScriptContent;
		std::vector<FunctionNode*> mFunctionNodes;
		LineNumberTranslation mLineNumberTranslation;
		std::wstring mCurrentScriptFilename;
		std::vector<ErrorMessage> mErrors;
	};

}
