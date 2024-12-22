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

class ControlsIn;
class NetplayManager;


class NetplayClient
{
public:
	enum class State
	{
		NONE,				// Not started anything yet
		CONNECT_TO_SERVER,	// Waiting for a game server connection
		REGISTERED,			// Sent registration to game server, now waiting for a "ConnectToNetplayPacket"
		CONNECT_TO_HOST,	// Waiting for connection to host
		CONNECTED,			// Connection to host established
		GAME_RUNNING,		// Game running
		FAILED
	};

	struct HostConnection : public NetConnection
	{
		// Nothing here yet
	};

public:
	NetplayClient(ConnectionManager& connectionManager, NetplayManager& netplayManager);
	~NetplayClient();

	inline State getState() const  { return mState; }
	inline const HostConnection& getHostConnection() const  { return mHostConnection; }

	void joinViaServer();
	void connectDirectlyToHost(std::string_view ip, uint16 port);

	void updateConnection(float deltaSeconds);
	bool onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation);

	bool canBeginNextFrame(uint32 frameNumber);
	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber);
	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);

private:
	struct ReceivedFrame
	{
		uint16 mInputByPlayer[2] = { 0 };
	};

private:
	ConnectionManager& mConnectionManager;
	NetplayManager& mNetplayManager;
	HostConnection mHostConnection;
	State mState = State::NONE;

	network::RegisterForNetplayRequest mRegistrationRequest;

	std::deque<ReceivedFrame> mReceivedFrames;
	uint32 mLatestFrameNumber = 0;
};
