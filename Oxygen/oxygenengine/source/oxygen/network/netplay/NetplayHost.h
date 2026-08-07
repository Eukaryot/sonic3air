/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"

#include "oxygen/network/netplay/NetplayPackets.h"

class ControlsIn;
class NetplayManager;


class NetplayHost
{
public:
	static const int MAX_PLAYERS = 4;

	enum class HostState
	{
		NONE,				// Idle, but reacting to direct connections
		CONNECT_TO_SERVER,	// Waiting for a game server connection
		REGISTERED,			// Sent registration to game server, now waiting for a client to join with a "ConnectToNetplayPacket"
		GAME_RUNNING,		// Game is running
		FAILED
	};

	struct ConnectingPlayer
	{
		SocketAddress mRemoteAddress;
		uint64 mSessionID = 0;
		int mSentPackets = 0;
		float mSendTimer = 0.0f;
	};

	struct PlayerConnection : public NetConnection
	{
		uint8 mPlayerIndex = 0;
		uint32 mStartGamePacketID = 0;
		uint32 mLastReceivedFrameNumber = 0;
		uint16 mLastReceivedInput = 0;
		int mCurrentLatency = 0;
	};

	struct InputFrame
	{
		uint16 mInputsByPlayer[MAX_PLAYERS] = { 0 };
	};

public:
	NetplayHost(ConnectionManager& connectionManager, NetplayManager& netplayManager);
	~NetplayHost();

	inline HostState getHostState() const  { return mHostState; }
	inline const std::vector<PlayerConnection*>& getPlayerConnections() const  { return mPlayerConnections; }

	void registerAtServer();

	NetConnection* createNetConnection(const SocketAddress& senderAddress);
	void destroyNetConnection(NetConnection& connection);

	void updateConnection(float deltaSeconds);
	bool onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation);

	void startGame();

	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber);
	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);

	uint32 getRegularInputChecksum(int& outFrameNumber) const;

private:
	void sendPunchthroughPacket(ConnectingPlayer& player);

private:
	ConnectionManager& mConnectionManager;
	NetplayManager& mNetplayManager;
	HostState mHostState = HostState::NONE;

	network::RegisterForNetplayRequest mRegistrationRequest;

	std::vector<ConnectingPlayer> mConnectingPlayers;
	std::vector<PlayerConnection*> mPlayerConnections;
	std::deque<InputFrame> mInputHistory;

	uint32 mInputChecksum = 0;
	uint32 mRegularInputChecksum = 0;
	int mRegularChecksumFrameNumber = 0;
};
