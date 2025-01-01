/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/TokenTypes.h"


namespace lemon
{
	bool isKeyword(const Token& token, Keyword keyword);
	bool isOperator(const Token& token, Operator op);
	bool isIdentifier(const Token& token, uint64 identifierHash);
	bool isParenthesis(const Token& token, ParenthesisType parenthesisType);
}
