/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace lemon
{

	enum class Operator : uint8
	{
		ASSIGN,
		ASSIGN_PLUS,
		ASSIGN_MINUS,
		ASSIGN_MULTIPLY,
		ASSIGN_DIVIDE,
		ASSIGN_MODULO,
		ASSIGN_SHIFT_LEFT,
		ASSIGN_SHIFT_RIGHT,
		ASSIGN_AND,
		ASSIGN_OR,
		ASSIGN_XOR,
		BINARY_PLUS,
		BINARY_MINUS,
		BINARY_MULTIPLY,
		BINARY_DIVIDE,
		BINARY_MODULO,
		BINARY_SHIFT_LEFT,
		BINARY_SHIFT_RIGHT,
		BINARY_AND,
		BINARY_OR,
		BINARY_XOR,
		LOGICAL_AND,
		LOGICAL_OR,
		UNARY_NOT,
		UNARY_BITNOT,
		UNARY_DECREMENT,
		UNARY_INCREMENT,
		COMPARE_EQUAL,
		COMPARE_NOT_EQUAL,
		COMPARE_LESS,
		COMPARE_LESS_OR_EQUAL,
		COMPARE_GREATER,
		COMPARE_GREATER_OR_EQUAL,
		QUESTIONMARK,		// First part of trinary operator ?:
		COLON,				// Second part of trinary operator ?:
		SEMICOLON_SEPARATOR,
		COMMA_SEPARATOR,
		PARENTHESIS_LEFT,
		PARENTHESIS_RIGHT,
		BRACKET_LEFT,
		BRACKET_RIGHT,
		_NUM_OPERATORS,
		_INVALID = _NUM_OPERATORS
	};

	enum class Keyword : uint8
	{
		_INVALID = 0,
		BLOCK_BEGIN,
		BLOCK_END,
		FUNCTION,
		GLOBAL,
		DEFINE,
		RETURN,
		CALL,
		JUMP,
		BREAK,
		CONTINUE,
		IF,
		ELSE,
		WHILE,
		FOR
	};

}
