/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Parser.h"
#include "lemon/compiler/ParserHelper.h"
#include "lemon/compiler/ParserTokens.h"


namespace lemon
{
	namespace
	{

		static const std::map<uint64, const DataTypeDefinition*> varTypeLookup =
		{
			{ rmx::getMurmur2_64(String("void")),	&PredefinedDataTypes::VOID },
			{ rmx::getMurmur2_64(String("s8")),		&PredefinedDataTypes::INT_8 },
			{ rmx::getMurmur2_64(String("s16")),	&PredefinedDataTypes::INT_16 },
			{ rmx::getMurmur2_64(String("s32")),	&PredefinedDataTypes::INT_32 },
			{ rmx::getMurmur2_64(String("s64")),	&PredefinedDataTypes::INT_64 },
			{ rmx::getMurmur2_64(String("bool")),	&PredefinedDataTypes::UINT_8 },		// Only a synonym for u8
			{ rmx::getMurmur2_64(String("u8")),		&PredefinedDataTypes::UINT_8 },
			{ rmx::getMurmur2_64(String("u16")),	&PredefinedDataTypes::UINT_16 },
			{ rmx::getMurmur2_64(String("u32")),	&PredefinedDataTypes::UINT_32 },
			{ rmx::getMurmur2_64(String("u64")),	&PredefinedDataTypes::UINT_64 },
			{ rmx::getMurmur2_64(String("string")),	&PredefinedDataTypes::UINT_64 }		// Only a synonym for u64
		};

		static const std::map<uint64, Keyword> keywordLookup =
		{
			{ rmx::getMurmur2_64(String("function")),	Keyword::FUNCTION },
			{ rmx::getMurmur2_64(String("global")),		Keyword::GLOBAL },
			{ rmx::getMurmur2_64(String("constant")),	Keyword::CONSTANT },
			{ rmx::getMurmur2_64(String("define")),		Keyword::DEFINE },
			{ rmx::getMurmur2_64(String("return")),		Keyword::RETURN },
			{ rmx::getMurmur2_64(String("call")),		Keyword::CALL },
			{ rmx::getMurmur2_64(String("jump")),		Keyword::JUMP },
			{ rmx::getMurmur2_64(String("break")),		Keyword::BREAK },
			{ rmx::getMurmur2_64(String("continue")),	Keyword::CONTINUE },
			{ rmx::getMurmur2_64(String("if")),			Keyword::IF },
			{ rmx::getMurmur2_64(String("else")),		Keyword::ELSE },
			{ rmx::getMurmur2_64(String("while")),		Keyword::WHILE },
			{ rmx::getMurmur2_64(String("for")),		Keyword::FOR }
		};

		static const std::vector<const char*> reservedKeywords =
		{
			"local",
			"auto",
			"float",
			"double",
			"switch",
			"case",
			"select",
			"choose",
			"do",
			"const",
			"fixed",
			"static",
			"virtual",
			"override",
			"function",
			"enum",
			"struct",
			"class"
		};
		static std::map<uint64, std::string> reservedKeywordLookup;

		void analyseIdentifier(const std::string& identifier, ParserTokenList& outTokens, uint32 lineNumber)
		{
			const uint64 identifierHash = rmx::getMurmur2_64(identifier);

			// Check for variable type
			{
				const auto it = varTypeLookup.find(identifierHash);
				if (it != varTypeLookup.end())
				{
					VarTypeParserToken& token = outTokens.create<VarTypeParserToken>();
					token.mDataType = it->second;
					return;
				}
			}

			// Check for keyword
			{
				const auto it = keywordLookup.find(identifierHash);
				if (it != keywordLookup.end())
				{
					KeywordParserToken& token = outTokens.create<KeywordParserToken>();
					token.mKeyword = it->second;
					return;
				}
			}

			// Check for reserved identifier
			{
				if (reservedKeywordLookup.empty())
				{
					for (const std::string& str : reservedKeywords)
					{
						reservedKeywordLookup.emplace(rmx::getMurmur2_64(str), str);
					}
				}
				if (reservedKeywordLookup.count(identifierHash) > 0)
				{
					CHECK_ERROR(false, "Reserved keyword '" << reservedKeywordLookup[identifierHash] << "' cannot be used as an identifier, please rename", lineNumber);
					return;
				}
			}

			// Check for "true", "false"
			static const uint64 trueHash  = rmx::getMurmur2_64(std::string("true"));
			static const uint64 falseHash = rmx::getMurmur2_64(std::string("false"));
			if (identifierHash == trueHash || identifierHash == falseHash)
			{
				ConstantParserToken& token = outTokens.create<ConstantParserToken>();
				token.mValue = (identifierHash == trueHash);
				return;
			}

			// Just an identifier
			IdentifierParserToken& token = outTokens.create<IdentifierParserToken>();
			token.mIdentifier = identifier;
		}
	}


	void Parser::splitLineIntoTokens(const std::string_view& input, uint32 lineNumber, ParserTokenList& outTokens)
	{
		const size_t length = input.length();

		// Do the actual parsing
		for (size_t pos = 0; pos < length; )
		{
			// Look at next character
			const char firstCharacter = input[pos];

			if (firstCharacter == '{')
			{
				outTokens.create<KeywordParserToken>().mKeyword = Keyword::BLOCK_BEGIN;
				++pos;
			}
			else if (firstCharacter == '}')
			{
				outTokens.create<KeywordParserToken>().mKeyword = Keyword::BLOCK_END;
				++pos;
			}
			else if (ParserHelper::isDigit(firstCharacter))
			{
				// It is a number
				ParserHelper::collectNumber(&input[pos], length - pos, mBufferString);
				pos += mBufferString.length();
				const int64 number = ParserHelper::parseInteger(mBufferString.c_str(), mBufferString.length(), lineNumber);

				ConstantParserToken& token = outTokens.create<ConstantParserToken>();
				token.mValue = number;
			}
			else if (ParserHelper::isLetter(firstCharacter) || (firstCharacter == '_'))
			{
				// It is an identifier or keyword
				ParserHelper::collectIdentifier(&input[pos], length - pos, mBufferString);
				pos += mBufferString.length();

				analyseIdentifier(mBufferString, outTokens, lineNumber);
			}
			else if (firstCharacter == '@')
			{
				// It is a label
				++pos;
				ParserHelper::collectIdentifier(&input[pos], length - pos, mBufferString);
				pos += mBufferString.length();

				LabelParserToken& token = outTokens.create<LabelParserToken>();
				token.mName = '@' + mBufferString;
			}
			else if (ParserHelper::isOperatorCharacter(firstCharacter))
			{
				// It is a single operator or multiple of them
				ParserHelper::collectOperators(&input[pos], length - pos, mBufferString);

				Operator op;
				for (size_t i = 0; i < mBufferString.length(); )
				{
					const size_t operatorLength = ParserHelper::findOperator(&mBufferString[i], mBufferString.length() - i, op);
					CHECK_ERROR(operatorLength > 0, "Operator not recognized", lineNumber);

					if (op == Operator::BINARY_DIVIDE)
					{
						// Check for comments
						if (mBufferString[i+1] == '/')
						{
							// Line comment: Pragma or not?
							pos += i+2;
							if (pos < input.size() && input[pos] == '#')
							{
								// Jump over '#' and ignore whitespace
								do
								{
									++pos;
								}
								while (input[pos] == ' ' || input[pos] == '\t');

								PragmaParserToken& token = outTokens.create<PragmaParserToken>();
								token.mContent = input.substr(pos);
							}

							// We're done with this line
							return;
						}
					}

					OperatorParserToken& token = outTokens.create<OperatorParserToken>();
					token.mOperator = op;

					i += operatorLength;
				}

				pos += mBufferString.length();
			}
			else if (firstCharacter == '"')
			{
				// It is a string
				++pos;
				size_t charactersRead;
				ParserHelper::collectStringLiteral(&input[pos], length - pos, mBufferString, charactersRead, lineNumber);
				StringLiteralParserToken& token = outTokens.create<StringLiteralParserToken>();
				token.mString = mBufferString;
				pos += charactersRead + 1;
			}
			else
			{
				// Just skip all other characters
				++pos;
			}
		}
	}

}
