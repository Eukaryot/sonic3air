/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/Operators.h"
#include "lemon/compiler/Utility.h"

#include <optional>


namespace lemon
{

	class ParserHelper
	{
	public:
		struct ParseNumberResult
		{
			enum class Type
			{
				NONE,
				INTEGER,
				FLOAT,
				DOUBLE
			};

			Type mType = Type::NONE;
			AnyBaseValue mValue;
			size_t mBytesRead = 0;
		};

	public:
		inline static bool isLetter(char ch)
		{
			return mLookup.mIsLetter[(uint8)ch];
		}

		inline static bool isDigitOrDot(char ch)
		{
			return mLookup.mIsDigitOrDot[(uint8)ch];
		}

		inline static bool isOperatorCharacter(char ch)
		{
			return mOperatorLookup.isOperatorCharacter(ch);
		}

		inline static size_t collectIdentifier(std::string_view input)
		{
			size_t pos = 0;
			for (; pos < input.length(); ++pos)
			{
				if (!mLookup.mIsIdentifierCharacter[(uint8)input[pos]])
					return pos;
			}
			return pos;
		}

		inline static size_t collectOperators(std::string_view input)
		{
			size_t pos = 0;
			for (; pos < input.length(); ++pos)
			{
				if (!mOperatorLookup.isOperatorCharacter(input[pos]))
					return pos;
			}
			return pos;
		}

		static void collectStringLiteral(std::string_view input, std::string& output, size_t& outCharactersRead, uint32 lineNumber);
		static void collectPreprocessorStatement(std::string_view input, std::string& output);
		static bool findEndOfBlockComment(std::string_view input, size_t& pos);
		static size_t skipStringLiteral(std::string_view input, uint32 lineNumber);
		static ParseNumberResult collectNumber(std::string_view input);

		inline static size_t findOperator(std::string_view input, Operator& outOperator)
		{
			return mOperatorLookup.lookup(input, outOperator);
		}

	private:
		struct Lookup
		{
			constexpr Lookup() :
				mIsLetter(),
				mIsDigitOrLetter(),
				mIsDigitOrDot(),
				mIsIdentifierCharacter()
			{
				for (size_t i = 0; i < 0x100; ++i)
				{
					const char ch = (char)i;
					const bool isLetter = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
					const bool isDigit = (ch >= '0' && ch <= '9');
					mIsLetter[i] = isLetter;
					mIsDigitOrLetter[i] = isDigit || isLetter;
					mIsDigitOrDot[i] = isDigit || ch == '.';
					mIsIdentifierCharacter[i] = isDigit || isLetter || (ch == '_') || (ch == '.');
				}
			}

			bool mIsLetter[0x100];
			bool mIsDigitOrLetter[0x100];
			bool mIsDigitOrDot[0x100];
			bool mIsIdentifierCharacter[0x100];
		};

		struct DigitLookup
		{
			DigitLookup(bool hexadecimal)
			{
				for (int i = 0; i < 55; ++i)
				{
					const char ch = '0' + (char)i;
					if (ch >= '0' && ch <= '9')
						mValues[i] = (uint8)(ch - '0');
					else if (hexadecimal && ch >= 'A' && ch <= 'F')
						mValues[i] = (uint8)(ch - 'A') + 10;
					else if (hexadecimal && ch >= 'a' && ch <= 'f')
						mValues[i] = (uint8)(ch - 'a') + 10;
					else
						mValues[i] = 0x80;	// Uppermost bit encodes an invalid value
				}
			}

			inline uint8 getValueByCharacter(char ch) const  { return (ch >= '0' && ch <= 'f') ? mValues[ch - '0'] : 0x80; }

			uint8 mValues[55];		// Range from '0' (48) to 'f' (102)
		};

		struct OperatorLookup
		{
		public:
			bool isOperatorCharacter(char ch);
			size_t lookup(std::string_view input, Operator& outOperator);

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

	private:
		inline static const Lookup mLookup;
		inline static const DigitLookup mDigitLookupHex = DigitLookup(true);
		inline static const DigitLookup mDigitLookupDec = DigitLookup(false);
		static OperatorLookup mOperatorLookup;
	};

}
