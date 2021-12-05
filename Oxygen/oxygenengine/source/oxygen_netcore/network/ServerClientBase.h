/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"

class ConnectionManager;
class NetConnection;


class ServerClientBase
{
protected:
	bool updateReceivePackets(ConnectionManager& connectionManager);

	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) = 0;

protected:
	struct ConnectionIDProvider
	{
		uint16 getNextID();
		uint16 mNextID = 1;
	};

protected:
	ConnectionIDProvider mConnectionIDProvider;
};
