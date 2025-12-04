/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/basics/GenericManager.h"
#include "lemon/compiler/TokenTypes.h"


namespace lemon
{

	class Function;
	class ScriptFunction;

	class API_EXPORT Node : public genericmanager::Element<Node>
	{
	public:
		enum class Type : uint8
		{
			UNDEFINED,
			BLOCK,
			PRAGMA,
			FUNCTION,
			LABEL,
			JUMP,
			JUMP_INDIRECT,
			BREAK,
			CONTINUE,
			RETURN,
			EXTERNAL,
			STATEMENT,
			IF_STATEMENT,
			WHILE_STATEMENT,
			FOR_STATEMENT
		};

	public:
		virtual ~Node() {}

		inline Type getType() const  { return (Type)genericmanager::Element<Node>::getType(); }

		inline uint32 getLineNumber() const			  { return mLineNumber; }
		inline void setLineNumber(uint32 lineNumber)  { mLineNumber = lineNumber; }

	protected:
		inline explicit Node(uint32 type) : genericmanager::Element<Node>(type) {}

	private:
		uint32 mLineNumber = 0;
	};


	template<typename T>
	class NodePtr : public genericmanager::ElementPtr<T, Node>
	{
	public:
		using genericmanager::ElementPtr<T, Node>::operator=;
	};


	class API_EXPORT NodeList : public genericmanager::ElementList<Node, 32>
	{
	};


	class API_EXPORT NodeFactory
	{
	public:
		template<typename T>
		static T& create()
		{
			return genericmanager::Manager<Node>::create<T>();
		}
	};



	#define DEFINE_LEMON_NODE_TYPE(_class_, _type_) \
		DEFINE_GENERIC_MANAGER_ELEMENT_TYPE(Node, Node, _class_, (uint32)_type_)


	// Concrete node types

	class API_EXPORT UndefinedNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(UndefinedNode, Type::UNDEFINED)

	public:
		TokenList mTokenList;
	};


	class API_EXPORT BlockNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(BlockNode, Type::BLOCK)

	public:
		NodeList mNodes;
	};


	class PragmaNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(PragmaNode, Type::PRAGMA)

	public:
		std::string mContent;
	};


	class FunctionNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(FunctionNode, Type::FUNCTION)

	public:
		ScriptFunction* mFunction = nullptr;
		NodePtr<BlockNode> mContent;
	};


	class LabelNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(LabelNode, Type::LABEL)

	public:
		FlyweightString mLabel;
		std::vector<ScriptFunction::AddressHook> mAddressHooks;
	};


	class JumpNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(JumpNode, Type::JUMP)

	public:
		TokenPtr<LabelToken> mLabelToken;
	};


	class JumpIndirectNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(JumpIndirectNode, Type::JUMP_INDIRECT)

	public:
		TokenPtr<StatementToken> mIndexToken;
		std::vector<TokenPtr<LabelToken>> mLabelTokens;
	};


	class BreakNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(BreakNode, Type::BREAK)
	};


	class ContinueNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(ContinueNode, Type::CONTINUE)
	};


	class ReturnNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(ReturnNode, Type::RETURN)

	public:
		TokenPtr<StatementToken> mStatementToken;
	};


	class ExternalNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(ExternalNode, Type::EXTERNAL)

	public:
		enum class SubType : uint8
		{
			EXTERNAL_CALL,
			EXTERNAL_JUMP
		};

	public:
		TokenPtr<StatementToken> mStatementToken;
		SubType mSubType = SubType::EXTERNAL_CALL;
	};


	class StatementNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(StatementNode, Type::STATEMENT)

	public:
		TokenPtr<StatementToken> mStatementToken;
	};


	class IfStatementNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(IfStatementNode, Type::IF_STATEMENT)

	public:
		TokenPtr<StatementToken> mConditionToken;
		NodePtr<Node> mContentIf;
		NodePtr<Node> mContentElse;
	};


	class WhileStatementNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(WhileStatementNode, Type::WHILE_STATEMENT)

	public:
		TokenPtr<StatementToken> mConditionToken;
		NodePtr<Node> mContent;
	};


	class ForStatementNode : public Node
	{
	public:
		DEFINE_LEMON_NODE_TYPE(ForStatementNode, Type::FOR_STATEMENT)

	public:
		TokenPtr<StatementToken> mInitialToken;
		TokenPtr<StatementToken> mConditionToken;
		TokenPtr<StatementToken> mIterationToken;
		NodePtr<Node> mContent;
	};


	#undef DEFINE_LEMON_NODE_TYPE

}
