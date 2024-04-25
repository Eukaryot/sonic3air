/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


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
		// Always round up to a multiple of 8 bytes, to ensure correct memory alignment on 64-bit machines (avoiding SIGBUS fault on ARM)
	#if !defined(PLATFORM_VITA)
		bytes = ((bytes + 7) & ~(size_t)0x07);
	#else
		// Let's use 4 bytes for the PSVITA
		bytes = ((bytes + 3) & ~(size_t)0x03);
	#endif
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
