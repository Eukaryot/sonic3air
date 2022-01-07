/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"

class ConnectionManager;
class NetConnection;


class ServerClientBase : public ConnectionListenerInterface
{
public:
	static uint64 getCurrentTimestamp();

protected:
	bool updateReceivePackets(ConnectionManager& connectionManager);

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) = 0;
	virtual void destroyNetConnection(NetConnection& connection) = 0;

private:
	void handleConnectionStartPacket(ConnectionManager& connectionManager, const ReceivedPacket& receivedPacket);
};
