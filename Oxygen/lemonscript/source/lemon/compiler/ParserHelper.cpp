/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/ParserHelper.h"


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

	size_t ParserHelper::OperatorLookup::lookup(const char* str, size_t maxLength, Operator& outOperator)
	{
		if (!mInitialized)
		{
			initialize();
		}

		size_t outLength = 0;
		const Entry* parentEntry = nullptr;
		for (size_t i = 0; i < maxLength; ++i)
		{
			const char character = str[i];
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

		const std::map<std::string, Operator> operatorStrings =
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
			const std::string& string = pair.first;
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

}
