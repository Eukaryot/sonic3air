/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Utility.h"
#include "lemon/compiler/Definitions.h"


namespace lemon
{

	class ParserHelper
	{
	public:
		inline static bool isDigit(char ch)
		{
			return (ch >= '0' && ch <= '9');
		}

		inline static bool isLetter(char ch)
		{
			return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
		}

		inline static bool isOperatorCharacter(char ch)
		{
			return mOperatorLookup.isOperatorCharacter(ch);
		}

		inline static void collectNumber(const char* input, size_t length, std::string& output)
		{
			output.clear();
			for (size_t pos = 0; pos < length; ++pos)
			{
				const char ch = input[pos];
				if (isDigit(ch) || isLetter(ch))
					output += ch;
				else
					break;
			}
		}

		inline static void collectIdentifier(const char* input, size_t length, std::string& output)
		{
			output.clear();
			for (size_t pos = 0; pos < length; ++pos)
			{
				const char ch = input[pos];
				if (isDigit(ch) || isLetter(ch) || (ch == '_') || (ch == '.'))
					output += ch;
				else
					break;
			}
		}

		inline static void collectOperators(const char* input, size_t length, std::string& output)
		{
			output.clear();
			for (size_t pos = 0; pos < length; ++pos)
			{
				const char ch = input[pos];
				if (isOperatorCharacter(ch))
					output += ch;
				else
					break;
			}
		}

		inline static void collectStringLiteral(const char* input, size_t length, std::string& output, size_t& outCharactersRead, uint32 lineNumber)
		{
			output.clear();
			size_t pos;
			for (pos = 0; pos < length; ++pos)
			{
				char ch = input[pos];

				// Use backslash as escape character
				if (ch == '\\' && pos+1 < length)
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
			CHECK_ERROR(pos < length, "String literal exceeds line", lineNumber);
			outCharactersRead = pos;
		}

		inline static void collectPreprocessorCondition(const char* input, size_t length, std::string& output)
		{
			output.clear();
			for (size_t pos = 0; pos < length; ++pos)
			{
				const char ch = input[pos];
				if (ch == '"')
				{
					for (++pos; pos < length; ++pos)
					{
						if (input[pos] == '"')
							break;
					}
					break;
				}
				if (ch == '/' && pos < length-1)
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

		inline static bool findEndOfBlockComment(const std::string_view& input, size_t& pos)
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

		inline static size_t skipStringLiteral(const char* input, size_t length, uint32 lineNumber)
		{
			for (size_t pos = 0; pos < length; ++pos)
			{
				if (input[pos] == '\\' && pos+1 < length)
				{
					++pos;
				}
				else if (input[pos] == '"')
				{
					return pos + 1;
				}
			}
			CHECK_ERROR(false, "String literal exceeds line", lineNumber);
			return length;
		}

		inline static int64 parseInteger(const char* input, size_t length, uint32 lineNumber)
		{
			int64 result = 0;
			if (length >= 3 && input[0] == '0' && input[1] == 'x')
			{
				for (size_t i = 2; i < length; ++i)
				{
					const char ch = input[i];
					if (ch >= '0' && ch <= '9')
						result = result * 16 + (ch - '0');
					else if (ch >= 'A' && ch <= 'F')
						result = result * 16 + (ch - 'A') + 10;
					else if (ch >= 'a' && ch <= 'f')
						result = result * 16 + (ch - 'a') + 10;
					else
						CHECK_ERROR(false, "Invalid hexadecimal number", lineNumber);
				}
			}
			else
			{
				for (size_t i = 0; i < length; ++i)
				{
					const char ch = input[i];
					if (ch >= '0' && ch <= '9')
						result = result * 10 + (ch - '0');
					else
						CHECK_ERROR(false, "Invalid decimal number", lineNumber);
				}
			}
			return result;
		}

		inline static size_t findOperator(const char* str, size_t maxLength, Operator& outOperator)
		{
			return mOperatorLookup.lookup(str, maxLength, outOperator);
		}

	private:
		struct OperatorLookup
		{
		public:
			bool isOperatorCharacter(char ch);
			size_t lookup(const char* str, size_t maxLength, Operator& outOperator);

		private:
			struct Entry
			{
				char mCharacter = 0;
				Operator mOperator = Operator::_NUM_OPERATORS;
				std::vector<Entry*> mChildren;
			};
			ObjectPool<Entry, 48> mAllEntries;
			Entry* mTopLevelEntries[96] = { nullptr };
			bool mIsOperatorCharacter[96];
			bool mInitialized = false;

			void initialize();
		};

		static OperatorLookup mOperatorLookup;
	};

}
