/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Constant.h"
#include "lemon/program/Variable.h"
#include "lemon/compiler/Operators.h"
#include "lemon/compiler/Token.h"
#include "lemon/compiler/TypeCasting.h"


namespace lemon
{
	class ConstantArray;
	class ConstantToken;
	class Function;
	class GlobalsLookup;
	class IdentifierToken;
	class LocalVariable;
	class Module;
	class ScriptFunction;
	class StatementToken;

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
		TokenProcessing(GlobalsLookup& globalsLookup, Module& module, const CompileOptions& compileOptions);

		void processTokens(TokenList& tokensRoot, uint32 lineNumber, const DataTypeDefinition* resultType = nullptr);
		void processForPreprocessor(TokenList& tokensRoot, uint32 lineNumber);

		bool resolveIdentifiers(TokenList& tokens);
		bool tryResolveIdentifier(TokenList& tokens, size_t pos);

		bool processConstant(TokenList& tokens, size_t pos);

		const ArrayDataType& getArrayDataType(const DataTypeDefinition& elementType, size_t arraySize);

	private:
		struct BinaryOperationResult
		{
			const TypeCasting::BinaryOperatorSignature* mSignature = nullptr;
			Function* mEnforcedFunction = nullptr;
			Operator mSplitToOperator = Operator::_INVALID;
		};

		struct BinaryOperationLookup
		{
			CachedBuiltinFunction* mCachedBuiltinFunction = nullptr;
			TypeCasting::BinaryOperatorSignature mSignature;
			Operator mSplitToOperator = Operator::_INVALID;

			inline BinaryOperationLookup(CachedBuiltinFunction* cachedBuiltinFunction, const DataTypeDefinition* left, const DataTypeDefinition* right, const DataTypeDefinition* result, Operator splitToOperator = Operator::_INVALID) :
				mCachedBuiltinFunction(cachedBuiltinFunction), mSignature(left, right, result), mSplitToOperator(splitToOperator) {}
		};

	private:
		void insertCastTokenIfNecessary(TokenPtr<StatementToken>& token, const DataTypeDefinition* targetDataType);
		void castCompileTimeConstant(ConstantToken& constantToken, const DataTypeDefinition* targetDataType);

		void processDefines(TokenList& tokens);
		void processConstants(TokenList& tokens);

		void processParentheses(TokenList& tokens);
		void processCommaSeparators(TokenList& tokens);

		void processTokenListRecursive(TokenList& tokens);
		void processTokenListRecursiveForPreprocessor(TokenList& tokens);

		void processVariableDefinitions(TokenList& tokens);
		void processFunctionCalls(TokenList& tokens);
		void processMemoryAccesses(TokenList& tokens);
		void processArrayAccesses(TokenList& tokens);
		void processExplicitCasts(TokenList& tokens);
		void processVariables(TokenList& tokens);

		void processUnaryOperations(TokenList& tokens);
		void processBinaryOperations(TokenList& tokens);

		void evaluateCompileTimeConstants(TokenList& tokens);
		bool evaluateCompileTimeConstantsRecursive(Token& inputToken, TokenPtr<StatementToken>& outTokenPtr);

		void resolveMakeCallable(TokenList& tokens);
		void resolveAddressOfFunctions(TokenList& tokens);
		void resolveAddressOfMemoryAccesses(TokenList& tokens);

		BinaryOperationResult getBestOperatorSignature(Operator op, const DataTypeDefinition* leftDataType, const DataTypeDefinition* rightDataType);

		const Function* getBuiltinArrayGetter(const DataTypeDefinition& elementType);
		const Function* getBuiltinArraySetter(const DataTypeDefinition& elementType);

		void assignStatementDataTypes(TokenList& tokens, const DataTypeDefinition* resultType);
		const DataTypeDefinition* assignStatementDataType(StatementToken& token, const DataTypeDefinition* resultType);

		const Variable* findVariable(uint64 nameHash);
		LocalVariable* findLocalVariable(uint64 nameHash);
		const ConstantArray* findConstantArray(uint64 nameHash);

	private:
		GlobalsLookup& mGlobalsLookup;
		Module& mModule;
		const CompileOptions& mCompileOptions;
		TypeCasting mTypeCasting;
		uint32 mLineNumber = 0;

		CachedBuiltinFunction mBuiltinConstantArrayAccess;
		CachedBuiltinFunction mBuiltinArrayBracketGetter;
		CachedBuiltinFunction mBuiltinArrayBracketSetter;
		CachedBuiltinFunction mBuiltinArrayLength;

		CachedBuiltinFunction mBuiltinStringOperatorPlus;
		CachedBuiltinFunction mBuiltinStringOperatorPlusInt64;
		CachedBuiltinFunction mBuiltinStringOperatorPlusInt64Inv;
		CachedBuiltinFunction mBuiltinStringOperatorLess;
		CachedBuiltinFunction mBuiltinStringOperatorLessOrEqual;
		CachedBuiltinFunction mBuiltinStringOperatorGreater;
		CachedBuiltinFunction mBuiltinStringOperatorGreaterOrEqual;
		CachedBuiltinFunction mBuiltinStringBracketGetter;
		CachedBuiltinFunction mBuiltinStringBracketSetter;

		std::vector<BinaryOperationLookup> mBinaryOperationLookup[(size_t)Operator::_NUM_OPERATORS];
	};

}