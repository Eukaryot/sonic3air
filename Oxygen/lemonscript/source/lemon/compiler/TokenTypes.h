/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/Operators.h"
#include "lemon/compiler/Token.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/utility/AnyBaseValue.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{

	class Function;

	enum class ParenthesisType
	{
		PARENTHESIS,
		BRACKET
	};


	#define DEFINE_LEMON_TOKEN_TYPE(_class_) \
		DEFINE_GENERIC_MANAGER_ELEMENT_TYPE(Token, Token, _class_, assignType(#_class_, false))

	#define DEFINE_LEMON_STATEMENT_TOKEN_TYPE(_class_) \
		DEFINE_GENERIC_MANAGER_ELEMENT_TYPE(Token, StatementToken, _class_, assignType(#_class_, true))


	class KeywordToken : public Token
	{
	public:
		DEFINE_LEMON_TOKEN_TYPE(KeywordToken)

	public:
		Keyword mKeyword = Keyword::_INVALID;
	};


	class VarTypeToken : public Token
	{
	public:
		DEFINE_LEMON_TOKEN_TYPE(VarTypeToken)

	public:
		const DataTypeDefinition* mDataType = nullptr;
	};


	class OperatorToken : public Token
	{
	public:
		DEFINE_LEMON_TOKEN_TYPE(OperatorToken)

	public:
		Operator mOperator = Operator::_INVALID;
	};


	class LabelToken : public Token
	{
	public:
		DEFINE_LEMON_TOKEN_TYPE(LabelToken)

	public:
		FlyweightString mName;
	};



	// --- Statement tokens ---

	class ConstantToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(ConstantToken)

	public:
		AnyBaseValue mValue { 0 };
	};


	class IdentifierToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(IdentifierToken)

	public:
		FlyweightString mName;
		const GlobalsLookup::Identifier* mResolved = nullptr;
	};


	class ParenthesisToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(ParenthesisToken)

	public:
		ParenthesisType mParenthesisType = ParenthesisType::PARENTHESIS;
		TokenList mContent;
	};


	class CommaSeparatedListToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(CommaSeparatedListToken)

	public:
		std::vector<TokenList> mContent;
	};


	class UnaryOperationToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(UnaryOperationToken)

	public:
		Operator mOperator = Operator::_INVALID;
		TokenPtr<StatementToken> mArgument;
	};


	class BinaryOperationToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(BinaryOperationToken)

	public:
		Operator mOperator = Operator::_INVALID;
		TokenPtr<StatementToken> mLeft;
		TokenPtr<StatementToken> mRight;
		const Function* mFunction = nullptr;	// Usually a null pointer, except if a certain function is enforced
	};


	class VariableToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(VariableToken)

	public:
		const Variable* mVariable = nullptr;
	};


	class FunctionToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(FunctionToken)

	public:
		const Function* mFunction = nullptr;
		bool mIsBaseCall = false;
		std::vector<TokenPtr<StatementToken>> mParameters;
	};


	class BracketAccessToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(BracketAccessToken)

	public:
		const Variable* mVariable = nullptr;
		TokenPtr<StatementToken> mParameter;
	};


	class MemoryAccessToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(MemoryAccessToken)

	public:
		TokenPtr<StatementToken> mAddress;
	};


	class ValueCastToken : public StatementToken
	{
	public:
		DEFINE_LEMON_STATEMENT_TOKEN_TYPE(ValueCastToken)

	public:
		TokenPtr<StatementToken> mArgument;
	};


	#undef DEFINE_LEMON_TOKEN_TYPE
	#undef DEFINE_LEMON_STATEMENT_TOKEN_TYPE

}
