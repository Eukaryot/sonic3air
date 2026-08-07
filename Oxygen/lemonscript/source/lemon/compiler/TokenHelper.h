/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
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
	bool isAssignment(const Token& token);

	template<class PRED>
	const Token* findInTokenTree(const Token& token, PRED predicate)
	{
		if (predicate(token))
			return &token;

		switch (token.getType())
		{
			case UnaryOperationToken::TYPE:
			{
				const UnaryOperationToken& uot = token.as<UnaryOperationToken>();
				return findInTokenTree(*uot.mArgument, predicate);
			}

			case BinaryOperationToken::TYPE:
			{
				const BinaryOperationToken& bot = token.as<BinaryOperationToken>();
				const Token* foundToken = findInTokenTree(*bot.mLeft, predicate);
				if (nullptr != foundToken)
				{
					return foundToken;
				}
				else
				{
					return findInTokenTree(*bot.mRight, predicate);
				}
			}

			case BracketAccessToken::TYPE:
			{
				const BracketAccessToken& bat = token.as<BracketAccessToken>();
				return findInTokenTree(*bat.mParameter, predicate);
			}

			case MemoryAccessToken::TYPE:
			{
				const MemoryAccessToken& mat = token.as<MemoryAccessToken>();
				return findInTokenTree(*mat.mAddress, predicate);
			}

			case ValueCastToken::TYPE:
			{
				const ValueCastToken& vct = token.as<ValueCastToken>();
				return findInTokenTree(*vct.mArgument, predicate);
			}
		}
		return nullptr;
	}
}
