/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/serverclient/Packets.h"

#include "oxygenserver/server/ServerNetConnection.h"
#include "oxygenserver/subsystems/Channels.h"
#include "oxygenserver/subsystems/NetplaySetup.h"
#include "oxygenserver/subsystems/UpdateCheck.h"
#include "oxygenserver/subsystems/VirtualDirectory.h"


class Server : public ConnectionListenerInterface
{
public:
	static inline bool mReceivedCloseEvent = false;

public:
	void runServer();

protected:
	// From ConnectionListenerInterface
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override;
	virtual void destroyNetConnection(NetConnection& connection) override;

	virtual bool onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation) override;
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
	NetplaySetup mNetplaySetup;
	UpdateCheck mUpdateCheck;
	VirtualDirectory mVirtualDirectory;

	// Cached data
	network::GetServerFeaturesRequest mCachedServerFeaturesRequest;
};
