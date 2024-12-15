/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/network/netplay/NetplayManager.h"

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/serverclient/Packets.h"


class EngineServerClient : public ConnectionListenerInterface, public SingleInstance<EngineServerClient>
{
public:
	struct Listener
	{
		virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) = 0;
	};

public:
	enum class ConnectionState
	{
		NOT_CONNECTED,		// Not connected to server at all
		CONNECTING,			// Connection has just started
		WAIT_FOR_FEATURES,	// Connection established, still waiting for server features response
		READY,				// Connection established and server features available
		FAILED				// Connection failed
	};

	enum class SocketUsage
	{
		UDP,			// Use UDP for server connection - this is the default
		TCP,			// Use TCP as a fallback for platforms which don't support UDP, but TCP (of which we have none at the moment)
		WEBSOCKETS,		// Use Websockets / WSS (building upon TCP), if that's the only thing available
	};

public:
	static SocketUsage getSocketUsage();
	static bool resolveGameServerHostName(const std::string& hostName, std::string& outServerIP);

public:
	EngineServerClient();
	~EngineServerClient();

	inline NetConnection& getServerConnection() { return mServerConnection; }

	void setListener(Listener* listener)  { mListener = listener; }

	bool setupClient();
	void updateClient(float timeElapsed);

	inline ConnectionState getConnectionState() const  { return mConnectionState; }
	inline bool isConnected() const					   { return (mConnectionState == ConnectionState::READY); }
	void connectToServer();

	inline bool hasReceivedServerFeatures() const	   { return (mConnectionState == ConnectionState::READY); }
	const network::GetServerFeaturesRequest::Response& getServerFeatures() const;

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override
	{
		// Do not allow incoming connections
		return nullptr;
	}

	virtual void destroyNetConnection(NetConnection& connection) override
	{
		RMX_ASSERT(false, "This should never get called");
	}

	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override;

private:
	void startConnectingToServer();

private:
	UDPSocket mUDPSocket;
	ConnectionManager mConnectionManager;
	NetConnection mServerConnection;

	NetplayManager mNetplayManager;
	Listener* mListener = nullptr;

	ConnectionState mConnectionState = ConnectionState::NOT_CONNECTED;
	uint64 mLastConnectionAttemptTimestamp = 0;

	network::GetServerFeaturesRequest mGetServerFeaturesRequest;
};
