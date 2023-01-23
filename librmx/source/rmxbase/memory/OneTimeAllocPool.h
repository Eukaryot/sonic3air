/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	// Fast memory pool that doesn't allow for freeing individual allocations
	class OneTimeAllocPool
	{
	public:
		~OneTimeAllocPool();

		inline void setPageSize(size_t pageSize)  { mPageSize = pageSize; }

		void clear();
		uint8* allocateMemory(size_t bytes);

	private:
		struct Page
		{
			uint8* mData = nullptr;
			size_t mSize = 0;
		};
		std::vector<Page> mPages;

		size_t mPageSize = 0x10000;
		uint8* mNextAllocationPointer = nullptr;
		size_t mRemainingSize = 0;
	};

}
