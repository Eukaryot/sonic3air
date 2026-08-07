/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/basics/GenericManager.h"
#include "lemon/compiler/Operators.h"
#include "lemon/program/DataType.h"
#include "lemon/utility/AnyBaseValue.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{

	class ParserToken : public genericmanager::Element<ParserToken>
	{
	public:
		enum class Type : uint8
		{
			KEYWORD,
			VARTYPE,
			OPERATOR,
			LABEL,
			PRAGMA,
			CONSTANT,
			STRING_LITERAL,
			IDENTIFIER
		};

	public:
		virtual ~ParserToken() {}

		inline Type getType() const  { return (Type)genericmanager::Element<ParserToken>::getType(); }

	protected:
		inline ParserToken(uint32 type) : genericmanager::Element<ParserToken>((uint32)type) {}
	};



	#define DEFINE_LEMON_PARSER_TOKEN_TYPE(_class_, _type_) \
		DEFINE_GENERIC_MANAGER_ELEMENT_TYPE(ParserToken, ParserToken, _class_, (uint32)_type_)


	class KeywordParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(KeywordParserToken, Type::KEYWORD)

	public:
		Keyword mKeyword = Keyword::_INVALID;
	};


	class VarTypeParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(VarTypeParserToken, Type::VARTYPE)

	public:
		const DataTypeDefinition* mDataType = nullptr;
	};


	class OperatorParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(OperatorParserToken, Type::OPERATOR)

	public:
		Operator mOperator = Operator::_INVALID;
	};


	class PragmaParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(PragmaParserToken, Type::PRAGMA)

	public:
		std::string mContent;
	};


	class LabelParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(LabelParserToken, Type::LABEL)

	public:
		FlyweightString mName;
	};


	class ConstantParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(ConstantParserToken, Type::CONSTANT)

	public:
		AnyBaseValue mValue { 0 };
		BaseType mBaseType = BaseType::INT_CONST;	// Can also be FLOAT or DOUBLE
	};


	class StringLiteralParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(StringLiteralParserToken, Type::STRING_LITERAL)

	public:
		FlyweightString mString;
	};


	class IdentifierParserToken : public ParserToken
	{
	public:
		DEFINE_LEMON_PARSER_TOKEN_TYPE(IdentifierParserToken, Type::IDENTIFIER)

	public:
		FlyweightString mName;
	};


	class ParserTokenList : public genericmanager::ElementList<ParserToken, 32>
	{
	};


	#undef DEFINE_LEMON_PARSER_TOKEN_TYPE

}
