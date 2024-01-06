/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct DebugNotificationInterface
{
	virtual void onScriptLog(std::string_view key, std::string_view value) = 0;
	virtual void onWatchTriggered(size_t watchIndex, uint32 address, uint16 bytes) = 0;
	virtual void onVRAMWrite(uint16 address, uint16 bytes) = 0;
};
