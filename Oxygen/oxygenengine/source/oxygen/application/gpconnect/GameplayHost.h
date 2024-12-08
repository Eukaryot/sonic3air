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

#include "oxygen/application/gpconnect/GPCPackets.h"

class ControlsIn;


class GameplayHost
{
public:
	struct PlayerConnection : public NetConnection
	{
		uint8 mPlayerIndex = 0;
		uint32 mLastReceivedFrameNumber = 0;
		uint16 mLastReceivedInput = 0;
		std::deque<uint16> mAppliedInputs;		// Applied player inputs for the last x frames
	};

public:
	explicit GameplayHost(ConnectionManager& connectionManager);
	~GameplayHost();

	inline const std::vector<PlayerConnection*>& getPlayerConnections() const  { return mPlayerConnections; }

	NetConnection* createNetConnection(const SocketAddress& senderAddress);
	void destroyNetConnection(NetConnection& connection);

	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber);

	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);

private:
	ConnectionManager& mConnectionManager;
	std::vector<PlayerConnection*> mPlayerConnections;
};
