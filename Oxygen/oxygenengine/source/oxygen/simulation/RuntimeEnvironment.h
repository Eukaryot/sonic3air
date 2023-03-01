/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/runtime/Runtime.h"

class EmulatorInterface;


struct RuntimeEnvironment : public lemon::Environment
{
	static const uint64 TYPE = rmx::compileTimeFNV_32("Oxygen_RuntimeEnvironment");

	inline RuntimeEnvironment() : lemon::Environment(TYPE) {}

	EmulatorInterface* mEmulatorInterface = nullptr;
};
