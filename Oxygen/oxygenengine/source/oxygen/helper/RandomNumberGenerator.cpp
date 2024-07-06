/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/RandomNumberGenerator.h"


namespace detail
{
	template<int k> uint64 rol64(uint64 x) { return (x << k) | (x >> (64 - k)); }
}


void RandomNumberGenerator::randomize()
{
	// Create a random RNG state
	//  -> This is simply using "rand()", as platform independence and determinism won't matter here
	for (int k = 0; k < 4; ++k)
		mState[k] = (uint64)rand() + ((uint64)rand() << 13) + ((uint64)rand() << 26) + ((uint64)rand() << 39) + ((uint64)rand() << 52);
}

void RandomNumberGenerator::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	if (formatVersion >= 6)
	{
		for (int k = 0; k < 4; ++k)
			serializer.serialize(mState[k]);
	}
}

uint64 RandomNumberGenerator::getRandomUint64()
{
	// This is an implementation of xoshiro256** - see https://en.wikipedia.org/wiki/Xorshift#xoshiro
	//  -> Unlike the library random number generators like "std::mersenne_twister_engine", this gives us direct access to the RNG state
	//  -> Also, it has a small memory footprint, i.e. doesn't waste much space in serializations
	//  -> Using this implementation instead of a library one ensures that it behaves the same on all platforms

	uint64* s = mState;
	const uint64 result = detail::rol64<7>(s[1] * 5) * 9;
	const uint64 t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;
	s[3] = detail::rol64<45>(s[3]);

	return result;
}
