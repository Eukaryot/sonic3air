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
#include "lemon/compiler/Token.h"


namespace lemon
{

	class Function;

	enum class ParenthesisType
	{
		PARENTHESIS,
		BRACKET
	};


	class KeywordToken : public Token
	{
	public:
		static const Type TYPE = Type::KEYWORD;

	public:
		inline KeywordToken() : Token(TYPE) {}

	public:
		Keyword mKeyword = Keyword::_INVALID;
	};


	class VarTypeToken : public Token
	{
	public:
		static const Type TYPE = Type::VARTYPE;

	public:
		inline VarTypeToken() : Token(TYPE) {}

	public:
		const DataTypeDefinition* mDataType = nullptr;
	};


	class OperatorToken : public Token
	{
	public:
		static const Type TYPE = Type::OPERATOR;

	public:
		inline OperatorToken() : Token(TYPE) {}

	public:
		Operator mOperator = Operator::_INVALID;
	};


	class LabelToken : public Token
	{
	public:
		static const Type TYPE = Type::LABEL;

	public:
		inline LabelToken() : Token(TYPE) {}

	public:
		std::string mName;
	};



	// --- Statement tokens ---

	class ConstantToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::CONSTANT;

	public:
		inline ConstantToken() : StatementToken(TYPE) {}

	public:
		int64 mValue = 0;
	};


	class IdentifierToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::IDENTIFIER;

	public:
		inline IdentifierToken() : StatementToken(TYPE) {}

	public:
		std::string mIdentifier;
	};


	class ParenthesisToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::PARENTHESIS;

	public:
		inline ParenthesisToken() : StatementToken(TYPE) {}

	public:
		ParenthesisType mParenthesisType = ParenthesisType::PARENTHESIS;
		TokenList mContent;
	};


	class CommaSeparatedListToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::COMMA_SEPARATED;

	public:
		inline CommaSeparatedListToken() : StatementToken(TYPE) {}

	public:
		std::vector<TokenList> mContent;
	};


	class UnaryOperationToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::UNARY_OPERATION;

	public:
		inline UnaryOperationToken() : StatementToken(TYPE) {}

	public:
		Operator mOperator = Operator::_INVALID;
		TokenPtr<StatementToken> mArgument;
	};


	class BinaryOperationToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::BINARY_OPERATION;

	public:
		inline BinaryOperationToken() : StatementToken(TYPE) {}

	public:
		Operator mOperator = Operator::_INVALID;
		TokenPtr<StatementToken> mLeft;
		TokenPtr<StatementToken> mRight;
		const Function* mFunction = nullptr;	// Usually a null pointer, except if a certain function is enforced
	};


	class VariableToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::VARIABLE;

	public:
		inline VariableToken() : StatementToken(TYPE) {}

	public:
		const Variable* mVariable = nullptr;
	};


	class FunctionToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::FUNCTION;

	public:
		inline FunctionToken() : StatementToken(TYPE) {}

	public:
		std::string mFunctionName;
		const Function* mFunction = nullptr;
		bool mIsBaseCall = false;
		std::vector<TokenPtr<StatementToken>> mParameters;
	};


	class MemoryAccessToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::MEMORY_ACCESS;

	public:
		inline MemoryAccessToken() : StatementToken(TYPE) {}

	public:
		TokenPtr<StatementToken> mAddress;
	};


	class ValueCastToken : public StatementToken
	{
	public:
		static const Type TYPE = Type::VALUE_CAST;

	public:
		inline ValueCastToken() : StatementToken(TYPE) {}

	public:
		TokenPtr<StatementToken> mArgument;
	};

}
