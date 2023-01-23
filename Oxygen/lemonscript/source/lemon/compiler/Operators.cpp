/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Operators.h"


namespace lemon
{

	namespace
	{
		static const constexpr uint8 operatorPriorityLookup[] =
		{
			15,	 // ASSIGN
			15,	 // ASSIGN_PLUS
			15,	 // ASSIGN_MINUS
			15,	 // ASSIGN_MULTIPLY
			15,	 // ASSIGN_DIVIDE
			15,	 // ASSIGN_MODULO
			15,	 // ASSIGN_SHIFT_LEFT
			15,	 // ASSIGN_SHIFT_RIGHT
			15,	 // ASSIGN_AND
			15,	 // ASSIGN_OR
			15,	 // ASSIGN_XOR
			6,	 // BINARY_PLUS
			6,	 // BINARY_MINUS
			5,	 // BINARY_MULTIPLY
			5,	 // BINARY_DIVIDE
			5,	 // BINARY_MODULO
			7,	 // BINARY_SHIFT_LEFT
			7,	 // BINARY_SHIFT_RIGHT
			10,	 // BINARY_AND
			12,	 // BINARY_OR
			11,	 // BINARY_XOR
			13,	 // LOGICAL_AND
			14,	 // LOGICAL_OR
			3,   // UNARY_NOT
			3,   // UNARY_BITNOT
			3,	 // UNARY_DECREMENT (actually 2 for post-, 3 for pre-decrement)
			3,	 // UNARY_INCREMENT (same here)
			9,	 // COMPARE_EQUAL
			9,	 // COMPARE_NOT_EQUAL
			8,	 // COMPARE_LESS
			8,	 // COMPARE_LESS_OR_EQUAL
			8,	 // COMPARE_GREATER
			8,	 // COMPARE_GREATER_OR_EQUAL
			15,	 // QUESTIONMARK
			15,	 // COLON
			18,	 // SEMICOLON_SEPARATOR (only in 'for' statements, otherwise ignored)
			17,	 // COMMA_SEPARATOR (should be evaluated separatedly, after all others)
			2,	 // PARENTHESIS_LEFT
			2,	 // PARENTHESIS_RIGHT
			2,	 // BRACKET_LEFT
			2	 // BRACKET_RIGHT
		};
		static_assert(sizeof(operatorPriorityLookup) == (size_t)Operator::_NUM_OPERATORS, "Update operator priority lookup");

		static const constexpr bool operatorAssociativityLookup[] =
		{
			// "false" = left to right
			// "true" = right to left
			false,		// Priority 0 (unused)
			false,		// Priority 1 (reserved for :: operator)
			false,		// Priority 2 (parentheses)
			true,		// Priority 3 (unary operators)
			false,		// Priority 4 (reserved for element access)
			false,		// Priority 5 (multiplication, division)
			false,		// Priority 6 (addition, subtraction)
			false,		// Priority 7 (shifts)
			false,		// Priority 8 (comparisons)
			false,		// Priority 9 (comparisons)
			false,		// Priority 10 (bitwise AND)
			false,		// Priority 11 (bitwise XOR)
			false,		// Priority 12 (bitwise OR)
			false,		// Priority 13 (logical AND)
			false,		// Priority 14 (logical OR)
			true,		// Priority 15 (assignments and trinary operator)
			true,		// Priority 16 (reserved for throw)
			false		// Priority 17 (comma separator)
		};

		const char* operatorCharacters[] =
		{
			"=",	// ASSIGN
			"+=",	// ASSIGN_PLUS
			"-=",	// ASSIGN_MINUS
			"*=",	// ASSIGN_MULTIPLY
			"/=",	// ASSIGN_DIVIDE
			"%=",	// ASSIGN_MODULO
			"<<=",	// ASSIGN_SHIFT_LEFT
			">>=",	// ASSIGN_SHIFT_RIGHT
			"&=",	// ASSIGN_AND
			"|=",	// ASSIGN_OR
			"^=",	// ASSIGN_XOR
			"+",	// BINARY_PLUS
			"-",	// BINARY_MINUS
			"*",	// BINARY_MULTIPLY
			"/",	// BINARY_DIVIDE
			"%",	// BINARY_MODULO
			"<<",	// BINARY_SHIFT_LEFT
			">>",	// BINARY_SHIFT_RIGHT
			"&",	// BINARY_AND
			"|",	// BINARY_OR
			"^",	// BINARY_XOR
			"&&",	// LOGICAL_AND
			"||",	// LOGICAL_OR
			"",		// UNARY_NOT
			"",		// UNARY_BITNOT
			"-",	// UNARY_DECREMENT
			"+",	// UNARY_INCREMENT
			"==",	// COMPARE_EQUAL
			"!=",	// COMPARE_NOT_EQUAL
			"<",	// COMPARE_LESS
			"<=",	// COMPARE_LESS_OR_EQUAL
			">",	// COMPARE_GREATER
			">=",	// COMPARE_GREATER_OR_EQUAL
			"?",	// QUESTIONMARK
			":",	// COLON
			";",	// SEMICOLON_SEPARATOR
			",",	// COMMA_SEPARATOR
			"(",	// PARENTHESIS_LEFT
			")",	// PARENTHESIS_RIGHT
			"[",	// BRACKET_LEFT
			"]",	// BRACKET_RIGHT
		};
	}


	const char* OperatorHelper::getOperatorCharacters(Operator op)
	{
		return operatorCharacters[(size_t)op];
	}

	uint8 OperatorHelper::getOperatorPriority(Operator op)
	{
		return operatorPriorityLookup[(size_t)op];
	}

	bool OperatorHelper::isOperatorAssociative(Operator op)
	{
		const uint8 priority = operatorPriorityLookup[(size_t)op];
		return operatorAssociativityLookup[priority];
	}

	OperatorHelper::OperatorType OperatorHelper::getOperatorType(Operator op)
	{
		switch (op)
		{
			case Operator::ASSIGN:
			case Operator::ASSIGN_PLUS:
			case Operator::ASSIGN_MINUS:
			case Operator::ASSIGN_MULTIPLY:
			case Operator::ASSIGN_DIVIDE:
			case Operator::ASSIGN_MODULO:
			case Operator::ASSIGN_SHIFT_LEFT:	// TODO: Special handling required
			case Operator::ASSIGN_SHIFT_RIGHT:	// TODO: Special handling required
			case Operator::ASSIGN_AND:
			case Operator::ASSIGN_OR:
			case Operator::ASSIGN_XOR:
			{
				return OperatorType::ASSIGNMENT;
			}

			case Operator::BINARY_PLUS:
			case Operator::BINARY_MINUS:
			case Operator::BINARY_MULTIPLY:
			case Operator::BINARY_DIVIDE:
			case Operator::BINARY_MODULO:
			case Operator::BINARY_SHIFT_LEFT:	// TODO: Special handling required
			case Operator::BINARY_SHIFT_RIGHT:	// TODO: Special handling required
			case Operator::BINARY_AND:
			case Operator::BINARY_OR:
			case Operator::BINARY_XOR:
			case Operator::LOGICAL_AND:
			case Operator::LOGICAL_OR:
			case Operator::COLON:
			{
				return OperatorType::SYMMETRIC;
			}

			case Operator::COMPARE_EQUAL:
			case Operator::COMPARE_NOT_EQUAL:
			case Operator::COMPARE_LESS:
			case Operator::COMPARE_LESS_OR_EQUAL:
			case Operator::COMPARE_GREATER:
			case Operator::COMPARE_GREATER_OR_EQUAL:
			{
				return OperatorType::COMPARISON;
			}

			case Operator::QUESTIONMARK:
			{
				return OperatorType::TRINARY;
			}

			default:
			{
				return OperatorType::UNKNOWN;
			}
		}
	}

}
