/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/SimulationState.h"


void SimulationState::reset()
{
	mRandomNumberGenerator.randomize();
}

void SimulationState::serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	mRandomNumberGenerator.serialize(serializer, formatVersion);
}
