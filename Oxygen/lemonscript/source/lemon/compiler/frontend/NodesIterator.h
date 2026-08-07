/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Node.h"


namespace lemon
{

	struct NodesIterator
	{
		BlockNode& mBlockNode;
		size_t mCurrentIndex = 0;
		std::vector<size_t> mIndicesToErase;

		inline NodesIterator(BlockNode& blockNode) :
			mBlockNode(blockNode)
		{}

		inline ~NodesIterator()
		{
			mBlockNode.mNodes.erase(mIndicesToErase);
		}

		inline void operator++()		{ ++mCurrentIndex; }
		inline Node& operator*() const	{ return mBlockNode.mNodes[mCurrentIndex]; }
		inline Node* operator->() const	{ return &mBlockNode.mNodes[mCurrentIndex]; }
		inline bool valid() const		{ return (mCurrentIndex < mBlockNode.mNodes.size()); }
		inline Node* get() const		{ return (mCurrentIndex < mBlockNode.mNodes.size()) ? &mBlockNode.mNodes[mCurrentIndex] : nullptr; }
		inline Node* peek() const		{ return (mCurrentIndex + 1 < mBlockNode.mNodes.size()) ? &mBlockNode.mNodes[mCurrentIndex + 1] : nullptr; }

		template<typename T> inline T* getSpecific() const   { Node* node = get();  return (nullptr != node && node->isA<T>()) ? static_cast<T*>(node) : nullptr; }
		template<typename T> inline T* peekSpecific() const  { Node* node = peek(); return (nullptr != node && node->isA<T>()) ? static_cast<T*>(node) : nullptr; }

		inline void eraseCurrent()
		{
			// For performance reasons, don't erase it here already, but all of these in one go later on
			if (mIndicesToErase.empty())
				mIndicesToErase.reserve(mBlockNode.mNodes.size() / 2);
			mIndicesToErase.push_back(mCurrentIndex);
		}
	};

}
