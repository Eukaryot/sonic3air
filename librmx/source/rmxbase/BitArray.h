/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "rmxbase/ErrorHandler.h"


template<size_t NUM_BITS>
class BitArray
{
public:
	static const constexpr size_t NUM_CHUNKS = (NUM_BITS + 63) / 64;

public:
	void clearAllBits()
	{
		memset(mChunks, 0, sizeof(mChunks));
	}

	void setAllBits()
	{
		memset(mChunks, 0xff, sizeof(mChunks));
	}

	bool isBitSet(size_t index) const
	{
		RMX_ASSERT(index < NUM_BITS, "Invalid index " << index << " for bit array of size " << NUM_BITS);
		return (bool)((mChunks[index >> 6] >> (index & 0x3f)) & 1);
	}

	void clearBit(size_t index)
	{
		RMX_ASSERT(index < NUM_BITS, "Invalid index " << index << " for bit array of size " << NUM_BITS);
		mChunks[index >> 6] &= ~(1ULL << (index & 0x3f));
	}

	void setBit(size_t index)
	{
		RMX_ASSERT(index < NUM_BITS, "Invalid index " << index << " for bit array of size " << NUM_BITS);
		mChunks[index >> 6] |= (1ULL << (index & 0x3f));
	}

	void setBit(size_t index, bool value)
	{
		RMX_ASSERT(index < NUM_BITS, "Invalid index " << index << " for bit array of size " << NUM_BITS);
		if (value)
			mChunks[index >> 6] |= (1ULL << (index & 0x3f));
		else
			mChunks[index >> 6] &= ~(1ULL << (index & 0x3f));
	}

	void setBitsInRange(size_t firstIndex, size_t lastIndex)
	{
		RMX_ASSERT(firstIndex < NUM_BITS, "Invalid first index " << firstIndex << " for bit array of size " << NUM_BITS);
		RMX_ASSERT(lastIndex < NUM_BITS, "Invalid last index " << lastIndex << " for bit array of size " << NUM_BITS);
		const size_t firstChunkIndex = (firstIndex >> 6);
		const size_t lastChunkIndex = (lastIndex >> 6);
		for (size_t chunkIndex = firstChunkIndex; chunkIndex <= lastChunkIndex; ++chunkIndex)
		{
			const size_t firstBitIndex = (chunkIndex == firstChunkIndex) ? (firstIndex & 0x3f) : 0;
			const size_t lastBitIndex = (chunkIndex == lastChunkIndex) ? (lastIndex & 0x3f) : 63;
			const size_t numBits = lastBitIndex - firstBitIndex + 1;
			if (numBits == 64)
			{
				mChunks[chunkIndex] = ~0ULL;	// Set all bits
			}
			else
			{
				mChunks[chunkIndex] |= ((~1ULL << numBits) - 1) << firstBitIndex;
			}
		}
	}

	bool anyBitSetInChunk(size_t chunkIndex) const
	{
		RMX_ASSERT(chunkIndex < NUM_CHUNKS, "Invalid chunk index " << chunkIndex << ", must be below " << NUM_CHUNKS);
		return mChunks[chunkIndex];
	}

	int getNextSetBit(size_t startIndex) const
	{
		RMX_ASSERT(startIndex < NUM_BITS, "Invalid index " << startIndex << " for bit array of size " << NUM_BITS);
		size_t chunkIndex = (startIndex >> 6);
		size_t bitIndex = (startIndex & 0x3f);
		while (chunkIndex < NUM_CHUNKS)
		{
			uint64 shifted = (mChunks[chunkIndex] >> bitIndex);		// Contains all bits in the current chunk, from the current bit index on
			if (shifted == 0)
			{
				// Go to the next chunk
				++chunkIndex;
				bitIndex = 0;
			}
			else if ((shifted & 0xffff) == 0)
			{
				// Skip the next 16 bits
				bitIndex += 16;
			}
			else
			{
				for (; bitIndex < 64; ++bitIndex, shifted >>= 1)
				{
					if (shifted & 1)
					{
						// The current bit is set
						return (int)((chunkIndex << 6) + bitIndex);
					}
				}
			}
		}

		// No set bit found any more
		return -1;
	}

	int getNextClearedBit(size_t startIndex) const
	{
		RMX_ASSERT(startIndex <= NUM_BITS, "Invalid index " << startIndex << " for bit array of size " << NUM_BITS);		// Allow a start index right after the last valid bit index here
		size_t chunkIndex = (startIndex >> 6);
		size_t bitIndex = (startIndex & 0x3f);
		while (chunkIndex < NUM_CHUNKS)
		{
			uint64 shifted = (mChunks[chunkIndex] >> bitIndex);		// Contains all bits in the current chunk, from the current bit index on
			const uint64 allBitsSet = (bitIndex == 0) ? ~0 : (1ULL << bitIndex) - 1;
			if (shifted == allBitsSet || bitIndex >= 64)
			{
				// Go to the next chunk
				++chunkIndex;
				bitIndex = 0;
			}
			else if ((shifted & 0xffff) == 0xffff)
			{
				// Skip the next 16 bits
				bitIndex += 16;
			}
			else
			{
				for (; bitIndex < 64; ++bitIndex, shifted >>= 1)
				{
					if ((shifted & 1) == 0)
					{
						// The current bit is cleared
						return (int)((chunkIndex << 6) + bitIndex);
					}
				}
			}
		}

		// No cleared bit found any more
		return -1;
	}

private:
	uint64 mChunks[NUM_CHUNKS] = { 0 };
};
