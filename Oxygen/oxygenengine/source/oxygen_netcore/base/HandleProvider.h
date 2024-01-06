/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <vector>


template<typename HANDLE, typename CONTENT, size_t DEFAULT_SIZE = 32>
class HandleProvider
{
public:
	struct Entry
	{
		HANDLE mHandle = 0;	// Invalid handle by default
		mutable CONTENT mContent;
	};

public:
	HandleProvider()
	{
		static_assert((DEFAULT_SIZE & (DEFAULT_SIZE - 1)) == 0);	// Make sure it's a power of two
		mEntries.resize(DEFAULT_SIZE);
		mBitmask = DEFAULT_SIZE - 1;
	}

	inline const std::vector<Entry>& getEntries() const	{ return mEntries; }

	static inline bool isValidHandle(HANDLE handle)		{ return (handle != 0); }
	static inline bool isValidEntry(const Entry& entry)	{ return (entry.mHandle != 0); }

	CONTENT* resolveHandle(HANDLE handle)
	{
		Entry& entry = mEntries[handle & mBitmask];
		return (entry.mHandle == handle) ? &entry.mContent : nullptr;
	}

	const Entry& createHandle()
	{
		// Make sure the lookup is always large enough (not filled by more than 75%)
		if (mNumValidEntries + 1 >= mEntries.size() * 3/4)
		{
			const size_t oldSize = mEntries.size();
			const size_t newSize = mEntries.size() * 2;
			mEntries.resize(newSize);
			mBitmask = (HANDLE)(mEntries.size() - 1);

			// Move all entries whose index does not fit any more
			for (size_t k = 0; k < oldSize; ++k)
			{
				Entry& entry = mEntries[k];
				if (isValidEntry(entry))
				{
					const size_t newIndex = (entry.mHandle) & mBitmask;
					if (newIndex != k)
					{
						std::swap(mEntries[newIndex], mEntries[k]);
					}
				}
			}
		}

		// Get a new random handle candidate
		HANDLE newHandle = (HANDLE)rand() + (HANDLE)rand() * RAND_MAX;
		for (size_t tries = 0; tries < mEntries.size(); ++tries)
		{
			if (isValidHandle(newHandle))
			{
				// Check if it's free in the entries list
				Entry& entry = mEntries[newHandle & mBitmask];
				if (!isValidEntry(entry))
				{
					// We're good to go
					entry = Entry { newHandle, CONTENT() };
					++mNumValidEntries;
					return entry;
				}
			}
			++newHandle;
		}

		RMX_ASSERT(false, "Handle creation failed");
		static const Entry FALLBACK = Entry {};
		return FALLBACK;
	}

	void destroyHandle(HANDLE handle)
	{
		Entry& entry = mEntries[handle & mBitmask];
		if (entry.mHandle == handle)
		{
			entry = Entry { 0, CONTENT() };
			--mNumValidEntries;
		}
	}

private:
	std::vector<Entry> mEntries;	// Using the lowest n bits of the handle as index
	size_t mNumValidEntries = 0;
	HANDLE mBitmask = 0;
};
