/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Preprocessor.h"
#include "lemon/compiler/Token.h"
#include "lemon/compiler/TokenTypes.h"
#include "lemon/compiler/parser/Parser.h"
#include "lemon/compiler/parser/ParserHelper.h"
#include "lemon/compiler/parser/ParserTokens.h"
#include "lemon/compiler/frontend/TokenProcessing.h"
#include "lemon/program/GlobalsLookup.h"


namespace lemon
{

	Preprocessor::Preprocessor(const CompileOptions& compileOptions, TokenProcessing& tokenProcessing) :
		mCompileOptions(compileOptions),
		mTokenProcessing(tokenProcessing)
	{
	}

	void Preprocessor::processLines(std::vector<std::string_view>& lines)
	{
		struct BlockStack
		{
			struct Block
			{
				bool mCondition = true;				// Whether the condition of this block is true, i.e. the block is going to be considered (if true) or ignored (if false)
				bool mInheritedCondition = true;	// Set if the condition of all blocks above is true; not including this block's own condition
				bool mCollapseParent = false;		// If set, removing this Block will also remove the parent block
			};
			std::vector<Block> mOpenBlocks;
			inline bool shouldConsiderContent() const  { return mOpenBlocks.empty() || (mOpenBlocks.back().mCondition && mOpenBlocks.back().mInheritedCondition); }
		};

		BlockStack blockStack;
		Parser parser;
		bool isInBlockComment = false;
		size_t blockCommentStart = 0;
		mLineNumber = 0;

		for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
		{
			std::string_view& line = lines[lineIndex];
			size_t length = line.length();
			mLineNumber = (uint32)lineIndex + 1;
			std::string* modifiedLine = nullptr;	// Gets set if this line gets modified

			if (isInBlockComment)
			{
				// For this line...
				blockCommentStart = 0;
			}
			else
			{
				// Check for preprocessor directives
				for (size_t pos = 0; pos < length; ++pos)
				{
					if (line[pos] == '#')
					{
						const std::string_view rest = line.substr(pos + 1);
						if (rmx::startsWith(rest, "if ") && rest.length() >= 4)
						{
							const bool isTrue = evaluateConditionString(rest.substr(3), parser);
							const bool inheritedCondition = blockStack.shouldConsiderContent();
							BlockStack::Block& block = vectorAdd(blockStack.mOpenBlocks);
							block.mCondition = isTrue;
							block.mInheritedCondition = inheritedCondition;
						}
						else if (rmx::startsWith(rest, "else"))
						{
							CHECK_ERROR(!blockStack.mOpenBlocks.empty(), "Found no #if for #else", mLineNumber);
							BlockStack::Block& block = blockStack.mOpenBlocks.back();
							block.mCondition = !block.mCondition;
						}
						else if (rmx::startsWith(rest, "elif") && rest.length() >= 6)
						{
							CHECK_ERROR(!blockStack.mOpenBlocks.empty(), "Found no #if for #elif", mLineNumber);
							BlockStack::Block& parentBlock = blockStack.mOpenBlocks.back();
							parentBlock.mCondition = !parentBlock.mCondition;
							const bool isTrue = evaluateConditionString(rest.substr(5), parser);
							const bool inheritedCondition = blockStack.shouldConsiderContent();
							BlockStack::Block& block = vectorAdd(blockStack.mOpenBlocks);
							block.mCondition = isTrue;
							block.mInheritedCondition = inheritedCondition;
							block.mCollapseParent = true;
						}
						else if (rmx::startsWith(rest, "endif"))
						{
							CHECK_ERROR(!blockStack.mOpenBlocks.empty(), "Found no #if for #endif", mLineNumber);
							while (!blockStack.mOpenBlocks.empty() && blockStack.mOpenBlocks.back().mCollapseParent)
							{
								blockStack.mOpenBlocks.pop_back();
							}
							CHECK_ERROR(!blockStack.mOpenBlocks.empty(), "Something went wrong in evaluating #endif", mLineNumber);
							blockStack.mOpenBlocks.pop_back();
						}
						else if (blockStack.shouldConsiderContent())
						{
							if (rmx::startsWith(rest, "define ") && rest.length() >= 8)
							{
								processDefinition(rest.substr(7), parser);
							}
							else if (rmx::startsWith(rest, "error ") && rest.length() >= 7)
							{
								CHECK_ERROR(false, rest.substr(6), mLineNumber);
							}
							else
							{
								CHECK_ERROR(false, "Invalid preprocessor directive", mLineNumber);
							}
						}

						// Clear this line, it should be ignored by the parser
						line = std::string_view();
						length = 0;
					}
					else
					{
						// Whitespaces are okay, everything else leads to a break
						if (line[pos] != ' ' && line[pos] != '\t')
							break;
					}
				}

				// Do nothing if line was cleared
				if (length == 0)
					continue;
			}

			// Check if inside an ignored region
			if (!blockStack.shouldConsiderContent())
			{
				// Clear this line, it should be ignored by the parser
				line = std::string_view();
				length = 0;
			}
			else
			{
				// Check for block comments
				for (size_t pos = 0; pos < length; )
				{
					if (isInBlockComment)
					{
						if (ParserHelper::findEndOfBlockComment(line, pos))
						{
							// Block comment ends here
							eraseFromLine(line, modifiedLine, blockCommentStart, pos - blockCommentStart);
							length -= (pos - blockCommentStart);
							pos = blockCommentStart;
							isInBlockComment = false;
							continue;
						}
						else
						{
							// Block comment exceeds this line
							line.remove_suffix(line.length() - blockCommentStart);
							break;
						}
					}
					else
					{
						// Check for comments
						if (line[pos] == '/' && pos < length-1)
						{
							if (line[pos+1] == '/')
							{
								// Ignore the rest of this line (i.e. do not scan for strings, block comment starts, etc.)
								//  -> But leave it as it is because the parser will have to evaluate pragmas
								break;
							}
							else if (line[pos+1] == '*')
							{
								// Start block comment
								isInBlockComment = true;
								eraseFromLine(line, modifiedLine, pos, 2);		// This is needed in case the line consists of only the block comment start
								length -= 2;
								blockCommentStart = pos;
								continue;
							}
						}
						else if (line[pos] == '"')
						{
							// It is a string
							pos += ParserHelper::skipStringLiteral(line.substr(pos+1), mLineNumber) + 1;
							continue;
						}

						// Just skip all other characters
						++pos;
					}
				}
			}
		}

		// Using last line number for these errors
		CHECK_ERROR(blockStack.mOpenBlocks.empty(), "Not all preprocessor blocks closed", mLineNumber);
		CHECK_ERROR(!isInBlockComment, "Still inside a block comment at end of file", mLineNumber);
	}

	void Preprocessor::eraseFromLine(std::string_view& line, std::string*& modifiedLine, size_t offset, size_t count)
	{
		if (count == 0 || offset >= line.length())
			return;

		if (nullptr == modifiedLine)
		{
			// Is this something that can be handled with only making changes of the string_view's range?
			if (offset == 0)
			{
				line.remove_prefix(count);
				return;
			}
			else if (offset + count >= line.length())
			{
				line.remove_suffix(line.length() - offset);
				return;
			}

			// Add new modified line
			modifiedLine = &mModifiedLines.createObject();
		}

		*modifiedLine = line;
		modifiedLine->erase(offset, count);
		line = std::string_view(*modifiedLine);
	}

	bool Preprocessor::evaluateConditionString(std::string_view input, Parser& parser)
	{
		// Parse input
		ParserHelper::collectPreprocessorStatement(input, mBufferString);
		CHECK_ERROR(!mBufferString.empty(), "Empty identifier after preprocessor #if", mLineNumber);
		ParserTokenList parserTokens;
		parser.splitLineIntoTokens(mBufferString, mLineNumber, parserTokens);

		return (evaluateConstantExpression(parserTokens) != 0);
	}

	void Preprocessor::processDefinition(std::string_view input, Parser& parser)
	{
		// Parse input
		ParserHelper::collectPreprocessorStatement(input, mBufferString);
		CHECK_ERROR(!mBufferString.empty(), "Empty identifier after preprocessor #if", mLineNumber);
		ParserTokenList parserTokens;
		parser.splitLineIntoTokens(mBufferString, mLineNumber, parserTokens);

		// Check for identifier
		CHECK_ERROR(!parserTokens.empty(), "Expected an identifier after #define", mLineNumber);
		CHECK_ERROR(parserTokens[0].isA<IdentifierParserToken>(), "Expected an identifier after #define", mLineNumber);
		const FlyweightString identifierName = parserTokens[0].as<IdentifierParserToken>().mName;

		// Check for value
		int64 value = 1;
		if (parserTokens.size() >= 2)
		{
			CHECK_ERROR(parserTokens.size() >= 3, "Assignment for #define expects using =", mLineNumber);
			CHECK_ERROR(parserTokens[1].isA<OperatorParserToken>() && parserTokens[1].as<OperatorParserToken>().mOperator == Operator::ASSIGN, "Assignment for #define expects using =", mLineNumber);
			parserTokens.erase(0, 2);
			value = evaluateConstantExpression(parserTokens);
		}

		if (nullptr != mPreprocessorDefinitions)
		{
			mPreprocessorDefinitions->setDefinition(identifierName, value);
		}
	}

	int64 Preprocessor::evaluateConstantExpression(const ParserTokenList& parserTokens) const
	{
		// Convert to tokens
		TokenList tokenList;
		tokenList.reserve(parserTokens.size());
		for (size_t i = 0; i < parserTokens.size(); ++i)
		{
			ParserToken& parserToken = parserTokens[i];
			switch (parserToken.getType())
			{
				case ParserToken::Type::KEYWORD:
				{
					CHECK_ERROR(false, "Keyword is not allowed in preprocessor statement", mLineNumber);
					break;
				}
				case ParserToken::Type::VARTYPE:
				{
					CHECK_ERROR(false, "Type is not allowed in preprocessor statement", mLineNumber);
					break;
				}
				case ParserToken::Type::OPERATOR:
				{
					tokenList.createBack<OperatorToken>().mOperator = parserToken.as<OperatorParserToken>().mOperator;
					break;
				}
				case ParserToken::Type::LABEL:
				{
					CHECK_ERROR(false, "Label is not allowed in preprocessor statement", mLineNumber);
					break;
				}
				case ParserToken::Type::PRAGMA:
				{
					CHECK_ERROR(false, "Pragma is not allowed in preprocessor statement", mLineNumber);
					break;
				}
				case ParserToken::Type::CONSTANT:
				{
					tokenList.createBack<ConstantToken>().mValue = parserToken.as<ConstantParserToken>().mValue;
					break;
				}
				case ParserToken::Type::STRING_LITERAL:
				{
					CHECK_ERROR(false, "String is not allowed in preprocessor statement", mLineNumber);
					break;
				}
				case ParserToken::Type::IDENTIFIER:
				{
					const uint64 hash = parserToken.as<IdentifierParserToken>().mName.getHash();

					// Unknown preprocessor definitions are okay, they automatically evaluate to 0
					tokenList.createBack<ConstantToken>().mValue.set<int64>((nullptr != mPreprocessorDefinitions) ? mPreprocessorDefinitions->getValue(hash) : 0);
					break;
				}
			}
		}

		// Build the token tree
		{
			// TODO: This extra context stuff is somewhat unnecessary here
			std::vector<LocalVariable*> localVariables;
			std::vector<Constant> localConstants;
			std::vector<ConstantArray*> localConstantArrays;
			mTokenProcessing.mContext.mFunction = nullptr;
			mTokenProcessing.mContext.mLocalVariables = &localVariables;
			mTokenProcessing.mContext.mLocalConstants = &localConstants;
			mTokenProcessing.mContext.mLocalConstantArrays = &localConstantArrays;
			mTokenProcessing.processForPreprocessor(tokenList, mLineNumber);
		}

		// Now traverse the tree recursively
		CHECK_ERROR(tokenList.size() == 1, "Preprocessor condition must evaluate to a single statement", mLineNumber);
		CHECK_ERROR(tokenList[0].isStatement(), "Preprocessor condition must evaluate to a statement", mLineNumber);
		return evaluateConstantToken(tokenList[0].as<StatementToken>());
	}

	int64 Preprocessor::evaluateConstantToken(const StatementToken& token) const
	{
		switch (token.getType())
		{
			case ConstantToken::TYPE:
			{
				return token.as<ConstantToken>().mValue.get<int64>();
			}

			case ParenthesisToken::TYPE:
			{
				const ParenthesisToken& pt = token.as<ParenthesisToken>();
				CHECK_ERROR(pt.mParenthesisType == ParenthesisType::PARENTHESIS, "Brackets are not allowed in preprocessor condition", mLineNumber);
				CHECK_ERROR(pt.mContent.size() == 1, "Parenthesis must contain exactly one statement", mLineNumber);
				CHECK_ERROR(pt.mContent[0].isStatement(), "Parenthesis must contain a statement", mLineNumber);
				return evaluateConstantToken(pt.mContent[0].as<StatementToken>());
			}

			case BinaryOperationToken::TYPE:
			{
				const BinaryOperationToken& bot = token.as<BinaryOperationToken>();
				switch (bot.mOperator)
				{
					case Operator::LOGICAL_AND:				 return (evaluateConstantToken(*bot.mLeft) != 0) && (evaluateConstantToken(*bot.mRight) != 0) ? 1 : 0;
					case Operator::LOGICAL_OR:				 return (evaluateConstantToken(*bot.mLeft) != 0) || (evaluateConstantToken(*bot.mRight) != 0) ? 1 : 0;
					case Operator::COMPARE_EQUAL:			 return (evaluateConstantToken(*bot.mLeft) == evaluateConstantToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_NOT_EQUAL:		 return (evaluateConstantToken(*bot.mLeft) != evaluateConstantToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_LESS:			 return (evaluateConstantToken(*bot.mLeft) < evaluateConstantToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_LESS_OR_EQUAL:	 return (evaluateConstantToken(*bot.mLeft) <= evaluateConstantToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_GREATER:			 return (evaluateConstantToken(*bot.mLeft) > evaluateConstantToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_GREATER_OR_EQUAL: return (evaluateConstantToken(*bot.mLeft) >= evaluateConstantToken(*bot.mRight)) ? 1 : 0;
					default:
						CHECK_ERROR(false, "Operator not allowed in preprocessor condition", mLineNumber);
						break;
				}
				break;
			}

			case UnaryOperationToken::TYPE:
			{
				const UnaryOperationToken& uot = token.as<UnaryOperationToken>();
				switch (uot.mOperator)
				{
					case Operator::UNARY_NOT:  return (evaluateConstantToken(*uot.mArgument) == 0) ? 1 : 0;
					default:
						CHECK_ERROR(false, "Operator not allowed in preprocessor condition", mLineNumber);
						break;
				}
				break;
			}

			default:
				CHECK_ERROR(false, "Token type not supported", mLineNumber);
				break;
		}
		return 0;
	}

}
