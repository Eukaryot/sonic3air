/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/parser/ParserHelper.h"


namespace lemon
{

	ParserHelper::OperatorLookup ParserHelper::mOperatorLookup;

	bool ParserHelper::OperatorLookup::isOperatorCharacter(char character)
	{
		if (!mInitialized)
		{
			initialize();
		}

		if ((unsigned)character < 32 || (unsigned)character >= 128)
			return false;

		return mIsOperatorCharacter[character - 32];
	}

	size_t ParserHelper::OperatorLookup::lookup(std::string_view input, Operator& outOperator)
	{
		if (!mInitialized)
		{
			initialize();
		}

		size_t outLength = 0;
		const Entry* parentEntry = nullptr;
		for (size_t i = 0; i < input.length(); ++i)
		{
			const char character = input[i];
			if ((unsigned)character < 32 || (unsigned)character >= 128)
				break;

			const Entry* currentEntry = nullptr;
			if (nullptr == parentEntry)
			{
				currentEntry = mTopLevelEntries[character - 32];
				RMX_ASSERT(nullptr != currentEntry, "Invalid top level entry");
			}
			else
			{
				for (Entry* child : parentEntry->mChildren)
				{
					if (child->mCharacter == character)
					{
						currentEntry = child;
						break;
					}
				}

				if (nullptr == currentEntry)
					break;
			}

			if (currentEntry->mOperator != Operator::_NUM_OPERATORS)
			{
				outOperator = currentEntry->mOperator;
				outLength = i + 1;
			}

			parentEntry = currentEntry;
		}

		return outLength;
	}

	void ParserHelper::OperatorLookup::initialize()
	{
		if (mInitialized)
			return;

		const std::vector<std::pair<std::string_view, Operator>> operatorStrings =
		{
			{ "=",   Operator::ASSIGN },
			{ "+=",  Operator::ASSIGN_PLUS },
			{ "-=",  Operator::ASSIGN_MINUS },
			{ "*=",  Operator::ASSIGN_MULTIPLY },
			{ "/=",  Operator::ASSIGN_DIVIDE },
			{ "%=",  Operator::ASSIGN_MODULO },
			{ "<<=", Operator::ASSIGN_SHIFT_LEFT },
			{ ">>=", Operator::ASSIGN_SHIFT_RIGHT },
			{ "&=",  Operator::ASSIGN_AND },
			{ "|=",  Operator::ASSIGN_OR },
			{ "^=",  Operator::ASSIGN_XOR },
			{ "+",   Operator::BINARY_PLUS },
			{ "-",   Operator::BINARY_MINUS },
			{ "*",   Operator::BINARY_MULTIPLY },
			{ "/",   Operator::BINARY_DIVIDE },
			{ "%",   Operator::BINARY_MODULO },
			{ "<<",  Operator::BINARY_SHIFT_LEFT },
			{ ">>",  Operator::BINARY_SHIFT_RIGHT },
			{ "&",   Operator::BINARY_AND },
			{ "|",   Operator::BINARY_OR },
			{ "^",   Operator::BINARY_XOR },
			{ "&&",  Operator::LOGICAL_AND },
			{ "||",  Operator::LOGICAL_OR },
			{ "!",   Operator::UNARY_NOT },
			{ "~",   Operator::UNARY_BITNOT },
			{ "--",  Operator::UNARY_DECREMENT },
			{ "++",  Operator::UNARY_INCREMENT },
			{ "==",  Operator::COMPARE_EQUAL },
			{ "!=",  Operator::COMPARE_NOT_EQUAL },
			{ "<",   Operator::COMPARE_LESS },
			{ "<=",  Operator::COMPARE_LESS_OR_EQUAL },
			{ ">",   Operator::COMPARE_GREATER },
			{ ">=",  Operator::COMPARE_GREATER_OR_EQUAL },
			{ "?",   Operator::QUESTIONMARK },
			{ ":",   Operator::COLON },
			{ ";",   Operator::SEMICOLON_SEPARATOR },
			{ ",",   Operator::COMMA_SEPARATOR },
			{ "(",   Operator::PARENTHESIS_LEFT },
			{ ")",   Operator::PARENTHESIS_RIGHT },
			{ "[",   Operator::BRACKET_LEFT },
			{ "]",   Operator::BRACKET_RIGHT }
		};

		for (int i = 0; i < 96; ++i)
		{
			mTopLevelEntries[i] = nullptr;
			mIsOperatorCharacter[i] = false;
		}

		for (const auto& pair : operatorStrings)
		{
			const std::string_view& string = pair.first;
			Entry* parentEntry = nullptr;
			for (size_t i = 0; i < string.length(); ++i)
			{
				const char character = string[i];
				mIsOperatorCharacter[character - 32] = true;

				Entry** currentEntryPtr = nullptr;
				if (nullptr == parentEntry)
				{
					currentEntryPtr = &mTopLevelEntries[character - 32];
				}
				else
				{
					for (size_t k = 0; k < parentEntry->mChildren.size(); ++k)
					{
						if (parentEntry->mChildren[k]->mCharacter == character)
						{
							currentEntryPtr = &parentEntry->mChildren[k];
							break;
						}
					}

					if (nullptr == currentEntryPtr)
					{
						currentEntryPtr = &vectorAdd(parentEntry->mChildren);
					}
				}

				Entry* currentEntry = *currentEntryPtr;
				if (nullptr == currentEntry)
				{
					currentEntry = &mAllEntries.createObject();
					currentEntry->mCharacter = character;
					*currentEntryPtr = currentEntry;
				}

				if (string.length() == i + 1)
				{
					currentEntry->mOperator = pair.second;
					break;
				}

				parentEntry = currentEntry;
			}
		}

		mInitialized = true;
	}

	void ParserHelper::collectStringLiteral(std::string_view input, std::string& output, size_t& outCharactersRead, uint32 lineNumber)
	{
		output.clear();
		size_t pos = 0;
		for (; pos < input.length(); ++pos)
		{
			char ch = input[pos];

			// Use backslash as escape character
			if (ch == '\\' && pos+1 < input.length())
			{
				++pos;
				ch = input[pos];
				if (ch == 'n')
					ch = '\n';
				else if (ch == 'r')
					ch = '\r';
				else if (ch == 't')
					ch = '\t';
			}
			else if (ch == '"')
			{
				break;
			}

			output += ch;
		}
		CHECK_ERROR(pos < input.length(), "String literal exceeds line", lineNumber);
		outCharactersRead = pos;
	}

	void ParserHelper::collectPreprocessorStatement(std::string_view input, std::string& output)
	{
		output.clear();
		for (size_t pos = 0; pos < input.length(); ++pos)
		{
			const char ch = input[pos];
			if (ch == '"')
			{
				for (++pos; pos < input.length(); ++pos)
				{
					if (input[pos] == '"')
						break;
				}
				break;
			}
			if (ch == '/' && pos < input.length()-1)
			{
				if (input[pos+1] == '/')
					break;
				if (input[pos+1] == '*')
				{
					pos += 2;
					if (findEndOfBlockComment(input, pos))
					{
						--pos;	// Go back one characters, as the for-loop will skip it again
						continue;
					}
					break;
				}
			}
			output += ch;
		}
	}

	bool ParserHelper::findEndOfBlockComment(std::string_view input, size_t& pos)
	{
		for (; pos + 1 < input.length(); ++pos)
		{
			if (input[pos] == '*' && input[pos + 1] == '/')
			{
				pos += 2;
				return true;
			}
		}
		return false;
	}

	size_t ParserHelper::skipStringLiteral(std::string_view input, uint32 lineNumber)
	{
		for (size_t pos = 0; pos < input.length(); ++pos)
		{
			if (input[pos] == '\\' && pos+1 < input.length())
			{
				++pos;
			}
			else if (input[pos] == '"')
			{
				return pos + 1;
			}
		}
		CHECK_ERROR(false, "String literal exceeds line", lineNumber);
		return input.length();
	}

	ParserHelper::ParseNumberResult ParserHelper::collectNumber(std::string_view input)
	{
		ParseNumberResult result;

		// Collect number characters, and also other characters that must not appear immediately after a number
		size_t len = 0;
		for (; len < input.length(); ++len)
		{
			if (!mLookup.mIsIdentifierCharacter[(uint8)input[len]])
				break;
		}

		std::string_view collectedString = input.substr(0, len);
		result.mBytesRead = len;

		// First check for hexadecimal integer
		if (len >= 3 && collectedString[0] == '0' && collectedString[1] == 'x')
		{
			collectedString = collectedString.substr(2);
			int64 number = 0;
			uint8 errorCheck = 0;
			for (char ch : collectedString)
			{
				const uint8 value = mDigitLookupHex.getValueByCharacter(ch);
				number = number * 16 + (int64)value;
				errorCheck |= value;
			}

			if ((errorCheck & 0x80) == 0)
			{
				result.mType = ParseNumberResult::Type::INTEGER;
				result.mValue = number;
			}

			// Return in any case, as we already know from the prefix that it can't be anything different than hexadecimal encoding
			return result;
		}

		// Next check for decimal integer
		{
			int64 number = 0;
			uint8 errorCheck = 0;
			for (char ch : collectedString)
			{
				const uint8 value = mDigitLookupDec.getValueByCharacter(ch);
				number = number * 10 + (int64)value;
				errorCheck |= value;
			}

			if ((errorCheck & 0x80) == 0)
			{
				result.mType = ParseNumberResult::Type::INTEGER;
				result.mValue = number;
				return result;
			}
		}

		// Check if it's a floating point type
		{
			if (collectedString.length() < 2)		// Can't be a single digit, that would be an integer
				return result;

			// Handle the special case that there's a minus after the string that is the the exponent's sign and thus part of the number
			if ((collectedString.back() == 'e' || collectedString.back() == 'E') && (len < input.length() && input[len] == '-'))
			{
				// Collect more characters
				++len;
				for (; len < input.length(); ++len)
				{
					if (!mLookup.mIsIdentifierCharacter[(uint8)input[len]])
						break;
				}
				collectedString = input.substr(0, len);
				result.mBytesRead = len;
			}

			bool isFloat = false;
			if (collectedString.back() == 'f')
			{
				collectedString = collectedString.substr(0, collectedString.length()-1);
				isFloat = true;
			}

			std::string_view integerString;
			std::string_view fractionalString;
			std::string_view exponentString;
			{
				const size_t dotPos = collectedString.find_first_of('.');
				const size_t expPos = collectedString.find_first_of("eE");
				if (dotPos == std::string_view::npos)
				{
					if (expPos == std::string_view::npos)
					{
						return result;
					}

					// It's something like "2e5"
					integerString = collectedString.substr(0, expPos);
					exponentString = collectedString.substr(expPos + 1);
				}
				else
				{
					integerString = collectedString.substr(0, dotPos);

					if (expPos == std::string_view::npos)
					{
						// It's something like "2.5"
						fractionalString = collectedString.substr(dotPos + 1);
					}
					else
					{
						if (dotPos > expPos)
							return result;

						// It's something like "2.1e5"
						fractionalString = collectedString.substr(dotPos + 1, expPos - dotPos - 1);
						exponentString = collectedString.substr(expPos + 1);
					}
				}
			}

			uint8 errorCheck = 0;
			uint64 integerPart = 0;
			for (char ch : integerString)
			{
				const uint8 value = mDigitLookupDec.getValueByCharacter(ch);
				integerPart = integerPart * 10 + (int64)value;
				errorCheck |= value;
			}
			if (errorCheck & 0x80)
				return result;

			double doubleNumber = (double)integerPart;

			if (!fractionalString.empty())
			{
				uint64 fractionalPart = 0;
				for (char ch : fractionalString)
				{
					const uint8 value = mDigitLookupDec.getValueByCharacter(ch);
					fractionalPart = fractionalPart * 10 + (int64)value;
					errorCheck |= value;
				}
				if (errorCheck & 0x80)
					return result;

				doubleNumber += (double)fractionalPart * std::pow(0.1, fractionalString.length());
			}

			if (!exponentString.empty())
			{
				// Handle negative exponent
				bool negativeExponent = false;
				if (exponentString[0] == '-')
				{
					negativeExponent = true;
					exponentString = exponentString.substr(1);
				}

				int64 exponentPart = 0;
				for (char ch : exponentString)
				{
					const uint8 value = mDigitLookupDec.getValueByCharacter(ch);
					exponentPart = exponentPart * 10 + (int64)value;
					errorCheck |= value;
				}
				if (errorCheck & 0x80)
					return result;

				doubleNumber *= std::pow(10.0, negativeExponent ? -exponentPart : exponentPart);
			}

			if (isFloat)
			{
				result.mType = ParseNumberResult::Type::FLOAT;
				result.mValue = (float)doubleNumber;
			}
			else
			{
				result.mType = ParseNumberResult::Type::DOUBLE;
				result.mValue = doubleNumber;
			}
			return result;
		}
	}

}
