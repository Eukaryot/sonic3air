/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	// This data hasher implementation uses a mixture of Murmur2 (which is faster for larger chunks of memory)
	// and FNV1a (which can be used to accumulate the chunk hashes easily)
	//  -> TODO: Some accumulative implementation of Murmur2 would be nice for this
	struct QuickDataHasher
	{
		uint64 mHash = 0;
		uint8 mChunk[0x1000];
		size_t mSize = 0;

		QuickDataHasher() : mHash(rmx::startFNV1a_64()) {}
		explicit QuickDataHasher(uint64 initialHash) : mHash(initialHash) {}

		uint64 getHash()
		{
			flush();
			return mHash;
		}

		void flush()
		{
			if (mSize > 0)
			{
				uint64 chunkHash = rmx::getMurmur2_64(mChunk, mSize);
				mHash = rmx::addToFNV1a_64(mHash, (uint8*)&chunkHash, sizeof(chunkHash));
				mSize = 0;
			}
		}

		void prepareNextData(size_t maximumExpectedSize)
		{
			if (mSize + maximumExpectedSize > 0x1000)
			{
				flush();
			}
		}

		void addData(uint8 value)
		{
			mChunk[mSize] = value;
			++mSize;
		}

		void addData(uint64 value)
		{
			memcpy(&mChunk[mSize], &value, sizeof(value));
			++mSize;
		}
	};
}
