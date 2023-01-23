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
		return (token.getType() == Token::Type::KEYWORD && token.as<KeywordToken>().mKeyword == keyword);
	}

	bool isOperator(const Token& token, Operator op)
	{
		return (token.getType() == Token::Type::OPERATOR && token.as<OperatorToken>().mOperator == op);
	}

	bool isIdentifier(const Token& token, uint64 identifierHash)
	{
		return (token.getType() == Token::Type::IDENTIFIER && token.as<IdentifierToken>().mName.getHash() == identifierHash);
	}

	bool isParenthesis(const Token& token, ParenthesisType parenthesisType)
	{
		return (token.getType() == Token::Type::PARENTHESIS && token.as<ParenthesisToken>().mParenthesisType == parenthesisType);
	}
}
