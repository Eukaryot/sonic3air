/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/TokenHelper.h"


namespace lemon
{
	bool isKeyword(const Token& token, Keyword keyword)
	{
		return (token.isA<KeywordToken>() && token.as<KeywordToken>().mKeyword == keyword);
	}

	bool isOperator(const Token& token, Operator op)
	{
		return (token.isA<OperatorToken>() && token.as<OperatorToken>().mOperator == op);
	}

	bool isIdentifier(const Token& token, uint64 identifierHash)
	{
		return (token.isA<IdentifierToken>() && token.as<IdentifierToken>().mName.getHash() == identifierHash);
	}

	bool isParenthesis(const Token& token, ParenthesisType parenthesisType)
	{
		return (token.isA<ParenthesisToken>() && token.as<ParenthesisToken>().mParenthesisType == parenthesisType);
	}
}
