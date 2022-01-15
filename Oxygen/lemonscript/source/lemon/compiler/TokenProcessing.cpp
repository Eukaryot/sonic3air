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
	}


	void TokenProcessing::processTokens(TokenList& tokensRoot, uint32 lineNumber, const DataTypeDefinition* resultType)
	{
		mLineNumber = lineNumber;

		// Process constants & defines
		processConstantsAndDefines(tokensRoot);

		// Split by parentheses
		//  -> Each linear token list represents contents of one pair of parenthesis, plus one for the whole root
		static std::vector<TokenList*> linearTokenLists;	// Not multi-threading safe
		linearTokenLists.clear();
		processParentheses(tokensRoot, linearTokenLists);

		// Split by commas
		processCommaSeparators(linearTokenLists);

		// We do the other processing steps on each linear token list individually
		for (TokenList* tokenList : linearTokenLists)
		{
			processVariableDefinitions(*tokenList);
			processFunctionCalls(*tokenList);
			processMemoryAccesses(*tokenList);
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

		// Split by parentheses
		//  -> Each linear token list represents contents of one pair of parenthesis, plus one for the whole root
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

	void TokenProcessing::processConstantsAndDefines(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER)
			{
				const uint64 identifierHash = rmx::getMurmur2_64(tokens[i].as<IdentifierToken>().mIdentifier);

				const Constant* constant = mContext.mGlobalsLookup.getConstantByName(identifierHash);
				if (nullptr != constant)
				{
					ConstantToken& token = tokens.createReplaceAt<ConstantToken>(i);
					token.mDataType = constant->getDataType();
					token.mValue = constant->getValue();
				}

				const Define* define = mContext.mGlobalsLookup.getDefineByName(identifierHash);
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
						const std::string& identifier = tokens[i+1].as<IdentifierToken>().mIdentifier;
						CHECK_ERROR(nullptr == findLocalVariable(identifier), "Variable name already used", mLineNumber);

						// Variable may already exist in function (but not in scope, we just checked that)
						RMX_ASSERT(nullptr != mContext.mFunction, "Invalid function pointer");
						LocalVariable* variable = mContext.mFunction->getLocalVariableByIdentifier(identifier);
						if (nullptr == variable)
						{
							variable = &mContext.mFunction->addLocalVariable(identifier, varType, mLineNumber);
						}
						mContext.mLocalVariables.push_back(variable);

						VariableToken& token = tokens.createReplaceAt<VariableToken>(i);
						token.mVariable = variable;

						tokens.erase(i+1);
					}
				}

				default:
					break;
			}
		}
	}

	void TokenProcessing::processFunctionCalls(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size()-1; ++i)
		{
			if (tokens[i].getType() == Token::Type::IDENTIFIER && tokens[i+1].getType() == Token::Type::PARENTHESIS)
			{
				// Must be a round parenthesis, not a bracket
				if (tokens[i+1].as<ParenthesisToken>().mParenthesisType == ParenthesisType::PARENTHESIS)
				{
					const std::string functionName = tokens[i].as<IdentifierToken>().mIdentifier;
					CHECK_ERROR(!mContext.mGlobalsLookup.getFunctionsByName(rmx::getMurmur2_64(functionName)).empty() || String(functionName).startsWith("base."), "Unknown function name '" + functionName + "'", mLineNumber);

					FunctionToken& token = tokens.createReplaceAt<FunctionToken>(i);
					token.mFunctionName = functionName;

					TokenList& content = tokens[i+1].as<ParenthesisToken>().mContent;
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
					tokens.erase(i+1);
				}
			}
		}
	}

	void TokenProcessing::processMemoryAccesses(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size()-1; ++i)
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

					MemoryAccessToken& token = tokens.createReplaceAt<MemoryAccessToken>(i);
					token.mDataType = dataType;
					token.mAddress = content[0].as<StatementToken>();
					tokens.erase(i+1);
				}
			}
		}
	}

	void TokenProcessing::processExplicitCasts(TokenList& tokens)
	{
		for (size_t i = 0; i < tokens.size()-1; ++i)
		{
			if (tokens[i].getType() == Token::Type::VARTYPE && tokens[i+1].getType() == Token::Type::PARENTHESIS)
			{
				// Must be a round parenthesis, not a bracket
				if (tokens[i+1].as<ParenthesisToken>().mParenthesisType == ParenthesisType::PARENTHESIS)
				{
					const DataTypeDefinition* targetType = tokens[i].as<VarTypeToken>().mDataType;

					ValueCastToken& token = tokens.createReplaceAt<ValueCastToken>(i);
					token.mArgument = tokens[i + 1].as<ParenthesisToken>();
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
				const std::string& name = token.as<IdentifierToken>().mIdentifier;

				// Search for local variables first
				const Variable* variable = findLocalVariable(name);
				if (nullptr == variable)
				{
					// Maybe it's a global variable
					const uint64 nameHash = rmx::getMurmur2_64(name);
					variable = mContext.mGlobalsLookup.getGlobalVariableByName(nameHash);
				}

				CHECK_ERROR(nullptr != variable, "Unable to resolve identifier: " + name, mLineNumber);

				VariableToken& token = tokens.createReplaceAt<VariableToken>(i);
				token.mVariable = variable;
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

						Token& leftToken = tokens[i - 1];
						if (!leftToken.isStatement())
							continue;

						UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
						token.mOperator = op;
						token.mArgument = &leftToken.as<StatementToken>();

						tokens.erase(i - 1);
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
							Token& leftToken = tokens[i - 1];
							if (leftToken.getType() != Token::Type::OPERATOR)
								continue;
						}

						Token& rightToken = tokens[i + 1];
						CHECK_ERROR(rightToken.isStatement(), "Right of operator is no statement", mLineNumber);

						UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
						token.mOperator = op;
						token.mArgument = &rightToken.as<StatementToken>();

						tokens.erase(i + 1);
						break;
					}

					case Operator::UNARY_DECREMENT:
					case Operator::UNARY_INCREMENT:
					{
						// Prefix
						if ((size_t)(i+1) == tokens.size())
							continue;

						Token& rightToken = tokens[i + 1];
						if (!rightToken.isStatement())
							continue;

						UnaryOperationToken& token = tokens.createReplaceAt<UnaryOperationToken>(i);
						token.mOperator = op;
						token.mArgument = &rightToken.as<StatementToken>();

						tokens.erase(i + 1);
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
				if (token.mDataType == &PredefinedDataTypes::STRING)
				{
					token.mDataType = &PredefinedDataTypes::STRING;
				}
				else
				{
					token.mDataType = (nullptr != resultType) ? resultType : &PredefinedDataTypes::CONST_INT;
				}
				break;
			}

			case Token::Type::VARIABLE:
			{
				// Use variable data type
				token.mDataType = token.as<VariableToken>().mVariable->getDataType();
				break;
			}

			case Token::Type::FUNCTION:
			{
				FunctionToken& ft = token.as<FunctionToken>();

				// Assign types
				std::vector<const DataTypeDefinition*> parameterTypes;
				parameterTypes.reserve(ft.mParameters.size());
				for (size_t i = 0; i < ft.mParameters.size(); ++i)
				{
					const DataTypeDefinition* type = assignStatementDataType(*ft.mParameters[i], nullptr);
					parameterTypes.push_back(type);
				}

				// Find out which function signature actually fits
				RMX_ASSERT(nullptr != mContext.mFunction, "Invalid function pointer");
				const Function* function = mContext.mFunction;
				String functionName(ft.mFunctionName);
				if (functionName.startsWith("base.") && functionName.getSubString(5, -1) == function->getName())
				{
					// Base call must use the same function signature as the current one
					CHECK_ERROR(parameterTypes.size() == function->getParameters().size(), "Base function call has different parameter count", mLineNumber);
					for (size_t i = 0; i < parameterTypes.size(); ++i)
					{
						CHECK_ERROR(parameterTypes[i] == function->getParameters()[i].mType, "Base function call has different parameter at index " + std::to_string(i), mLineNumber);
					}

					// Make this a call to itself, the runtime system will resolve that to a base call to whatever is the actual base function
					ft.mIsBaseCall = true;
				}
				else
				{
					const std::vector<Function*>& functions = mContext.mGlobalsLookup.getFunctionsByName(rmx::getMurmur2_64(functionName));
					CHECK_ERROR(!functions.empty(), "Unknown function name '" + ft.mFunctionName + "'", mLineNumber);

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
					CHECK_ERROR(bestPriority < 0xff000000, "No appropriate function overload found calling '" + ft.mFunctionName + "', the number or types of parameters passed are wrong", mLineNumber);
				}

				// TODO: Perform implicit casts for parameters here?

				ft.mFunction = function;
				ft.mDataType = function->getReturnType();
				break;
			}

			case Token::Type::MEMORY_ACCESS:
			{
				MemoryAccessToken& mat = token.as<MemoryAccessToken>();
				assignStatementDataType(*mat.mAddress, &PredefinedDataTypes::UINT_32);

				// Data type of the memory access token itself was already set on creation
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

	LocalVariable* TokenProcessing::findLocalVariable(const std::string& name)
	{
		for (LocalVariable* var : mContext.mLocalVariables)
		{
			if (var->getName() == name)
				return var;
		}
		return nullptr;
	}

}
