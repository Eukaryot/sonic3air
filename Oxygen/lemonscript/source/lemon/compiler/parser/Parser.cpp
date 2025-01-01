/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
			{ rmx::constMurmur2_64("void"),		&PredefinedDataTypes::VOID },
			{ rmx::constMurmur2_64("s8"),		&PredefinedDataTypes::INT_8 },
			{ rmx::constMurmur2_64("s16"),		&PredefinedDataTypes::INT_16 },
			{ rmx::constMurmur2_64("s32"),		&PredefinedDataTypes::INT_32 },
			{ rmx::constMurmur2_64("s64"),		&PredefinedDataTypes::INT_64 },
			{ rmx::constMurmur2_64("bool"),		&PredefinedDataTypes::UINT_8 },		// Only a synonym for u8
			{ rmx::constMurmur2_64("u8"),		&PredefinedDataTypes::UINT_8 },
			{ rmx::constMurmur2_64("u16"),		&PredefinedDataTypes::UINT_16 },
			{ rmx::constMurmur2_64("u32"),		&PredefinedDataTypes::UINT_32 },
			{ rmx::constMurmur2_64("u64"),		&PredefinedDataTypes::UINT_64 },
			{ rmx::constMurmur2_64("float"),	&PredefinedDataTypes::FLOAT },
			{ rmx::constMurmur2_64("double"),	&PredefinedDataTypes::DOUBLE },
			{ rmx::constMurmur2_64("string"),	&PredefinedDataTypes::STRING }
		};

		static const std::map<uint64, Keyword> keywordLookup =
		{
			{ rmx::constMurmur2_64("function"),		Keyword::FUNCTION },
			{ rmx::constMurmur2_64("global"),		Keyword::GLOBAL },
			{ rmx::constMurmur2_64("constant"),		Keyword::CONSTANT },
			{ rmx::constMurmur2_64("define"),		Keyword::DEFINE },
			{ rmx::constMurmur2_64("declare"),		Keyword::DECLARE },
			{ rmx::constMurmur2_64("return"),		Keyword::RETURN },
			{ rmx::constMurmur2_64("call"),			Keyword::CALL },
			{ rmx::constMurmur2_64("jump"),			Keyword::JUMP },
			{ rmx::constMurmur2_64("break"),		Keyword::BREAK },
			{ rmx::constMurmur2_64("continue"),		Keyword::CONTINUE },
			{ rmx::constMurmur2_64("if"),			Keyword::IF },
			{ rmx::constMurmur2_64("else"),			Keyword::ELSE },
			{ rmx::constMurmur2_64("while"),		Keyword::WHILE },
			{ rmx::constMurmur2_64("for"),			Keyword::FOR },
			{ rmx::constMurmur2_64("addressof"),	Keyword::ADDRESSOF },
			{ rmx::constMurmur2_64("makeCallable"), Keyword::MAKECALLABLE }
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
			constexpr uint64 trueHash  = rmx::constMurmur2_64("true");
			constexpr uint64 falseHash = rmx::constMurmur2_64("false");
			if (identifierHash == trueHash || identifierHash == falseHash)
			{
				ConstantParserToken& token = outTokens.create<ConstantParserToken>();
				token.mValue.set(identifierHash == trueHash);
				token.mBaseType = BaseType::INT_CONST;
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
			else if (ParserHelper::isDigitOrDot(firstCharacter))
			{
				// It is a number (integer or floating point)
				const std::string_view rest = input.substr(pos);
				const ParserHelper::ParseNumberResult result = ParserHelper::collectNumber(rest);
				switch (result.mType)
				{
					case ParserHelper::ParseNumberResult::Type::INTEGER:
					{
						ConstantParserToken& token = outTokens.create<ConstantParserToken>();
						token.mValue = result.mValue;
						token.mBaseType = BaseType::INT_CONST;
						break;
					}

					case ParserHelper::ParseNumberResult::Type::FLOAT:
					{
						ConstantParserToken& token = outTokens.create<ConstantParserToken>();
						token.mValue = result.mValue;
						token.mBaseType = BaseType::FLOAT;
						break;
					}

					case ParserHelper::ParseNumberResult::Type::DOUBLE:
					{
						ConstantParserToken& token = outTokens.create<ConstantParserToken>();
						token.mValue = result.mValue;
						token.mBaseType = BaseType::DOUBLE;
						break;
					}

					default:
						CHECK_ERROR(false, "Invalid number '" << rest.substr(0, result.mBytesRead) << "'", lineNumber);
				}
				pos += result.mBytesRead;
			}
			else if (ParserHelper::isLetter(firstCharacter) || (firstCharacter == '_'))
			{
				// It is an identifier or keyword
				const std::string_view rest = input.substr(pos);
				const size_t identifierLength = ParserHelper::collectIdentifier(rest);
				pos += identifierLength;
				analyseIdentifier(rest.substr(0, identifierLength), outTokens, lineNumber);
			}
			else if (firstCharacter == '@')
			{
				// It is a label
				const std::string_view rest = input.substr(pos);
				++pos;
				const size_t identifierLength = ParserHelper::collectIdentifier(rest.substr(1));
				pos += identifierLength;
				LabelParserToken& token = outTokens.create<LabelParserToken>();
				token.mName = rest.substr(0, identifierLength + 1);
			}
			else if (ParserHelper::isOperatorCharacter(firstCharacter))
			{
				// It is a single operator or multiple of them
				const std::string_view rest = input.substr(pos);
				const size_t operatorsLength = ParserHelper::collectOperators(rest);
				const char* start = &input[pos];

				Operator op;
				for (size_t i = 0; i < operatorsLength; )
				{
					const size_t operatorLength = ParserHelper::findOperator(input.substr(pos + i, operatorsLength - i), op);
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
				ParserHelper::collectStringLiteral(input.substr(pos), mBufferString, charactersRead, lineNumber);
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
