/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/helper/RandomNumberGenerator.h"


class SimulationState
{
public:
	void reset();
	void serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion);

	inline RandomNumberGenerator& getRandomNumberGenerator()  { return mRandomNumberGenerator; }

private:
	RandomNumberGenerator mRandomNumberGenerator;
};
