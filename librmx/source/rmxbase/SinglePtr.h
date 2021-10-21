/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// SinglePtr
template<class CLASS> class SinglePtr
{
private:
	static inline CLASS* mPointer = nullptr;
	static inline int mRefCounter = 0;
	bool mIsWeak;

public:
	SinglePtr(bool weak = false)
	{
		mIsWeak = weak;
		if (mIsWeak)
			return;

		if (nullptr == mPointer)
		{
			mPointer = new CLASS();
			assert(mRefCounter == 0);
		}
		++mRefCounter;
	}

	~SinglePtr()
	{
		if (mIsWeak)
			return;

		assert(mRefCounter > 0);
		--mRefCounter;
		if (mRefCounter == 0)
			SAFE_DELETE(mPointer);
	}

	bool valid() const			{ return mPointer != nullptr; }

	CLASS& operator*() const	{ return *mPointer; }
	operator CLASS*() const		{ return mPointer; }
	CLASS* operator->() const	{ return mPointer; }
};


// WeakSinglePtr
template<class CLASS> class WeakSinglePtr : public SinglePtr<CLASS>
{
public:
	WeakSinglePtr() : SinglePtr<CLASS>(true) {}
};
