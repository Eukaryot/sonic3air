/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/GenericManager.h"
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

		inline Type getType() const { return mType; }

		template<typename T> const T& as() const { return *static_cast<const T*>(this); }
		template<typename T> T& as() { return *static_cast<T*>(this); }

		inline uint32 getLineNumber() const  { return mLineNumber; }
		inline void setLineNumber(uint32 lineNumber)  { mLineNumber = lineNumber; }

	protected:
		inline Node(Type type) : genericmanager::Element<Node>((uint32)type), mType(type) {}

	private:
		const Type mType;
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



	// Concrete node types

	class API_EXPORT UndefinedNode : public Node
	{
	public:
		static const Type TYPE = Type::UNDEFINED;

	public:
		inline UndefinedNode() : Node(TYPE) {}

	public:
		TokenList mTokenList;
	};


	class API_EXPORT BlockNode : public Node
	{
	public:
		static const Type TYPE = Type::BLOCK;

	public:
		inline BlockNode() : Node(TYPE) {}
		inline virtual ~BlockNode() {}

	public:
		NodeList mNodes;
	};


	class PragmaNode : public Node
	{
	public:
		static const Type TYPE = Type::PRAGMA;

	public:
		inline PragmaNode() : Node(TYPE) {}

	public:
		std::string mContent;
	};


	class FunctionNode : public Node
	{
	public:
		static const Type TYPE = Type::FUNCTION;

	public:
		inline FunctionNode() : Node(TYPE) {}
		inline ~FunctionNode() {}

	public:
		ScriptFunction* mFunction = nullptr;
		NodePtr<BlockNode> mContent;
	};


	class LabelNode : public Node
	{
	public:
		static const Type TYPE = Type::LABEL;

	public:
		inline LabelNode() : Node(TYPE) {}

	public:
		std::string mLabel;
	};


	class JumpNode : public Node
	{
	public:
		static const Type TYPE = Type::JUMP;

	public:
		inline JumpNode() : Node(TYPE) {}

	public:
		TokenPtr<LabelToken> mLabelToken;
	};


	class BreakNode : public Node
	{
	public:
		static const Type TYPE = Type::BREAK;

	public:
		inline BreakNode() : Node(TYPE) {}
	};


	class ContinueNode : public Node
	{
	public:
		static const Type TYPE = Type::CONTINUE;

	public:
		inline ContinueNode() : Node(TYPE) {}
	};


	class ReturnNode : public Node
	{
	public:
		static const Type TYPE = Type::RETURN;

	public:
		inline ReturnNode() : Node(TYPE) {}

	public:
		TokenPtr<StatementToken> mStatementToken;
	};


	class ExternalNode : public Node
	{
	public:
		static const Type TYPE = Type::EXTERNAL;

	public:
		enum class SubType : uint8
		{
			EXTERNAL_CALL,
			EXTERNAL_JUMP
		};

	public:
		inline ExternalNode() : Node(TYPE) {}

	public:
		TokenPtr<StatementToken> mStatementToken;
		SubType mSubType = SubType::EXTERNAL_CALL;
	};


	class StatementNode : public Node
	{
	public:
		static const Type TYPE = Type::STATEMENT;

	public:
		inline StatementNode() : Node(TYPE) {}

	public:
		TokenPtr<StatementToken> mStatementToken;
	};


	class IfStatementNode : public Node
	{
	public:
		static const Type TYPE = Type::IF_STATEMENT;

	public:
		inline IfStatementNode() : Node(TYPE) {}

	public:
		TokenPtr<StatementToken> mConditionToken;
		NodePtr<Node> mContentIf;
		NodePtr<Node> mContentElse;
	};


	class WhileStatementNode : public Node
	{
	public:
		static const Type TYPE = Type::WHILE_STATEMENT;

	public:
		inline WhileStatementNode() : Node(TYPE) {}

	public:
		TokenPtr<StatementToken> mConditionToken;
		NodePtr<Node> mContent;
	};


	class ForStatementNode : public Node
	{
	public:
		static const Type TYPE = Type::FOR_STATEMENT;

	public:
		inline ForStatementNode() : Node(TYPE) {}

	public:
		TokenPtr<StatementToken> mInitialToken;
		TokenPtr<StatementToken> mConditionToken;
		TokenPtr<StatementToken> mIterationToken;
		NodePtr<Node> mContent;
	};


}
