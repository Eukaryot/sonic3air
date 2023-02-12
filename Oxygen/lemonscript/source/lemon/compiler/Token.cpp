/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Token.h"
#include "lemon/compiler/TokenTypes.h"


namespace lemon
{

	void TokenSerializer::serializeToken(VectorBinarySerializer& serializer, TokenPtr<Token>& token, const GlobalsLookup& globalsLookup)
	{
		if (serializer.isReading())
		{
			const Token::Type tokenType = (Token::Type)serializer.read<uint8>();
			switch (tokenType)
			{
				case Token::Type::KEYWORD:			token = &genericmanager::Manager<Token>::template create<KeywordToken>();			 break;
				case Token::Type::VARTYPE:			token = &genericmanager::Manager<Token>::template create<VarTypeToken>();			 break;
				case Token::Type::OPERATOR:			token = &genericmanager::Manager<Token>::template create<OperatorToken>();			 break;
				case Token::Type::LABEL:			token = &genericmanager::Manager<Token>::template create<LabelToken>();				 break;
				case Token::Type::CONSTANT:			token = &genericmanager::Manager<Token>::template create<ConstantToken>();			 break;
				case Token::Type::IDENTIFIER:		token = &genericmanager::Manager<Token>::template create<IdentifierToken>();		 break;
				case Token::Type::PARENTHESIS:		token = &genericmanager::Manager<Token>::template create<ParenthesisToken>();		 break;
				case Token::Type::COMMA_SEPARATED:	token = &genericmanager::Manager<Token>::template create<CommaSeparatedListToken>(); break;
				case Token::Type::UNARY_OPERATION:	token = &genericmanager::Manager<Token>::template create<UnaryOperationToken>();	 break;
				case Token::Type::BINARY_OPERATION:	token = &genericmanager::Manager<Token>::template create<BinaryOperationToken>();	 break;
				case Token::Type::VARIABLE:			token = &genericmanager::Manager<Token>::template create<VariableToken>();			 break;
				case Token::Type::FUNCTION:			token = &genericmanager::Manager<Token>::template create<FunctionToken>();			 break;
				case Token::Type::MEMORY_ACCESS:	token = &genericmanager::Manager<Token>::template create<MemoryAccessToken>();		 break;
				case Token::Type::VALUE_CAST:		token = &genericmanager::Manager<Token>::template create<ValueCastToken>();			 break;

				default:
					RMX_ERROR("Unknown or unsupported token type to create", );
			}
		}
		else
		{
			serializer.writeAs<uint8>(token->getType());
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
				serializer.writeAs<uint8>(tokenList[k].getType());
				serializeTokenData(serializer, tokenList[k], globalsLookup);
			}
		}
	}

	void TokenSerializer::serializeTokenData(VectorBinarySerializer& serializer, Token& token_, const GlobalsLookup& globalsLookup)
	{
		switch (token_.getType())
		{
			case Token::Type::KEYWORD:
			{
				KeywordToken& token = token_.as<KeywordToken>();
				serializer.serializeAs<uint8>(token.mKeyword);
				break;
			}

			case Token::Type::VARTYPE:
			{
				VarTypeToken& token = token_.as<VarTypeToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				break;
			}

			case Token::Type::OPERATOR:
			{
				OperatorToken& token = token_.as<OperatorToken>();
				serializer.serializeAs<uint8>(token.mOperator);
				break;
			}

			case Token::Type::LABEL:
			{
				LabelToken& token = token_.as<LabelToken>();
				token.mName.serialize(serializer);
				break;
			}

			case Token::Type::CONSTANT:
			{
				ConstantToken& token = token_.as<ConstantToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				if (serializer.isReading())
					token.mValue.set(serializer.read<uint64>());
				else
					serializer.write(token.mValue.get<uint64>());
				break;
			}

			case Token::Type::IDENTIFIER:
			{
				IdentifierToken& token = token_.as<IdentifierToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				token.mName.serialize(serializer);
				break;
			}

			case Token::Type::PARENTHESIS:
			{
				ParenthesisToken& token = token_.as<ParenthesisToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializer.serializeAs<uint8>(token.mParenthesisType);
				serializeTokenList(serializer, token.mContent, globalsLookup);
				break;
			}

			case Token::Type::COMMA_SEPARATED:
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

			case Token::Type::UNARY_OPERATION:
			{
				UnaryOperationToken& token = token_.as<UnaryOperationToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializer.serializeAs<uint8>(token.mOperator);
				serializeToken(serializer, token.mArgument, globalsLookup);
				break;
			}

			case Token::Type::BINARY_OPERATION:
			{
				BinaryOperationToken& token = token_.as<BinaryOperationToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializer.serializeAs<uint8>(token.mOperator);
				serializeToken(serializer, token.mLeft, globalsLookup);
				serializeToken(serializer, token.mRight, globalsLookup);
				break;
			}

			case Token::Type::VARIABLE:
			{
				RMX_ERROR("Not supported", );
			/*
				VariableToken& token = token_.as<VariableToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				// TODO: Serialize the variable pointer
			*/
				break;
			}

			case Token::Type::FUNCTION:
			{
				RMX_ERROR("Not supported", );
			/*
				FunctionToken& token = token_.as<FunctionToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				// TODO: Serialize the other members
			*/
				break;
			}

			case Token::Type::MEMORY_ACCESS:
			{
				MemoryAccessToken& token = token_.as<MemoryAccessToken>();
				globalsLookup.serializeDataType(serializer, token.mDataType);
				serializeToken(serializer, token.mAddress, globalsLookup);
				break;
			}

			case Token::Type::VALUE_CAST:
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
