/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/LogDisplay.h"


struct DebugNotificationInterface
{
	virtual void onWatchTriggered(size_t watchIndex, uint32 address, uint16 bytes) = 0;
	virtual void onVRAMWrite(uint16 address, uint16 bytes) = 0;
	virtual void onLog(LogDisplay::ScriptLogSingleEntry& scriptLogSingleEntry) = 0;
};
