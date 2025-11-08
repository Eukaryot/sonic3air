/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Token.h"
#include "lemon/compiler/TokenTypes.h"


namespace lemon
{

	struct TokenSerializationIDTranslator
	{
		static const uint8 LAST_ENTRY = 0x4b;

		// Initialization was meant to be done in a constructor, but that can lead to the constructor of our static instance getting called too early, namely before all token calsses could register
		void initialize()
		{
			#define ADD_ENTRIES(_serializationID_, _type_) \
				static_assert(_serializationID_ <= LAST_ENTRY); \
				mFactoryBySerializationID[_serializationID_] = &_type_::CLASS.getFactory(); \
				mSerializationIDByTokenType[_type_::TYPE] = _serializationID_;

			// Serialization IDs used here are those originally used as enum values for token types (though that enum does not exist any more)
			ADD_ENTRIES(0x00, KeywordToken);
			ADD_ENTRIES(0x01, VarTypeToken);
			ADD_ENTRIES(0x02, OperatorToken);
			ADD_ENTRIES(0x03, LabelToken);
			ADD_ENTRIES(0x41, ConstantToken);
			ADD_ENTRIES(0x42, IdentifierToken);
			ADD_ENTRIES(0x43, ParenthesisToken);
			ADD_ENTRIES(0x44, CommaSeparatedListToken);
			ADD_ENTRIES(0x45, UnaryOperationToken);
			ADD_ENTRIES(0x46, BinaryOperationToken);
			ADD_ENTRIES(0x47, VariableToken);
			ADD_ENTRIES(0x48, FunctionToken);
			ADD_ENTRIES(0x49, BracketAccessToken);
			ADD_ENTRIES(0x4a, MemoryAccessToken);
			ADD_ENTRIES(0x4b, ValueCastToken);

			#undef ADD_ENTRIES

			mInitialized = true;
		}

		Token* createToken(uint8 serializationID)
		{
			if (!mInitialized)
				initialize();

			RMX_CHECK(serializationID <= LAST_ENTRY, "Unknown or unsupported token type to create (" << serializationID << ")", RMX_REACT_THROW);
			genericmanager::detail::ElementFactoryBase<Token>* factory = mFactoryBySerializationID[serializationID];
			RMX_CHECK(nullptr != factory, "Unknown or unsupported token type to create (" << serializationID << ")", RMX_REACT_THROW);
			return &factory->create();
		}

		uint8 getSerializationID(Token& token)
		{
			if (!mInitialized)
				initialize();

			const uint8* serializationID = mapFind(mSerializationIDByTokenType, token.getType());
			RMX_CHECK(nullptr != serializationID, "Unknown or unsupported token type to save", RMX_REACT_THROW);
			return *serializationID;
		}

	private:
		bool mInitialized = false;
		genericmanager::detail::ElementFactoryBase<Token>* mFactoryBySerializationID[LAST_ENTRY + 1] = { nullptr };
		std::unordered_map<uint32, uint8> mSerializationIDByTokenType;
	};

	static TokenSerializationIDTranslator mTranslator;


	void TokenSerializer::serializeToken(VectorBinarySerializer& serializer, TokenPtr<Token>& token, const GlobalsLookup& globalsLookup)
	{
		if (serializer.isReading())
		{
			token = mTranslator.createToken(serializer.read<uint8>());
		}
		else
		{
			serializer.write<uint8>(mTranslator.getSerializationID(*token));
		}

		serializeTokenData(serializer, *token, globalsLookup);
	}

	void TokenSerializer::serializeToken(VectorBinarySerializer& serializer, TokenPtr<StatementToken>& token, const GlobalsLookup& globalsLookup)
	{
		serializeToken(serializer, reinterpret_cast<TokenPtr<Token>&>(token), globalsLookup);
	}

	void TokenSerializer::serializeTokenList(VectorBinarySerializer& serializer, TokenList& tokenList, const GlobalsLookup& globalsLookup)
	{
		if (serializer.isReading())
		{
			const size_t numberOfTokens = (size_t)serializer.read<uint8>();
			for (size_t k = 0; k < numberOfTokens; ++k)
			{
				TokenPtr<Token> tokenPtr;
				serializeToken(serializer, tokenPtr, globalsLookup);
				tokenList.add(*tokenPtr);
			}
		}
		else
		{
			serializer.writeAs<uint8>(tokenList.size());
			for (size_t k = 0; k < tokenList.size(); ++k)
			{
				const uint8 serializationID = mTranslator.getSerializationID(tokenList[k]);
				serializer.write<uint8>(serializationID);
				serializeTokenData(serializer, tokenList[k], globalsLookup);
			}
		}
	}

	void TokenSerializer::serializeTokenData(VectorBinarySerializer& serializer, Token& token_, const GlobalsLookup& globalsLookup)
	{
		switch (token_.getType())
		{
			case KeywordToken::TYPE:
			{
				KeywordToken& token = token_.as<KeywordToken>();
				serializer.serializeAs<uint8>(token.mKeyword);
				break;
			}

			case VarTypeToken::TYPE:
			{
				VarTypeToken& token = token_.as<VarTypeToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				break;
			}

			case OperatorToken::TYPE:
			{
				OperatorToken& token = token_.as<OperatorToken>();
				serializer.serializeAs<uint8>(token.mOperator);
				break;
			}

			case LabelToken::TYPE:
			{
				LabelToken& token = token_.as<LabelToken>();
				token.mName.serialize(serializer);
				break;
			}

			case ConstantToken::TYPE:
			{
				ConstantToken& token = token_.as<ConstantToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				if (serializer.isReading())
					token.mValue.set(serializer.read<uint64>());
				else
					serializer.write(token.mValue.get<uint64>());
				break;
			}

			case IdentifierToken::TYPE:
			{
				IdentifierToken& token = token_.as<IdentifierToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				token.mName.serialize(serializer);
				break;
			}

			case ParenthesisToken::TYPE:
			{
				ParenthesisToken& token = token_.as<ParenthesisToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializer.serializeAs<uint8>(token.mParenthesisType);
				serializeTokenList(serializer, token.mContent, globalsLookup);
				break;
			}

			case CommaSeparatedListToken::TYPE:
			{
				CommaSeparatedListToken& token = token_.as<CommaSeparatedListToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);

				size_t numberOfEntries = token.mContent.size();
				serializer.serializeAs<uint8>(numberOfEntries);
				token.mContent.resize(numberOfEntries);
				for (size_t k = 0; k < numberOfEntries; ++k)
				{
					serializeTokenList(serializer, token.mContent[k], globalsLookup);
				}
				break;
			}

			case UnaryOperationToken::TYPE:
			{
				UnaryOperationToken& token = token_.as<UnaryOperationToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializer.serializeAs<uint8>(token.mOperator);
				serializeToken(serializer, token.mArgument, globalsLookup);
				break;
			}

			case BinaryOperationToken::TYPE:
			{
				BinaryOperationToken& token = token_.as<BinaryOperationToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializer.serializeAs<uint8>(token.mOperator);
				serializeToken(serializer, token.mLeft, globalsLookup);
				serializeToken(serializer, token.mRight, globalsLookup);
				break;
			}

			case VariableToken::TYPE:
			{
				RMX_ERROR("Not supported", );
			/*
				VariableToken& token = token_.as<VariableToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				// TODO: Serialize the variable pointer
			*/
				break;
			}

			case FunctionToken::TYPE:
			{
				RMX_ERROR("Not supported", );
			/*
				FunctionToken& token = token_.as<FunctionToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				// TODO: Serialize the other members
			*/
				break;
			}

			case BracketAccessToken::TYPE:
			{
				BracketAccessToken& token = token_.as<BracketAccessToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializeToken(serializer, token.mParameter, globalsLookup);
				break;
			}

			case MemoryAccessToken::TYPE:
			{
				MemoryAccessToken& token = token_.as<MemoryAccessToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializeToken(serializer, token.mAddress, globalsLookup);
				break;
			}

			case ValueCastToken::TYPE:
			{
				ValueCastToken& token = token_.as<ValueCastToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializeToken(serializer, token.mArgument, globalsLookup);
				break;
			}

			default:
				RMX_ERROR("Unknown token type to write", );
		}
	}

}
