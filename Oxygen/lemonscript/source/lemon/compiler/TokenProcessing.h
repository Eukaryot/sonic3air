/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Constant.h"
#include "lemon/program/Variable.h"
#include "lemon/compiler/Operators.h"


namespace lemon
{
	class Function;
	class ConstantArray;
	class GlobalsLookup;
	class LocalVariable;
	class ScriptFunction;
	class StatementToken;
	class TokenList;
	struct GlobalCompilerConfig;

	class TokenProcessing
	{
	public:
		struct Context
		{
			std::vector<LocalVariable*>* mLocalVariables = nullptr;
			std::vector<Constant>* mLocalConstants = nullptr;
			std::vector<ConstantArray*>* mLocalConstantArrays = nullptr;
			ScriptFunction* mFunction = nullptr;
		};

		struct CachedBuiltinFunction
		{
			std::vector<Function*> mFunctions;
		};

	public:
		Context mContext;

	public:
		TokenProcessing(GlobalsLookup& globalsLookup, const GlobalCompilerConfig& config);

		void processTokens(TokenList& tokensRoot, uint32 lineNumber, const DataTypeDefinition* resultType = nullptr);
		void processForPreprocessor(TokenList& tokensRoot, uint32 lineNumber);

	private:
		void resolveIdentifiers(TokenList& tokens);

		void processDefines(TokenList& tokens);
		void processConstants(TokenList& tokens);
		void processParentheses(TokenList& tokens, std::vector<TokenList*>& outLinearTokenLists);
		void processCommaSeparators(std::vector<TokenList*>& linearTokenLists);

		void processVariableDefinitions(TokenList& tokens);
		void processFunctionCalls(TokenList& tokens);
		void processMemoryAccesses(TokenList& tokens);
		void processArrayAccesses(TokenList& tokens);
		void processExplicitCasts(TokenList& tokens);
		void processVariables(TokenList& tokens);

		void processUnaryOperations(TokenList& tokens);
		void processBinaryOperations(TokenList& tokens);

		void assignStatementDataTypes(TokenList& tokens, const DataTypeDefinition* resultType);
		const DataTypeDefinition* assignStatementDataType(StatementToken& token, const DataTypeDefinition* resultType);

		const Variable* findVariable(uint64 nameHash);
		LocalVariable* findLocalVariable(uint64 nameHash);
		const ConstantArray* findConstantArray(uint64 nameHash);

	private:
		GlobalsLookup& mGlobalsLookup;
		const GlobalCompilerConfig& mConfig;
		uint32 mLineNumber = 0;

		CachedBuiltinFunction mBuiltinConstantArrayAccess;
		CachedBuiltinFunction mBuiltinStringOperatorPlus;
		CachedBuiltinFunction mBuiltinStringOperatorLess;
		CachedBuiltinFunction mBuiltinStringOperatorLessOrEqual;
		CachedBuiltinFunction mBuiltinStringOperatorGreater;
		CachedBuiltinFunction mBuiltinStringOperatorGreaterOrEqual;
		CachedBuiltinFunction mBuiltinStringLength;
	};

}