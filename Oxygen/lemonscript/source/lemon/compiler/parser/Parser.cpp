/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/parser/Parser.h"
#include "lemon/compiler/parser/ParserHelper.h"
#include "lemon/compiler/parser/ParserTokens.h"


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
			{ rmx::getMurmur2_64(String("float")),	&PredefinedDataTypes::FLOAT },
			{ rmx::getMurmur2_64(String("double")),	&PredefinedDataTypes::DOUBLE },
			{ rmx::getMurmur2_64(String("string")),	&PredefinedDataTypes::STRING }
		};

		static const std::map<uint64, Keyword> keywordLookup =
		{
			{ rmx::getMurmur2_64(String("function")),	Keyword::FUNCTION },
			{ rmx::getMurmur2_64(String("global")),		Keyword::GLOBAL },
			{ rmx::getMurmur2_64(String("constant")),	Keyword::CONSTANT },
			{ rmx::getMurmur2_64(String("define")),		Keyword::DEFINE },
			{ rmx::getMurmur2_64(String("declare")),	Keyword::DECLARE },
			{ rmx::getMurmur2_64(String("return")),		Keyword::RETURN },
			{ rmx::getMurmur2_64(String("call")),		Keyword::CALL },
			{ rmx::getMurmur2_64(String("jump")),		Keyword::JUMP },
			{ rmx::getMurmur2_64(String("break")),		Keyword::BREAK },
			{ rmx::getMurmur2_64(String("continue")),	Keyword::CONTINUE },
			{ rmx::getMurmur2_64(String("if")),			Keyword::IF },
			{ rmx::getMurmur2_64(String("else")),		Keyword::ELSE },
			{ rmx::getMurmur2_64(String("while")),		Keyword::WHILE },
			{ rmx::getMurmur2_64(String("for")),		Keyword::FOR },
			{ rmx::getMurmur2_64(String("addressof")),	Keyword::ADDRESSOF }
		};

		static const std::vector<const char*> reservedKeywords =
		{
			// These keywords are meant to be reserved for potential future use, and must not be used as identifiers
			"local",
			"auto",
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
			"enum",
			"struct",
			"class",
			"foreach",
			"in",
			"out",
			"ref",
			"typeof",
		};
		static std::map<uint64, std::string> reservedKeywordLookup;

		void analyseIdentifier(const std::string_view& identifier, ParserTokenList& outTokens, uint32 lineNumber)
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
			static const uint64 trueHash  = rmx::getMurmur2_64(std::string_view("true"));
			static const uint64 falseHash = rmx::getMurmur2_64(std::string_view("false"));
			if (identifierHash == trueHash || identifierHash == falseHash)
			{
				ConstantParserToken& token = outTokens.create<ConstantParserToken>();
				token.mValue = (identifierHash == trueHash);
				return;
			}

			// Just an identifier
			IdentifierParserToken& token = outTokens.create<IdentifierParserToken>();
			token.mName.set(identifier);
		}
	}


	void Parser::splitLineIntoTokens(std::string_view input, uint32 lineNumber, ParserTokenList& outTokens)
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
				const char* start = &input[pos];
				const size_t numberLength = ParserHelper::collectNumber(start, length - pos);
				RMX_ASSERT(numberLength > 0, "Failed to collect a number, even though the first cahracter is a digit");
				pos += numberLength;
				const int64 number = ParserHelper::parseInteger(start, numberLength, lineNumber);
				ConstantParserToken& token = outTokens.create<ConstantParserToken>();
				token.mValue = number;
			}
			else if (ParserHelper::isLetter(firstCharacter) || (firstCharacter == '_'))
			{
				// It is an identifier or keyword
				const char* start = &input[pos];
				const size_t identifierLength = ParserHelper::collectIdentifier(start, length - pos);
				pos += identifierLength;
				analyseIdentifier(std::string_view(start, identifierLength), outTokens, lineNumber);
			}
			else if (firstCharacter == '@')
			{
				// It is a label
				++pos;
				const char* start = &input[pos];
				const size_t identifierLength = ParserHelper::collectIdentifier(start, length - pos);
				pos += identifierLength;
				LabelParserToken& token = outTokens.create<LabelParserToken>();
				token.mName = std::string(start-1, identifierLength+1);
			}
			else if (ParserHelper::isOperatorCharacter(firstCharacter))
			{
				// It is a single operator or multiple of them
				const char* start = &input[pos];
				const size_t operatorsLength = ParserHelper::collectOperators(start, length - pos);

				Operator op;
				for (size_t i = 0; i < operatorsLength; )
				{
					const size_t operatorLength = ParserHelper::findOperator(&start[i], operatorsLength - i, op);
					CHECK_ERROR(operatorLength > 0, "Operator not recognized", lineNumber);

					if (op == Operator::BINARY_DIVIDE)
					{
						// Check for comments
						if (start[i+1] == '/')
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

				pos += operatorsLength;
			}
			else if (firstCharacter == '"')
			{
				// It is a string
				++pos;
				size_t charactersRead;
				ParserHelper::collectStringLiteral(&input[pos], length - pos, mBufferString, charactersRead, lineNumber);
				StringLiteralParserToken& token = outTokens.create<StringLiteralParserToken>();
				token.mString.set(mBufferString);
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
