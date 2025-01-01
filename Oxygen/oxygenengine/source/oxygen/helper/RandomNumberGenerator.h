/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class RandomNumberGenerator
{
public:
	void randomize();
	void serialize(VectorBinarySerializer& serializer, uint8 formatVersion);

	uint64 getRandomUint64();

	uint64* accessState()  { return mState; }

private:
	uint64 mState[4] = { 1, 2, 3, 4 };	// Just some arbitrary initial state avoiding that everything is zero
};
