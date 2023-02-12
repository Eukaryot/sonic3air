/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/GenericManager.h"
#include "lemon/program/DataType.h"


namespace lemon
{
	class GlobalsLookup;
	class Variable;


	class API_EXPORT Token : public genericmanager::Element<Token>
	{
	public:
		enum class Type : uint8		// Keep values under 0x80 for optimizations in "genericmanager::ElementFactoryMap" to work
		{
			KEYWORD,
			VARTYPE,
			OPERATOR,
			LABEL,

			// Statements
			STATEMENT = 0x40,
			CONSTANT,
			IDENTIFIER,
			PARENTHESIS,
			COMMA_SEPARATED,
			UNARY_OPERATION,
			BINARY_OPERATION,
			VARIABLE,
			FUNCTION,
			MEMORY_ACCESS,
			VALUE_CAST
		};

	public:
		virtual ~Token() {}

		inline Type getType() const  { return (Type)genericmanager::Element<Token>::getType(); }
		inline bool isStatement() const  { return (genericmanager::Element<Token>::getType() & (uint32)Type::STATEMENT) != 0; }

		template<typename T> const T& as() const  { return *static_cast<const T*>(this); }
		template<typename T> T& as()  { return *static_cast<T*>(this); }

	protected:
		inline Token(Type type) : genericmanager::Element<Token>((uint32)type) {}
	};


	class StatementToken : public Token
	{
	public:
		const DataTypeDefinition* mDataType = nullptr;

	protected:
		inline StatementToken(Type type) : Token(type) {}
	};



	template<typename T>
	class TokenPtr : public genericmanager::ElementPtr<T, Token>
	{
	public:
		using genericmanager::ElementPtr<T, Token>::operator=;
	};


	class TokenList : public genericmanager::ElementList<Token, 16>
	{
	};


	class TokenSerializer
	{
	public:
		static void serializeToken(VectorBinarySerializer& serializer, TokenPtr<Token>& token, const GlobalsLookup& globalsLookup);
		static void serializeToken(VectorBinarySerializer& serializer, TokenPtr<StatementToken>& token, const GlobalsLookup& globalsLookup);
		static void serializeTokenList(VectorBinarySerializer& serializer, TokenList& tokenList, const GlobalsLookup& globalsLookup);

	private:
		static void serializeTokenData(VectorBinarySerializer& serializer, Token& token, const GlobalsLookup& globalsLookup);
	};

}
