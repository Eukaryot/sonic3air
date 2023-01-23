/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/serverclient/Packets.h"

#include "oxygenserver/server/ServerNetConnection.h"
#include "oxygenserver/subsystems/Channels.h"
#include "oxygenserver/subsystems/UpdateCheck.h"
#include "oxygenserver/subsystems/VirtualDirectory.h"


class Server : public ServerClientBase
{
public:
	void runServer();

protected:
	// From ServerClientBase
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override;
	virtual void destroyNetConnection(NetConnection& connection) override;

	// From ConnectionListenerInterface
	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override;
	virtual bool onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation) override;

private:
	void performCleanup();

private:
	// Connection management
	std::unordered_map<uint32, ServerNetConnection*> mNetConnectionsByPlayerID;
	ObjectPool<ServerNetConnection> mNetConnectionPool;
	uint64 mLastCleanupTimestamp = 0;

	// Sub-systems
	Channels mChannels;
	UpdateCheck mUpdateCheck;
	VirtualDirectory mVirtualDirectory;

	// Cached data
	network::GetServerFeaturesRequest mCachedServerFeaturesRequest;
};
