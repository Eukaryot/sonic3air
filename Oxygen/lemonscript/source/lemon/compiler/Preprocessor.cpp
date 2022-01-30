/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Preprocessor.h"
#include "lemon/compiler/Parser.h"
#include "lemon/compiler/ParserHelper.h"
#include "lemon/compiler/ParserTokens.h"
#include "lemon/compiler/Token.h"
#include "lemon/compiler/TokenProcessing.h"
#include "lemon/compiler/TokenTypes.h"
#include "lemon/program/GlobalsLookup.h"


namespace lemon
{

	Preprocessor::Preprocessor(const GlobalCompilerConfig& config) :
		mConfig(config)
	{
	}

	void Preprocessor::processLines(std::vector<std::string_view>& lines)
	{
		struct PreprocessorBlock
		{
			bool mIgnored = false;
			bool mInheritedIgnored = false;
		};
		std::vector<PreprocessorBlock> openBlocks;

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
				// Check for preprocessor commands
				for (size_t pos = 0; pos < length; ++pos)
				{
					if (line[pos] == '#')
					{
						const String rest = line.substr(pos + 1);
						if (rest.startsWith("if ") && rest.length() >= 4)
						{
							const bool isTrue = evaluateConditionString(&(*rest)[3], rest.length() - 3, parser);
							const bool inheritedIgnored = (openBlocks.empty()) ? false : openBlocks.back().mIgnored;
							PreprocessorBlock& block = vectorAdd(openBlocks);
							block.mIgnored = !isTrue;
							block.mInheritedIgnored = inheritedIgnored;
						}
						else if (rest.startsWith("else"))
						{
							CHECK_ERROR(!openBlocks.empty(), "Found no #if for #else", mLineNumber);
							PreprocessorBlock& block = openBlocks.back();
							block.mIgnored = !block.mIgnored;
						}
						else if (rest.startsWith("endif"))
						{
							CHECK_ERROR(!openBlocks.empty(), "Found no #if for #endif", mLineNumber);
							openBlocks.pop_back();
						}
						else
						{
							CHECK_ERROR(false, "Invalid preprocessor command", mLineNumber);
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
			if (!openBlocks.empty() && (openBlocks.back().mIgnored || openBlocks.back().mInheritedIgnored))
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
						if (ParserHelper::findEndOfBlockComment(line.data(), line.length(), pos))
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
							pos += ParserHelper::skipStringLiteral(&line[pos+1], length-pos-1, mLineNumber) + 1;
							continue;
						}

						// Just skip all other characters
						++pos;
					}
				}
			}
		}

		// Using last line number for these errors
		CHECK_ERROR(openBlocks.empty(), "Not all preprocessor blocks closed", mLineNumber);
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

	bool Preprocessor::evaluateConditionString(const char* characters, size_t len, Parser& parser)
	{
		// Parse input
		ParserHelper::collectPreprocessorCondition(characters, len, mBufferString);
		CHECK_ERROR(!mBufferString.empty(), "Empty identifier after preprocessor #if", mLineNumber);
		ParserTokenList parserTokens;
		parser.splitLineIntoTokens(mBufferString, mLineNumber, parserTokens);

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
					CHECK_ERROR(false, "Keyword is not allowed in preprocessor condition", mLineNumber);
					break;
				}
				case ParserToken::Type::VARTYPE:
				{
					CHECK_ERROR(false, "Type is not allowed in preprocessor condition", mLineNumber);
					break;
				}
				case ParserToken::Type::OPERATOR:
				{
					tokenList.createBack<OperatorToken>().mOperator = parserToken.as<OperatorParserToken>().mOperator;
					break;
				}
				case ParserToken::Type::LABEL:
				{
					CHECK_ERROR(false, "Label is not allowed in preprocessor condition", mLineNumber);
					break;
				}
				case ParserToken::Type::PRAGMA:
				{
					CHECK_ERROR(false, "Pragma is not allowed in preprocessor condition", mLineNumber);
					break;
				}
				case ParserToken::Type::CONSTANT:
				{
					tokenList.createBack<ConstantToken>().mValue = parserToken.as<ConstantParserToken>().mValue;
					break;
				}
				case ParserToken::Type::STRING_LITERAL:
				{
					CHECK_ERROR(false, "String is not allowed in preprocessor condition", mLineNumber);
					break;
				}
				case ParserToken::Type::IDENTIFIER:
				{
					const std::string& identifier = parserToken.as<IdentifierParserToken>().mIdentifier;

					// Unknown preprocessor definitions are okay, they automatically evaluate to 0
					tokenList.createBack<ConstantToken>().mValue = (nullptr != mPreprocessorDefinitions) ? mPreprocessorDefinitions->getValue(identifier) : 0;
					break;
				}
			}
		}

		// Build the token tree
		{
			// TODO: This extra context stuff is somewhat unnecessary here
			GlobalsLookup globalsLookup;
			std::vector<LocalVariable*> localVariables;
			TokenProcessing::Context tokenProcessingContext(globalsLookup, localVariables, nullptr);
			TokenProcessing tokenProcessing(tokenProcessingContext, mConfig);
			tokenProcessing.processForPreprocessor(tokenList, mLineNumber);
		}

		// Now traverse the tree recursively
		CHECK_ERROR(tokenList.size() == 1, "Preprocessor condition must evaluate to a single statement", mLineNumber);
		CHECK_ERROR(tokenList[0].isStatement(), "Preprocessor condition must evaluate to a statement", mLineNumber);
		return (evaluateConditionToken(tokenList[0].as<StatementToken>()) != 0);
	}

	int64 Preprocessor::evaluateConditionToken(const StatementToken& token) const
	{
		switch (token.getType())
		{
			case Token::Type::CONSTANT:
			{
				return token.as<ConstantToken>().mValue;
			}

			case Token::Type::PARENTHESIS:
			{
				const ParenthesisToken& pt = token.as<ParenthesisToken>();
				CHECK_ERROR(pt.mParenthesisType == ParenthesisType::PARENTHESIS, "Brackets are not allowed in preprocessor condition", mLineNumber);
				CHECK_ERROR(pt.mContent.size() == 1, "Parenthesis must contain exactly one statement", mLineNumber);
				CHECK_ERROR(pt.mContent[0].isStatement(), "Parenthesis must contain a statement", mLineNumber);
				return evaluateConditionToken(pt.mContent[0].as<StatementToken>());
			}

			case Token::Type::BINARY_OPERATION:
			{
				const BinaryOperationToken& bot = token.as<BinaryOperationToken>();
				switch (bot.mOperator)
				{
					case Operator::LOGICAL_AND:				 return (evaluateConditionToken(*bot.mLeft) != 0) && (evaluateConditionToken(*bot.mRight) != 0) ? 1 : 0;
					case Operator::LOGICAL_OR:				 return (evaluateConditionToken(*bot.mLeft) != 0) || (evaluateConditionToken(*bot.mRight) != 0) ? 1 : 0;
					case Operator::COMPARE_EQUAL:			 return (evaluateConditionToken(*bot.mLeft) == evaluateConditionToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_NOT_EQUAL:		 return (evaluateConditionToken(*bot.mLeft) != evaluateConditionToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_LESS:			 return (evaluateConditionToken(*bot.mLeft) < evaluateConditionToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_LESS_OR_EQUAL:	 return (evaluateConditionToken(*bot.mLeft) <= evaluateConditionToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_GREATER:			 return (evaluateConditionToken(*bot.mLeft) > evaluateConditionToken(*bot.mRight)) ? 1 : 0;
					case Operator::COMPARE_GREATER_OR_EQUAL: return (evaluateConditionToken(*bot.mLeft) >= evaluateConditionToken(*bot.mRight)) ? 1 : 0;
					default:
						CHECK_ERROR(false, "Operator not allowed in preprocessor condition", mLineNumber);
						break;
				}
				break;
			}

			case Token::Type::UNARY_OPERATION:
			{
				const UnaryOperationToken& uot = token.as<UnaryOperationToken>();
				switch (uot.mOperator)
				{
					case Operator::UNARY_NOT:	 return (evaluateConditionToken(*uot.mArgument) == 0) ? 1 : 0;
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
		return false;
	}

}
