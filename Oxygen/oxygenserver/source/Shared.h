/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionManager.h"


static const Sockets::ProtocolFamily SERVER_PROTOCOL_FAMILY = Sockets::ProtocolFamily::DualStack;

static const uint16 UDP_SERVER_PORT = 21094;	// Only used by test client, for the server see "config.json"
static const uint16 TCP_SERVER_PORT = 21095;	// Only used by test client, for the server see "config.json"


#ifdef DEBUG
static void setupDebugSettings(ConnectionManager::DebugSettings& debugSettings)
{
	debugSettings.mSendingPacketLoss = 0.0f;
	debugSettings.mReceivingPacketLoss = 0.0f;
}
#endif
