/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/TokenProcessing.h"
#include "lemon/compiler/TokenTypes.h"
#include "lemon/compiler/TypeCasting.h"
#include "lemon/compiler/Utility.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/runtime/StandardLibrary.h"


namespace lemon
{

	namespace
	{
		std::string getOperatorNotAllowedErrorMessage(Operator op)
		{
			if (op >= Operator::UNARY_NOT && op <= Operator::UNARY_INCREMENT)
			{
				return std::string("Unary operator ") + OperatorHelper::getOperatorCharacters(op) + " is not allowed here";
			}
			else if (op <= Operator::COLON)
			{
				return std::string("Binary operator ") + OperatorHelper::getOperatorCharacters(op) + " is not allowed here";
			}
			else
			{
				switch (op)
				{
					case Operator::SEMICOLON_SEPARATOR:	return "Semicolon ; is only allowed in for-loops";
					case Operator::COMMA_SEPARATOR:		return "Comma , is not allowed here";
					case Operator::PARENTHESIS_LEFT:	return "Parenthesis ( is not allowed here";
					case Operator::PARENTHESIS_RIGHT:	return "Parenthesis ) is not allowed here";
					case Operator::BRACKET_LEFT:		return "Bracket [ is not allowed here";
					case Operator::BRACKET_RIGHT:		return "Bracket ] is not allowed here";
					default: break;
				}
			}
			return "Operator is not allowed here";
		}

		bool tryReplaceConstantsUnary(const ConstantToken& constRight, Operator op, int64& outValue)
		{
			if (constRight.mDataType == &PredefinedDataTypes::STRING)
				return false;

			switch (op)
			{
				// TODO: Add this as well - it's just postponed for now, as it introduces too many changes in S3AIR that might possibly be harmful
				//case Operator::BINARY_MINUS:  outValue = -constRight.mValue;				return true;
				case Operator::UNARY_NOT:	  outValue = (constRight.mValue == 0) ? 1 : 0;	return true;
				case Operator::UNARY_BITNOT:  outValue = ~constRight.mValue;				return true;
				default: break;
			}
			return false;
		}

		bool tryReplaceConstantsBinary(const ConstantToken& constLeft, const ConstantToken& constRight, Operator op, int64& outValue)
		{
			if (constLeft.mDataType == &PredefinedDataTypes::STRING || constRight.mDataType == &PredefinedDataTypes::STRING)
				return false;

			switch (op)
			{
				case Operator::BINARY_PLUS:			outValue = constLeft.mValue + constRight.mValue;	return true;
				case Operator::BINARY_MINUS:		outValue = constLeft.mValue - constRight.mValue;	return true;
				case Operator::BINARY_MULTIPLY:		outValue = constLeft.mValue * constRight.mValue;	return true;
				case Operator::BINARY_DIVIDE:		outValue = (constRight.mValue == 0) ? 0 : (constLeft.mValue / constRight.mValue);	return true;
				case Operator::BINARY_MODULO:		outValue = constLeft.mValue % constRight.mValue;	return true;
				case Operator::BINARY_SHIFT_LEFT:	outValue = constLeft.mValue << constRight.mValue;	return true;
				case Operator::BINARY_SHIFT_RIGHT:	outValue = constLeft.mValue >> constRight.mValue;	return true;
				case Operator::BINARY_AND:			outValue = constLeft.mValue & constRight.mValue;	return true;
				case Operator::BINARY_OR:			outValue = constLeft.mValue | constRight.mValue;	return true;
				case Operator::BINARY_XOR:			outValue = constLeft.mValue ^ constRight.mValue;	return true;
				default: break;
			}
			return false;
		}

		void fillCachedBuiltinFunction(TokenProcessing::CachedBuiltinFunction& outCached, const GlobalsLookup& globalsLookup, const StandardLibrary::FunctionName& functionName, bool allowOnlyOne)
		{
			const std::vector<Function*>& functions = globalsLookup.getFunctionsByName(functionName.mHash);
			RMX_ASSERT(!functions.empty(), "Unable to find built-in function '" << functionName.mName << "'");
			if (allowOnlyOne)
				RMX_ASSERT(functions.size() == 1, "Multiple definitions for built-in function '" << functionName.mName << "'");
			outCached.mFunctions = functions;
		}

		template<typename T>
		T* findInList(const std::vector<T*>& list, uint64 nameHash)
		{
			for (T* item : list)
			{
				if (item->getName().getHash() == nameHash)
					return item;
			}
			return nullptr;
		}
	}


	TokenProcessing::TokenProcessing(GlobalsLookup& globalsLookup, const GlobalCompilerConfig& config) :
		mGlobalsLookup(globalsLookup),
		mConfig(config)
	{
		fillCachedBuiltinFunction(mBuiltinConstantArrayAccess,			globalsLookup, StandardLibrary::BUILTIN_NAME_CONSTANT_ARRAY_ACCESS, false);
		fillCachedBuiltinFunction(mBuiltinStringOperatorPlus,			globalsLookup, StandardLibrary::BUILTIN_NAME_STRING_OPERATOR_PLUS, true);
		fillCachedBuiltinFunction(mBuiltinStringOperatorLess,			globalsLookup, StandardLibrary::BUILTIN_NAME_STRING_OPERATOR_LESS, true);
		fillCachedBuiltinFunction(mBuiltinStringOperatorLessOrEqual,	globalsLookup, StandardLibrary::BUILTIN_NAME_STRING_OPERATOR_LESS_OR_EQUAL, true);
		fillCachedBuiltinFunction(mBuiltinStringOperatorGreater,		globalsLookup, StandardLibrary::BUILTIN_NAME_STRING_OPERATOR_GREATER, true);
		fillCachedBuiltinFunction(mBuiltinStringOperatorGreaterOrEqual,	globalsLookup, StandardLibrary::BUILTIN_NAME_STRING_OPERATOR_GREATER_OR_EQUAL, true);
		fillCachedBuiltinFunction(mBuiltinStringLength,					globalsLookup, StandardLibrary::BUILTIN_NAME_STRING_LENGTH, true);
	}

	void TokenProcessing::processTokens(TokenList& tokensRoot, uint32 lineNumber, const DataTypeDefinition* resultType)
	{
		mLineNumber = lineNumber;

		// Try to resolve identifiers
		resolveIdentifiers(tokensRoot);

		// Process defines early, as they can introduce new tokens that need to be considered in the following steps
		processDefines(tokensRoot);

		// Process constants
		processConstants(tokensRoot);

		// Build linear token lists that can mostly be processed individually
		//  -> Each linear token list represents contents of one pair of parenthesis, or a comma-separated part in there -- plus there's always one linear token list for the whole root
		//  -> They are sorted so that inner token lists have a lower index in "linearTokenLists" than their outer token list, so they're evaluated first
		static std::vector<TokenList*> linearTokenLists;	// Not multi-threading safe
		linearTokenLists.clear();
		{
			// Split by parentheses
			processParentheses(tokensRoot, linearTokenLists);

			// Split by commas
			processCommaSeparators(linearTokenLists);
		}

		// We do the other processing steps on each linear token list individually
		for (TokenList* tokenList : linearTokenLists)
		{
			processVariableDefinitions(*tokenList);
			processFunctionCalls(*tokenList);
			processMemoryAccesses(*tokenList);
			processArrayAccesses(*tokenList);
			processExplicitCasts(*tokenList);
			processVariables(*tokenList);

			processUnaryOperations(*tokenList);
			processBinaryOperations(*tokenList);
		}

		// TODO: Statement type assignment will require resolving all identifiers first -- check if this is done here
		assignStatementDataTypes(tokensRoot, resultType);
	}

	void TokenProcessing::processForPreprocessor(TokenList& tokensRoot, uint32 lineNumber)
	{
		mLineNumber = lineNumber;

		// Build linear token lists that can mostly be processed individually
		static std::vector<TokenList*> linearTokenLists;	// Not multi-threading safe
		linearTokenLists.clear();
		processParentheses(tokensRoot, linearTokenLists);

		// We do the other processing steps on each linear token list individually
		for (TokenList* tokenList : linearTokenLists)
		{
			processUnaryOperations(*tokenList);
			processBinaryOperations(*tokenList);
		}
	}

	bool TokenProcessing::resolveIdentifiers(TokenList& tokens)
	{
		bool anyResolved = false;
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER)
			{
				IdentifierToken& identifierToken = tokens[i].as<IdentifierToken>();
				if (nullptr == identifierToken.mResolved)
				{
					const uint64 nameHash = identifierToken.mName.getHash();
					identifierToken.mResolved = mGlobalsLookup.resolveIdentifierByHash(nameHash);
					anyResolved = true;
				}
			}
		}
		return anyResolved;
	}

	void TokenProcessing::processDefines(TokenList& tokens)
	{
		bool anyDefineResolved = false;
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER)
			{
				IdentifierToken& identifierToken = tokens[i].as<IdentifierToken>();
				if (nullptr != identifierToken.mResolved && identifierToken.mResolved->getType() == GlobalsLookup::Identifier::Type::DEFINE)
				{
					const Define& define = identifierToken.mResolved->as<Define>();
					tokens.erase(i);
					for (size_t k = 0; k < define.mContent.size(); ++k)
					{
						tokens.insert(define.mContent[k], i + k);
					}

					// TODO: Add implicit cast if necessary

					anyDefineResolved = true;
				}
			}
		}

		if (anyDefineResolved)
		{
			resolveIdentifiers(tokens);
		}
	}

	void TokenProcessing::processConstants(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER)
			{
				IdentifierToken& identifierToken = tokens[i].as<IdentifierToken>();
				const Constant* constant = nullptr;
				if (nullptr != identifierToken.mResolved && identifierToken.mResolved->getType() == GlobalsLookup::Identifier::Type::CONSTANT)
				{
					constant = &identifierToken.mResolved->as<Constant>();
				}
				else
				{
					for (const Constant& localConstant : *mContext.mLocalConstants)
					{
						if (localConstant.getName() == identifierToken.mName)
						{
							constant = &localConstant;
							break;
						}
					}
					if (nullptr == constant)
						continue;
				}

				ConstantToken& newToken = tokens.createReplaceAt<ConstantToken>(i);
				newToken.mDataType = constant->getDataType();
				newToken.mValue = constant->getValue();
			}
		}
	}

	void TokenProcessing::processParentheses(TokenList& tokens, std::vector<TokenList*>& outLinearTokenLists)
	{
		static std::vector<std::pair<ParenthesisType, size_t>> parenthesisStack;	// Not multi-threading safe
		parenthesisStack.clear();
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::OPERATOR)
			{
				const OperatorToken& opToken = tokens[i].as<OperatorToken>();
				if (opToken.mOperator == Operator::PARENTHESIS_LEFT ||
					opToken.mOperator == Operator::BRACKET_LEFT)
				{
					const ParenthesisType type = (opToken.mOperator == Operator::PARENTHESIS_LEFT) ? ParenthesisType::PARENTHESIS : ParenthesisType::BRACKET;
					parenthesisStack.emplace_back(type, i);
				}
				else if (opToken.mOperator == Operator::PARENTHESIS_RIGHT ||
						 opToken.mOperator == Operator::BRACKET_RIGHT)
				{
					const ParenthesisType type = (opToken.mOperator == Operator::PARENTHESIS_RIGHT) ? ParenthesisType::PARENTHESIS : ParenthesisType::BRACKET;
					CHECK_ERROR(!parenthesisStack.empty() && parenthesisStack.back().first == type, "Parenthesis not matching (too many closed)", mLineNumber);

					// Pack all between parentheses into a new token
					const size_t startPosition = parenthesisStack.back().second;
					const size_t endPosition = i;
					const bool isEmpty = (endPosition == startPosition + 1);

					parenthesisStack.pop_back();

					// Left parenthesis will be replaced with a parenthesis token representing the whole thing
					ParenthesisToken& token = tokens.createReplaceAt<ParenthesisToken>(startPosition);
					token.mParenthesisType = type;

					// Right parenthesis just gets removed
					tokens.erase(endPosition);

					if (!isEmpty)
					{
						// Copy content as new token list into the parenthesis token
						token.mContent.moveFrom(tokens, startPosition + 1, endPosition - startPosition - 1);

						// Add to output
						outLinearTokenLists.push_back(&token.mContent);
					}

					i -= (endPosition - startPosition);
				}
			}
		}

		CHECK_ERROR(parenthesisStack.empty(), "Parenthesis not matching (too many open)", mLineNumber);

		// Add to output
		outLinearTokenLists.push_back(&tokens);
	}

	void TokenProcessing::processCommaSeparators(std::vector<TokenList*>& linearTokenLists)
	{
		static std::vector<size_t> commaPositions;	// Not multi-threading safe
		for (size_t k = 0; k < linearTokenLists.size(); ++k)
		{
			TokenList& tokens = *linearTokenLists[k];

			// Find comma positions
			commaPositions.clear();
			for (size_t i = 0; i < tokens.size(); ++i)
			{
				Token& token = tokens[i];
				if (token.getType() == Token::Type::OPERATOR && token.as<OperatorToken>().mOperator == Operator::COMMA_SEPARATOR)
				{
					commaPositions.push_back(i);
				}
			}

			// Any commas?
			if (!commaPositions.empty())
			{
				CommaSeparatedListToken& commaSeparatedListToken = tokens.createFront<CommaSeparatedListToken>();
				commaSeparatedListToken.mContent.resize(commaPositions.size() + 1);

				// All comma positions have changed by 1
				for (size_t& pos : commaPositions)
					++pos;

				// Add "virtual" comma at the front for symmetry reasons
				commaPositions.insert(commaPositions.begin(), 0);

				for (int j = (int)commaPositions.size() - 1; j >= 0; --j)
				{
					const size_t first = commaPositions[j] + 1;
					commaSeparatedListToken.mContent[j].moveFrom(tokens, first, tokens.size() - first);

					if (j > 0)
					{
						// Erase the comma token itself
						CHECK_ERROR(tokens[commaPositions[j]].getType() == Token::Type::OPERATOR && tokens[commaPositions[j]].as<OperatorToken>().mOperator == Operator::COMMA_SEPARATOR, "Wrong token index", mLineNumber);
						tokens.erase(commaPositions[j]);
					}
				}
				CHECK_ERROR(tokens.size() == 1, "Token list must only contain the CommaSeparatedListToken afterwards", mLineNumber);

				// Add each part to linear token list (in order)
				for (size_t j = 0; j < commaPositions.size(); ++j)
				{
					++k;
					linearTokenLists.insert(linearTokenLists.begin() + k, &commaSeparatedListToken.mContent[j]);
				}
			}
		}
	}

	void TokenProcessing::processVariableDefinitions(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			Token& token = tokens[i];
			switch (token.getType())
			{
				case Token::Type::KEYWORD:
				{
					const Keyword keyword = token.as<KeywordToken>().mKeyword;
					if (keyword == Keyword::FUNCTION)
					{
						// Next token must be an identifier
						CHECK_ERROR(i+1 < tokens.size() && tokens[i+1].getType() == Token::Type::IDENTIFIER, "Function keyword must be followed by an identifier", mLineNumber);

						// TODO: We could register the function name here already, so it is known later on...

					}
					break;
				}

				case Token::Type::VARTYPE:
				{
					const DataTypeDefinition* varType = token.as<VarTypeToken>().mDataType;

					// Next token must be an identifier
					CHECK_ERROR(i+1 < tokens.size(), "Type name must not be the last token", mLineNumber);

					// Next token must be an identifier
					Token& nextToken = tokens[i+1];
					if (nextToken.getType() == Token::Type::IDENTIFIER)
					{
						CHECK_ERROR(varType->mClass != DataTypeDefinition::Class::VOID, "void variables not allowed", mLineNumber);

						// Create new variable
						const IdentifierToken& identifierToken = tokens[i+1].as<IdentifierToken>();
						CHECK_ERROR(nullptr == findLocalVariable(identifierToken.mName.getHash()), "Variable name already used", mLineNumber);

						// Variable may already exist in function (but not in scope, we just checked that)
						RMX_ASSERT(nullptr != mContext.mFunction, "Invalid function pointer");
						LocalVariable* variable = mContext.mFunction->getLocalVariableByIdentifier(identifierToken.mName.getHash());
						if (nullptr == variable)
						{
							variable = &mContext.mFunction->addLocalVariable(identifierToken.mName, varType, mLineNumber);
						}
						mContext.mLocalVariables->push_back(variable);

						VariableToken& token = tokens.createReplaceAt<VariableToken>(i);
						token.mVariable = variable;
						token.mDataType = variable->getDataType();

						tokens.erase(i+1);
					}
					break;
				}

				default:
					break;
			}
		}
	}

	void TokenProcessing::processFunctionCalls(TokenList& tokens)
	{
		for (size_t i = 0; i + 1 < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER && tokens[i+1].getType() == Token::Type::PARENTHESIS)
			{
				// Must be a round parenthesis, not a bracket
				if (tokens[i+1].as<ParenthesisToken>().mParenthesisType == ParenthesisType::PARENTHESIS)
				{
					TokenList& content = tokens[i+1].as<ParenthesisToken>().mContent;
					IdentifierToken& identifierToken = tokens[i].as<IdentifierToken>();
					const std::string_view functionName = identifierToken.mName.getString();
					bool isBaseCall = false;
					const Function* function = nullptr;
					const Variable* thisPointerVariable = nullptr;

					bool isValidFunctionCall = false;
					const std::vector<Function*>& candidateFunctions = mGlobalsLookup.getFunctionsByName(identifierToken.mName.getHash());
					if (!candidateFunctions.empty())
					{
						// Is it a global function
						isValidFunctionCall = true;
					}
					else if (rmx::startsWith(functionName, "base."))
					{
						// It's a base call
						CHECK_ERROR(functionName.substr(5) == mContext.mFunction->getName().getString(), "Base call goes to a different function", mLineNumber);
						isValidFunctionCall = true;
						isBaseCall = true;
					}
					else
					{
						// Special handling for "string.length()" and "array.length()"
						//  -> TODO: Generalize this to pave the way for other kinds of "method calls"
						if (rmx::endsWith(functionName, ".length") && content.empty())
						{
							const std::string_view variableName = functionName.substr(0, functionName.length() - 7);
							const Variable* variable = findVariable(rmx::getMurmur2_64(variableName));
							if (nullptr != variable && variable->getDataType() == &PredefinedDataTypes::STRING)
							{
								function = mBuiltinStringLength.mFunctions[0];
								thisPointerVariable = variable;
								isValidFunctionCall = true;
							}
							else
							{
								const ConstantArray* constantArray = findConstantArray(rmx::getMurmur2_64(variableName));
								if (nullptr != constantArray)
								{
									// This can simply be replaced with a compile-time constant
									ConstantToken& constantToken = tokens.createReplaceAt<ConstantToken>(i);
									constantToken.mValue = (uint64)constantArray->getSize();
									constantToken.mDataType = &PredefinedDataTypes::CONST_INT;
									tokens.erase(i+1);
									continue;
								}
							}
						}
					}
					CHECK_ERROR(isValidFunctionCall, "Unknown function name '" << functionName << "'", mLineNumber);

					// Create function token
					FunctionToken& token = tokens.createReplaceAt<FunctionToken>(i);

					// Build list of parameters
					if (!content.empty())
					{
						if (content[0].getType() == Token::Type::COMMA_SEPARATED)
						{
							const std::vector<TokenList>& tokenLists = content[0].as<CommaSeparatedListToken>().mContent;
							token.mParameters.reserve(tokenLists.size());
							for (const TokenList& tokenList : tokenLists)
							{
								CHECK_ERROR(tokenList.size() == 1, "Function parameter content must be one token", mLineNumber);
								CHECK_ERROR(tokenList[0].isStatement(), "Function parameter content must be a statement", mLineNumber);
								vectorAdd(token.mParameters) = tokenList[0].as<StatementToken>();
							}
						}
						else
						{
							CHECK_ERROR(content.size() == 1, "Function parameter content must be one token", mLineNumber);
							CHECK_ERROR(content[0].isStatement(), "Function parameter content must be a statement", mLineNumber);
							vectorAdd(token.mParameters) = content[0].as<StatementToken>();
						}
					}
					if (nullptr != thisPointerVariable)
					{
						VariableToken& variableToken = vectorAdd(token.mParameters).create<VariableToken>();
						variableToken.mVariable = thisPointerVariable;
						variableToken.mDataType = thisPointerVariable->getDataType();
					}
					tokens.erase(i+1);

					// Assign types
					static std::vector<const DataTypeDefinition*> parameterTypes;	// Not multi-threading safe
					parameterTypes.resize(token.mParameters.size());
					for (size_t i = 0; i < token.mParameters.size(); ++i)
					{
						parameterTypes[i] = assignStatementDataType(*token.mParameters[i], nullptr);
					}

					// If the function was not determined already, do that now
					if (nullptr == function)
					{
						// Find out which function signature actually fits
						RMX_ASSERT(nullptr != mContext.mFunction, "Invalid function pointer");
						if (isBaseCall)
						{
							// Base call must use the same function signature as the current one
							CHECK_ERROR(parameterTypes.size() == mContext.mFunction->getParameters().size(), "Base function call has different parameter count", mLineNumber);
							for (size_t i = 0; i < parameterTypes.size(); ++i)
							{
								CHECK_ERROR(parameterTypes[i] == mContext.mFunction->getParameters()[i].mType, "Base function call has different parameter at index " + std::to_string(i), mLineNumber);
							}

							// Use the very same function again, as a base call
							function = mContext.mFunction;
							token.mIsBaseCall = true;
						}
						else
						{
							// Find best-fitting correct function overload
							function = nullptr;
							uint32 bestPriority = 0xff000000;
							for (const Function* candidateFunction : candidateFunctions)
							{
								const uint32 priority = TypeCasting(mConfig).getPriorityOfSignature(parameterTypes, candidateFunction->getParameters());
								if (priority < bestPriority)
								{
									bestPriority = priority;
									function = candidateFunction;
								}
							}
							CHECK_ERROR(bestPriority < 0xff000000, "No appropriate function overload found calling '" << functionName << "', the number or types of parameters passed are wrong", mLineNumber);
						}

						// TODO: Perform implicit casts for parameters here?
					}

					if (nullptr != function)
					{
						token.mFunction = function;
						token.mDataType = function->getReturnType();
					}
				}
			}
		}
	}

	void TokenProcessing::processMemoryAccesses(TokenList& tokens)
	{
		for (size_t i = 0; i + 1 < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::VARTYPE && tokens[i+1].getType() == Token::Type::PARENTHESIS)
			{
				// Must be a bracket
				if (tokens[i+1].as<ParenthesisToken>().mParenthesisType == ParenthesisType::BRACKET)
				{
					TokenList& content = tokens[i+1].as<ParenthesisToken>().mContent;
					CHECK_ERROR(content.size() == 1, "Expected exactly one token inside brackets", mLineNumber);
					CHECK_ERROR(content[0].isStatement(), "Expected statement token inside brackets", mLineNumber);

					const DataTypeDefinition* dataType = tokens[i].as<VarTypeToken>().mDataType;
					CHECK_ERROR(dataType->mClass == DataTypeDefinition::Class::INTEGER && dataType->as<IntegerDataType>().mSemantics == IntegerDataType::Semantics::DEFAULT, "Memory access is only possible using basic integer types, but not '" << dataType->toString() << "'", mLineNumber);

					MemoryAccessToken& token = tokens.createReplaceAt<MemoryAccessToken>(i);
					token.mDataType = dataType;
					token.mAddress = content[0].as<StatementToken>();
					tokens.erase(i+1);

					assignStatementDataType(*token.mAddress, &PredefinedDataTypes::UINT_32);
				}
			}
		}
	}

	void TokenProcessing::processArrayAccesses(TokenList& tokens)
	{
		for (size_t i = 0; i + 1 < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER && tokens[i+1].getType() == Token::Type::PARENTHESIS)
			{
				// Must be a bracket
				if (tokens[i+1].as<ParenthesisToken>().mParenthesisType == ParenthesisType::BRACKET)
				{
					// Check the identifier
					IdentifierToken& identifierToken = tokens[i].as<IdentifierToken>();
					const ConstantArray* constantArray = nullptr;
					if (nullptr != identifierToken.mResolved && identifierToken.mResolved->getType() == GlobalsLookup::Identifier::Type::CONSTANT_ARRAY)
					{
						constantArray = &identifierToken.mResolved->as<ConstantArray>();
					}
					else
					{
						// Check for local constant array
						constantArray = findInList(*mContext.mLocalConstantArrays, identifierToken.mName.getHash());
						CHECK_ERROR(nullptr != constantArray, "Unable to resolve identifier: " << identifierToken.mName.getString(), mLineNumber);
					}

					TokenList& content = tokens[i+1].as<ParenthesisToken>().mContent;
					CHECK_ERROR(content.size() == 1, "Expected exactly one token inside brackets", mLineNumber);
					CHECK_ERROR(content[0].isStatement(), "Expected statement token inside brackets", mLineNumber);

					const Function* matchingFunction = nullptr;
					for (const Function* function : mBuiltinConstantArrayAccess.mFunctions)
					{
						if (function->getReturnType() == constantArray->getElementDataType())
						{
							matchingFunction = function;
							break;
						}
					}
					if (nullptr == matchingFunction)
						continue;

				#ifdef DEBUG
					const Function::ParameterList& parameterList = matchingFunction->getParameters();
					RMX_ASSERT(parameterList.size() == 2 && parameterList[0].mType == &PredefinedDataTypes::UINT_32 && parameterList[1].mType == &PredefinedDataTypes::UINT_32, "Function signature for constant array access does not fit");
				#endif

					FunctionToken& token = tokens.createReplaceAt<FunctionToken>(i);
					token.mFunction = matchingFunction;
					token.mParameters.resize(2);
					ConstantToken& idToken = token.mParameters[0].create<ConstantToken>();
					idToken.mValue = constantArray->getID();
					idToken.mDataType = &PredefinedDataTypes::UINT_32;
					token.mParameters[1] = content[0].as<StatementToken>();		// Array index
					token.mDataType = matchingFunction->getReturnType();

					assignStatementDataType(*token.mParameters[0], &PredefinedDataTypes::UINT_32);

					tokens.erase(i+1);
				}
			}
		}
	}

	void TokenProcessing::processExplicitCasts(TokenList& tokens)
	{
		for (size_t i = 0; i + 1 < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::VARTYPE && tokens[i+1].getType() == Token::Type::PARENTHESIS)
			{
				// Must be a round parenthesis, not a bracket
				if (tokens[i+1].as<ParenthesisToken>().mParenthesisType == ParenthesisType::PARENTHESIS)
				{
					const DataTypeDefinition* targetType = tokens[i].as<VarTypeToken>().mDataType;

					ValueCastToken& token = tokens.createReplaceAt<ValueCastToken>(i);
					token.mArgument = tokens[i+1].as<ParenthesisToken>();
					token.mDataType = targetType;
					tokens.erase(i+1);
				}
			}
		}
	}

	void TokenProcessing::processVariables(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			Token& token = tokens[i];
			if (token.getType() == Token::Type::IDENTIFIER)
			{
				// Check the identifier
				IdentifierToken& identifierToken = tokens[i].as<IdentifierToken>();
				const Variable* variable = nullptr;
				if (nullptr != identifierToken.mResolved && identifierToken.mResolved->getType() == GlobalsLookup::Identifier::Type::GLOBAL_VARIABLE)
				{
					variable = &identifierToken.mResolved->as<Variable>();
				}
				else
				{
					// Check for local variable
					variable = findLocalVariable(identifierToken.mName.getHash());
					CHECK_ERROR(nullptr != variable, "Unable to resolve identifier: " << identifierToken.mName.getString(), mLineNumber);
				}

				VariableToken& token = tokens.createReplaceAt<VariableToken>(i);
				token.mVariable = variable;
				token.mDataType = variable->getDataType();
			}
		}
	}

	void TokenProcessing::processUnaryOperations(TokenList& tokens)
	{
		// Left to right associative
		for (int i = 0; i < (int)tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::OPERATOR)
			{
				const Operator op = tokens[i].as<OperatorToken>().mOperator;
				switch (op)
				{
					case Operator::UNARY_DECREMENT:
					case Operator::UNARY_INCREMENT:
					{
						// Postfix
						if (i == 0)
							continue;

						Token& leftToken = tokens[i-1];
						if (!leftToken.isStatement())
							continue;

						UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
						token.mOperator = op;
						token.mArgument = &leftToken.as<StatementToken>();

						tokens.erase(i-1);
						break;
					}

					default:
						break;
				}
			}
		}

		// Right to left associative: Go through in reverse order
		for (int i = (int)tokens.size() - 1; i >= 0; --i)
		{
			if (tokens[i].getType() == Token::Type::OPERATOR)
			{
				const Operator op = tokens[i].as<OperatorToken>().mOperator;
				switch (op)
				{
					case Operator::BINARY_MINUS:
					case Operator::UNARY_NOT:
					case Operator::UNARY_BITNOT:
					{
						CHECK_ERROR((size_t)(i+1) != tokens.size(), "Unary operator not allowed as last", mLineNumber);

						// Minus could be binary or unary... let's find out
						if (op == Operator::BINARY_MINUS && i > 0)
						{
							Token& leftToken = tokens[i-1];
							if (leftToken.getType() != Token::Type::OPERATOR)
								continue;
						}

						Token& rightToken = tokens[i+1];
						CHECK_ERROR(rightToken.isStatement(), "Right of operator is no statement", mLineNumber);

						// Check for compile-time constant expression
						bool replacedWithConstant = false;
						if (rightToken.getType() == ConstantToken::TYPE)
						{
							int64 resultValue;
							if (tryReplaceConstantsUnary(rightToken.as<ConstantToken>(), op, resultValue))
							{
								ConstantToken& token = tokens.createReplaceAt<ConstantToken>(i);
								token.mValue = resultValue;
								token.mDataType = rightToken.as<ConstantToken>().mDataType;
								replacedWithConstant = true;
							}
						}

						if (!replacedWithConstant)
						{
							UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
							token.mOperator = op;
							token.mArgument = &rightToken.as<StatementToken>();
						}

						tokens.erase(i+1);
						break;
					}

					case Operator::UNARY_DECREMENT:
					case Operator::UNARY_INCREMENT:
					{
						// Prefix
						if ((size_t)(i+1) == tokens.size())
							continue;

						Token& rightToken = tokens[i+1];
						if (!rightToken.isStatement())
							continue;

						UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
						token.mOperator = op;
						token.mArgument = &rightToken.as<StatementToken>();

						tokens.erase(i+1);
						break;
					}

					default:
						break;
				}
			}
		}
	}

	void TokenProcessing::processBinaryOperations(TokenList& tokens)
	{
		for (;;)
		{
			// Find operator with lowest priority
			uint8 bestPriority = 0xff;
			size_t bestPosition = 0;
			for (size_t i = 0; i < tokens.size(); ++i)
			{
				if (tokens[i].getType() == Token::Type::OPERATOR)
				{
					const Operator op = tokens[i].as<OperatorToken>().mOperator;
					CHECK_ERROR((i > 0 && i < tokens.size()-1) && (op != Operator::SEMICOLON_SEPARATOR), getOperatorNotAllowedErrorMessage(op), mLineNumber);

					const uint8 priority = OperatorHelper::getOperatorPriority(op);
					const bool isLower = (priority == bestPriority) ? OperatorHelper::isOperatorAssociative(op) : (priority < bestPriority);
					if (isLower)
					{
						bestPriority = priority;
						bestPosition = i;
					}
				}
			}

			if (bestPosition == 0)
				break;

			const Operator op = tokens[bestPosition].as<OperatorToken>().mOperator;
			Token& leftToken = tokens[bestPosition - 1];
			Token& rightToken = tokens[bestPosition + 1];

			CHECK_ERROR(leftToken.isStatement(), "Left of operator is no statement", mLineNumber);
			CHECK_ERROR(rightToken.isStatement(), "Right of operator is no statement", mLineNumber);

			// Check for compile-time constant expression
			bool replacedWithConstant = false;
			if (leftToken.getType() == ConstantToken::TYPE && rightToken.getType() == ConstantToken::TYPE)
			{
				int64 resultValue;
				if (tryReplaceConstantsBinary(leftToken.as<ConstantToken>(), rightToken.as<ConstantToken>(), op, resultValue))
				{
					ConstantToken& token = tokens.createReplaceAt<ConstantToken>(bestPosition);
					token.mValue = resultValue;
					token.mDataType = leftToken.as<ConstantToken>().mDataType;
					replacedWithConstant = true;
				}
			}

			if (!replacedWithConstant)
			{
				BinaryOperationToken& token = tokens.createReplaceAt<BinaryOperationToken>(bestPosition);
				token.mOperator = op;
				token.mLeft = &leftToken.as<StatementToken>();
				token.mRight = &rightToken.as<StatementToken>();
			}

			tokens.erase(bestPosition + 1);
			tokens.erase(bestPosition - 1);
		}
	}

	void TokenProcessing::assignStatementDataTypes(TokenList& tokens, const DataTypeDefinition* resultType)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].isStatement())
			{
				assignStatementDataType(tokens[i].as<StatementToken>(), resultType);
			}
		}
	}

	const DataTypeDefinition* TokenProcessing::assignStatementDataType(StatementToken& token, const DataTypeDefinition* resultType)
	{
		switch (token.getType())
		{
			case Token::Type::CONSTANT:
			{
				if (token.mDataType != &PredefinedDataTypes::STRING)
				{
					token.mDataType = (nullptr != resultType) ? resultType : &PredefinedDataTypes::CONST_INT;
				}
				break;
			}

			case Token::Type::VARIABLE:
			{
				// Nothing to do, data type was already set when creating the token
				break;
			}

			case Token::Type::FUNCTION:
			{
				// Nothing to do, "processFunctionCalls" cared about everything already
				break;
			}

			case Token::Type::MEMORY_ACCESS:
			{
				// Nothing to do, "processMemoryAccesses" cared about everything already
				break;
			}

			case Token::Type::PARENTHESIS:
			{
				ParenthesisToken& pt = token.as<ParenthesisToken>();

				CHECK_ERROR(pt.mContent.size() == 1, "Parenthesis content must be one token", mLineNumber);
				CHECK_ERROR(pt.mContent[0].isStatement(), "Parenthesis content must be a statement", mLineNumber);

				StatementToken& innerStatement = pt.mContent[0].as<StatementToken>();
				token.mDataType = assignStatementDataType(innerStatement, resultType);
				break;
			}

			case Token::Type::UNARY_OPERATION:
			{
				UnaryOperationToken& uot = token.as<UnaryOperationToken>();
				token.mDataType = assignStatementDataType(*uot.mArgument, resultType);
				break;
			}

			case Token::Type::BINARY_OPERATION:
			{
				BinaryOperationToken& bot = token.as<BinaryOperationToken>();
				const OperatorHelper::OperatorType opType = OperatorHelper::getOperatorType(bot.mOperator);
				const DataTypeDefinition* expectedType = (opType == OperatorHelper::OperatorType::SYMMETRIC) ? resultType : nullptr;

				const DataTypeDefinition* leftDataType = assignStatementDataType(*bot.mLeft, expectedType);
				const DataTypeDefinition* rightDataType = assignStatementDataType(*bot.mRight, (opType == OperatorHelper::OperatorType::ASSIGNMENT) ? leftDataType : expectedType);

				if (mConfig.mScriptFeatureLevel >= 2)
				{
					// Special handling for certain operations with two strings
					if (leftDataType == &PredefinedDataTypes::STRING && rightDataType == &PredefinedDataTypes::STRING)
					{
						CachedBuiltinFunction* cachedBuiltinFunction = nullptr;
						switch (bot.mOperator)
						{
							case Operator::BINARY_PLUS:				 cachedBuiltinFunction = &mBuiltinStringOperatorPlus;  break;
							case Operator::COMPARE_LESS:			 cachedBuiltinFunction = &mBuiltinStringOperatorLess;  break;
							case Operator::COMPARE_LESS_OR_EQUAL:	 cachedBuiltinFunction = &mBuiltinStringOperatorLessOrEqual;  break;
							case Operator::COMPARE_GREATER:			 cachedBuiltinFunction = &mBuiltinStringOperatorGreater;  break;
							case Operator::COMPARE_GREATER_OR_EQUAL: cachedBuiltinFunction = &mBuiltinStringOperatorGreaterOrEqual;  break;
							default: break;
						}

						if (nullptr != cachedBuiltinFunction)
						{
							bot.mFunction = cachedBuiltinFunction->mFunctions[0];
							token.mDataType = &PredefinedDataTypes::STRING;
							break;
						}
					}
				}

				// Choose best fitting signature
				const TypeCasting::BinaryOperatorSignature* signature = nullptr;
				const bool result = TypeCasting(mConfig).getBestSignature(bot.mOperator, leftDataType, rightDataType, &signature);
				CHECK_ERROR(result, "Cannot implicitly cast between types '" << leftDataType->toString() << "' and '" << rightDataType->toString() << "'", mLineNumber);

				token.mDataType = signature->mResult;

				if (opType != OperatorHelper::OperatorType::TRINARY)
				{
					if (leftDataType->mClass == DataTypeDefinition::Class::INTEGER && rightDataType->mClass == DataTypeDefinition::Class::INTEGER)
					{
						// Where necessary, add implicit casts
						if (leftDataType->mBytes != signature->mLeft->mBytes)	// Ignore signed/unsigned differences
						{
							TokenPtr<StatementToken> inner = bot.mLeft;
							ValueCastToken& vct = bot.mLeft.create<ValueCastToken>();
							vct.mDataType = signature->mLeft;
							vct.mArgument = inner;
						}
						if (rightDataType->mBytes != signature->mRight->mBytes)	// Ignore signed/unsigned differences
						{
							TokenPtr<StatementToken> inner = bot.mRight;
							ValueCastToken& vct = bot.mRight.create<ValueCastToken>();
							vct.mDataType = signature->mRight;
							vct.mArgument = inner;
						}
					}
				}
				break;
			}

			case Token::Type::VALUE_CAST:
			{
				ValueCastToken& vct = token.as<ValueCastToken>();

				// This token has the correct data type assigned already
				//  -> What's left is determining its contents' data type
				assignStatementDataType(*vct.mArgument, token.mDataType);

				// Check if types fit together at all
				CHECK_ERROR(TypeCasting(mConfig).getImplicitCastPriority(vct.mArgument->mDataType, vct.mDataType) != 0xff, "Explicit cast not possible", mLineNumber);
				break;
			}

			default:
				break;
		}
		return token.mDataType;
	}

	const Variable* TokenProcessing::findVariable(uint64 nameHash)
	{
		// Search for local variables first
		const Variable* variable = findLocalVariable(nameHash);
		if (nullptr == variable)
		{
			// Maybe it's a global variable
			const GlobalsLookup::Identifier* resolvedIdentifier = mGlobalsLookup.resolveIdentifierByHash(nameHash);
			if (nullptr != resolvedIdentifier && resolvedIdentifier->getType() == GlobalsLookup::Identifier::Type::GLOBAL_VARIABLE)
			{
				variable = &resolvedIdentifier->as<Variable>();
			}
		}
		return variable;
	}

	LocalVariable* TokenProcessing::findLocalVariable(uint64 nameHash)
	{
		return findInList(*mContext.mLocalVariables, nameHash);
	}

	const ConstantArray* TokenProcessing::findConstantArray(uint64 nameHash)
	{
		// Search for local constant arrays first
		const ConstantArray* constantArray = findInList(*mContext.mLocalConstantArrays, nameHash);
		if (nullptr == constantArray)
		{
			// Maybe it's a global constant array
			const GlobalsLookup::Identifier* resolvedIdentifier = mGlobalsLookup.resolveIdentifierByHash(nameHash);
			if (nullptr != resolvedIdentifier && resolvedIdentifier->getType() == GlobalsLookup::Identifier::Type::CONSTANT_ARRAY)
			{
				constantArray = &resolvedIdentifier->as<ConstantArray>();
			}
		}
		return constantArray;
	}

}
