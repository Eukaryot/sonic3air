/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Node.h"
#include "lemon/compiler/Utility.h"


namespace lemon
{
	class BlockNode;

	class BlockNodeStack
	{
	public:
		BlockNodeStack() = default;
		explicit BlockNodeStack(BlockNode& node) : mStack{&node} {}

		template<typename T>
		T& appendNode(uint32 lineNumber)
		{
			T& node = mStack.back()->mNodes.createBack<T>();
			node.setLineNumber(lineNumber);
			return node;
		}

		void pushBlockNode(uint32 lineNumber)
		{
			BlockNode& node = appendNode<BlockNode>(lineNumber);
			mStack.push_back(&node);		// Also push as new block to the stack
		}

		void popBlockNode(uint32 lineNumber)
		{
			mStack.pop_back();
			CHECK_ERROR(!mStack.empty(), "Closed too many blocks", lineNumber);
		}

	public:
		std::vector<BlockNode*> mStack;
	};

}
