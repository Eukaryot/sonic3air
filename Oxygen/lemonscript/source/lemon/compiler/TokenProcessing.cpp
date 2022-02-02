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

		bool tryReplaceConstants(const ConstantToken& constLeft, const ConstantToken& constRight, Operator op, int64& outValue)
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
				// TODO: More to add here...?
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
			processIdentifiers(*tokenList);

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

	void TokenProcessing::processDefines(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER)
			{
				const uint64 identifierHash = tokens[i].as<IdentifierToken>().mNameHash;
				const Define* define = mGlobalsLookup.getDefineByName(identifierHash);
				if (nullptr != define)
				{
					tokens.erase(i);
					for (size_t k = 0; k < define->mContent.size(); ++k)
					{
						tokens.insert(define->mContent[k], i + k);
					}

					// TODO: Add implicit cast if necessary
				}
			}
		}
	}

	void TokenProcessing::processConstants(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER)
			{
				const uint64 identifierHash = tokens[i].as<IdentifierToken>().mNameHash;
				const Constant* constant = mGlobalsLookup.getConstantByName(identifierHash);
				if (nullptr != constant)
				{
					ConstantToken& token = tokens.createReplaceAt<ConstantToken>(i);
					token.mDataType = constant->getDataType();
					token.mValue = constant->getValue();
				}
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
						CHECK_ERROR(nullptr == findLocalVariable(identifierToken.mNameHash), "Variable name already used", mLineNumber);

						// Variable may already exist in function (but not in scope, we just checked that)
						RMX_ASSERT(nullptr != mContext.mFunction, "Invalid function pointer");
						LocalVariable* variable = mContext.mFunction->getLocalVariableByIdentifier(identifierToken.mNameHash);
						if (nullptr == variable)
						{
							variable = &mContext.mFunction->addLocalVariable(identifierToken.mName, identifierToken.mNameHash, varType, mLineNumber);
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
					const std::string_view functionName = identifierToken.mName;
					const uint64 nameHash = identifierToken.mNameHash;
					bool isBaseCall = false;
					const Function* function = nullptr;
					const Variable* thisPointer = nullptr;

					bool isValidFunctionCall = false;
					if (!mGlobalsLookup.getFunctionsByName(nameHash).empty())
					{
						// Is it a global function
						isValidFunctionCall = true;
					}
					else if (rmx::startsWith(functionName, "base."))
					{
						// It's a base call
						CHECK_ERROR(functionName.substr(5) == mContext.mFunction->getName(), "Base call goes to a different function", mLineNumber);
						isValidFunctionCall = true;
						isBaseCall = true;
					}
					else
					{
						// Special handling for "string.length()"
						//  -> TODO: Generalize this to pave the way for other kinds of "method calls"
						if (rmx::endsWith(functionName, ".length") && content.empty())
						{
							const std::string_view variableName = functionName.substr(0, functionName.length() - 7);
							const Variable* variable = findVariable(rmx::getMurmur2_64(variableName));
							if (nullptr != variable && variable->getDataType() == &PredefinedDataTypes::STRING)
							{
								function = mBuiltinStringLength.mFunctions[0];
								thisPointer = variable;
								isValidFunctionCall = true;
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
					if (nullptr != thisPointer)
					{
						VariableToken& variableToken = vectorAdd(token.mParameters).create<VariableToken>();
						variableToken.mVariable = thisPointer;
						variableToken.mDataType = thisPointer->getDataType();
					}
					tokens.erase(i+1);

					// Assign types
					std::vector<const DataTypeDefinition*> parameterTypes;
					parameterTypes.reserve(token.mParameters.size());
					for (size_t i = 0; i < token.mParameters.size(); ++i)
					{
						const DataTypeDefinition* type = assignStatementDataType(*token.mParameters[i], nullptr);
						parameterTypes.push_back(type);
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
							const std::vector<Function*>& functions = mGlobalsLookup.getFunctionsByName(nameHash);
							CHECK_ERROR(!functions.empty(), "Unknown function name '" << functionName << "'", mLineNumber);

							// Find best-fitting correct function overload
							function = nullptr;
							uint32 bestPriority = 0xff000000;
							for (const Function* candidateFunction : functions)
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
					const uint64 arrayNameHash = tokens[i].as<IdentifierToken>().mNameHash;
					const ConstantArray* constantArray = mGlobalsLookup.getConstantArrayByName(arrayNameHash);
					if (nullptr != constantArray)
					{
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

	void TokenProcessing::processIdentifiers(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			Token& token = tokens[i];
			if (token.getType() == Token::Type::IDENTIFIER)
			{
				const IdentifierToken& identifierToken = token.as<IdentifierToken>();
				const Variable* variable = findVariable(identifierToken.mNameHash);
				CHECK_ERROR(nullptr != variable, "Unable to resolve identifier: " << identifierToken.mName, mLineNumber);

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

						UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
						token.mOperator = op;
						token.mArgument = &rightToken.as<StatementToken>();

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

			// Check for constants, we might calculate the result at compile time
			bool replacedWithConstant = false;
			if (leftToken.getType() == ConstantToken::TYPE && rightToken.getType() == ConstantToken::TYPE)
			{
				int64 resultValue;
				if (tryReplaceConstants(leftToken.as<ConstantToken>(), rightToken.as<ConstantToken>(), op, resultValue))
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
			variable = mGlobalsLookup.getGlobalVariableByName(nameHash);
		}
		return variable;
	}

	LocalVariable* TokenProcessing::findLocalVariable(uint64 nameHash)
	{
		for (LocalVariable* var : *mContext.mLocalVariables)
		{
			if (var->getNameHash() == nameHash)
				return var;
		}
		return nullptr;
	}

}
