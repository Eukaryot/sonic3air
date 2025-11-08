/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/basics/GenericManager.h"
#include "lemon/program/DataType.h"


namespace lemon
{
	class GlobalsLookup;
	class Variable;


	class API_EXPORT Token : public genericmanager::Element<Token>
	{
	public:
		virtual ~Token() {}

		inline bool isStatement() const  { return (getType() & 0x10000000) != 0; }

	protected:
		inline Token(Type type) : genericmanager::Element<Token>(type) {}

	protected:
		static constexpr Type assignType(const char* data, bool isStatement)  { return (rmx::constMurmur2_64(data) & 0x0fffffff) + isStatement * 0x10000000; }
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
