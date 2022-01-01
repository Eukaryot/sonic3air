/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/GenericManager.h"
#include "lemon/program/DataType.h"


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

		inline Type getType() const { return mType; }

		template<typename T> const T& as() const { return *static_cast<const T*>(this); }
		template<typename T> T& as() { return *static_cast<T*>(this); }

	protected:
		inline ParserToken(Type type) : genericmanager::Element<ParserToken>((uint32)type), mType(type) {}

	private:
		const Type mType;
	};


	class KeywordParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::KEYWORD;

	public:
		inline KeywordParserToken() : ParserToken(TYPE) {}

	public:
		Keyword mKeyword = Keyword::_INVALID;
	};


	class VarTypeParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::VARTYPE;

	public:
		inline VarTypeParserToken() : ParserToken(TYPE) {}

	public:
		const DataTypeDefinition* mDataType = nullptr;
	};


	class OperatorParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::OPERATOR;

	public:
		inline OperatorParserToken() : ParserToken(TYPE) {}

	public:
		Operator mOperator = Operator::_INVALID;
	};


	class PragmaParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::PRAGMA;

	public:
		inline PragmaParserToken() : ParserToken(TYPE) {}

	public:
		std::string mContent;
	};


	class LabelParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::LABEL;

	public:
		inline LabelParserToken() : ParserToken(TYPE) {}

	public:
		std::string mName;
	};


	class ConstantParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::CONSTANT;

	public:
		inline ConstantParserToken() : ParserToken(TYPE) {}

	public:
		int64 mValue = 0;
	};


	class StringLiteralParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::STRING_LITERAL;

	public:
		inline StringLiteralParserToken() : ParserToken(TYPE) {}

	public:
		std::string mString;
	};


	class IdentifierParserToken : public ParserToken
	{
	public:
		static const Type TYPE = Type::IDENTIFIER;

	public:
		inline IdentifierParserToken() : ParserToken(TYPE) {}

	public:
		std::string mIdentifier;
	};


	class ParserTokenList : public genericmanager::ElementList<ParserToken, 32>
	{
	};

}
