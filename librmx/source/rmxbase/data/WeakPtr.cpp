/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace rmx
{

	WeakPtrTarget::~WeakPtrTarget()
	{
		for (WeakPtrBase* weakPtr = mWeakPtrList; nullptr != weakPtr; )
		{
			WeakPtrBase* next = weakPtr->mNext;
			weakPtr->mTarget = nullptr;
			weakPtr->mPrev = nullptr;
			weakPtr->mNext = nullptr;
			weakPtr = next;
		}
		mWeakPtrList = nullptr;
	}


	void WeakPtrBase::clearTarget()
	{
		if (nullptr == mTarget)
			return;

		if (nullptr != mPrev)
			mPrev->mNext = mNext;
		else
			mTarget->mWeakPtrList = mNext;

		if (nullptr != mNext)
			mNext->mPrev = mPrev;

		mTarget = nullptr;
		mPrev = nullptr;
		mNext = nullptr;
	}

	void WeakPtrBase::setTarget(WeakPtrTarget& target)
	{
		if (mTarget == &target)
			return;

		clearTarget();

		mNext = target.mWeakPtrList;
		if (nullptr != mNext)
			mNext->mPrev = this;

		mTarget = &target;
		target.mWeakPtrList = this;
	}

	void WeakPtrBase::setTarget(WeakPtrTarget* target)
	{
		if (nullptr == target)
			clearTarget();
		else
			setTarget(*target);
	}

}
