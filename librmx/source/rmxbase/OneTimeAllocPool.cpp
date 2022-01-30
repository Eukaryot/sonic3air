/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


namespace rmx
{
	OneTimeAllocPool::~OneTimeAllocPool()
	{
		clear();
	}

	void OneTimeAllocPool::clear()
	{
		for (Page& page : mPages)
			delete[] page.mData;
		mPages.clear();
		mNextAllocationPointer = nullptr;
		mRemainingSize = 0;
	}

	uint8* OneTimeAllocPool::allocateMemory(size_t bytes)
	{
		if (bytes > mRemainingSize)
		{
			RMX_CHECK(bytes <= mPageSize, "Too large memory allocation of " << bytes << " bytes", return nullptr);

			// Add a new page
			Page& page = vectorAdd(mPages);
			page.mData = new uint8[mPageSize];
			page.mSize = mPageSize;

			mNextAllocationPointer = page.mData;
			mRemainingSize = mPageSize;
		}

		uint8* ptr = mNextAllocationPointer;
		mNextAllocationPointer += bytes;
		mRemainingSize -= bytes;
		return ptr;
	}
}
