/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"

#include "oxygen/application/gpconnect/GPCPackets.h"

class ControlsIn;
class GameplayConnector;


class GameplayHost
{
public:
	enum class State
	{
		NONE,				// Idle, but reacting to direct connections
		CONNECT_TO_SERVER,	// Waiting for a game server connection
		REGISTERED,			// Sent registration to game server, now waiting for a client to join with a "ConnectToNetplayPacket"
		PUNCHTHROUGH,
		RUNNING,			// Connection to client established
		FAILED
	};

	struct PlayerConnection : public NetConnection
	{
		uint8 mPlayerIndex = 0;
		uint32 mLastReceivedFrameNumber = 0;
		uint16 mLastReceivedInput = 0;
		std::deque<uint16> mAppliedInputs;		// Applied player inputs for the last x frames
	};

public:
	GameplayHost(ConnectionManager& connectionManager, GameplayConnector& gameplayConnector);
	~GameplayHost();

	inline State getState() const  { return mState; }
	inline const std::vector<PlayerConnection*>& getPlayerConnections() const  { return mPlayerConnections; }

	void registerAtServer();

	NetConnection* createNetConnection(const SocketAddress& senderAddress);
	void destroyNetConnection(NetConnection& connection);

	void updateConnection(float deltaSeconds);
	bool onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation);

	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber);
	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);

private:
	ConnectionManager& mConnectionManager;
	GameplayConnector& mGameplayConnector;
	State mState = State::NONE;

	network::RegisterForNetplayRequest mRegistrationRequest;

	std::vector<PlayerConnection*> mPlayerConnections;
};
