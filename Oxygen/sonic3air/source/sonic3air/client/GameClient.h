/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/client/GhostSync.h"
#include "sonic3air/client/UpdateCheck.h"

#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/serverclient/Packets.h"


#if defined(PLATFORM_WEB)
	#define GAME_CLIENT_USE_WSS		// Emscripten does not support UDP, so we need to use WebSockets
#else
	#define GAME_CLIENT_USE_UDP		// This is the default
	//#define GAME_CLIENT_USE_TCP	// This is just a fallback for platforms which don't support UDP, but TCP (of which we have none at the moment)
#endif


class GameClient : public ServerClientBase, public SingleInstance<GameClient>
{
public:
	enum class ConnectionState
	{
		NOT_CONNECTED,
		CONNECTING,
		ESTABLISHED,
		FAILED
	};

public:
	GameClient();
	~GameClient();

	NetConnection& getServerConnection() { return mServerConnection; }
	GhostSync& getGhostSync()			 { return mGhostSync; }
	UpdateCheck& getUpdateCheck()		 { return mUpdateCheck; }

	void setupClient();
	void updateClient(float timeElapsed);

	inline ConnectionState getConnectionState() const  { return mConnectionState; }
	inline bool isConnected() const					   { return (mConnectionState == ConnectionState::ESTABLISHED); }
	void connectToServer();

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
	enum class State
	{
		NONE,
		STARTED,
		READY,
	};

private:
	void startConnectingToServer(uint64 currentTimestamp);

	void updateNotConnected(uint64 currentTimestamp);
	void updateConnected();

private:
	UDPSocket mUDPSocket;
	ConnectionManager mConnectionManager;
	NetConnection mServerConnection;
	ConnectionState mConnectionState = ConnectionState::NOT_CONNECTED;
	State mState = State::NONE;
	uint64 mLastConnectionAttemptTimestamp = 0;

	GhostSync mGhostSync;
	UpdateCheck mUpdateCheck;
	network::GetServerFeaturesRequest mGetServerFeaturesRequest;
};
