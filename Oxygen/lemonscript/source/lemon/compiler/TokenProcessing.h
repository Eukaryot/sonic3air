/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Variable.h"
#include "lemon/compiler/Operators.h"


namespace lemon
{
	class GlobalsLookup;
	class ScriptFunction;
	class LocalVariable;
	class TokenList;
	class StatementToken;
	struct GlobalCompilerConfig;

	class TokenProcessing
	{
	public:
		struct Context
		{
			GlobalsLookup& mGlobalsLookup;
			std::vector<LocalVariable*>& mLocalVariables;
			ScriptFunction* mFunction = nullptr;

			inline Context(GlobalsLookup& globalsLookup, std::vector<LocalVariable*>& localVariables, ScriptFunction* function) :
				mGlobalsLookup(globalsLookup), mLocalVariables(localVariables), mFunction(function)
			{}
		};

	public:
		inline TokenProcessing(const Context& context, const GlobalCompilerConfig& config) : mContext(context), mConfig(config) {}

		void processTokens(TokenList& tokensRoot, uint32 lineNumber, const DataTypeDefinition* resultType = nullptr);
		void processForPreprocessor(TokenList& tokensRoot, uint32 lineNumber);

	private:
		void processDefines(TokenList& tokens);
		void processConstants(TokenList& tokens);
		void processParentheses(TokenList& tokens, std::vector<TokenList*>& outLinearTokenLists);
		void processCommaSeparators(std::vector<TokenList*>& linearTokenLists);

		void processVariableDefinitions(TokenList& tokens);
		void processFunctionCalls(TokenList& tokens);
		void processMemoryAccesses(TokenList& tokens);
		void processArrayAccesses(TokenList& tokens);
		void processExplicitCasts(TokenList& tokens);
		void processIdentifiers(TokenList& tokens);

		void processUnaryOperations(TokenList& tokens);
		void processBinaryOperations(TokenList& tokens);

		void assignStatementDataTypes(TokenList& tokens, const DataTypeDefinition* resultType);
		const DataTypeDefinition* assignStatementDataType(StatementToken& token, const DataTypeDefinition* resultType);

		const Variable* findVariable(uint64 nameHash);
		LocalVariable* findLocalVariable(uint64 nameHash);

	private:
		const Context& mContext;
		const GlobalCompilerConfig& mConfig;
		uint32 mLineNumber = 0;
	};

}