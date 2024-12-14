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

class ControlsIn;
class GameplayConnector;


class GameplayClient
{
public:
	enum class ConnectionState
	{
		NONE,
		CONNECT_TO_SERVER,
		WAIT_FOR_EXTERNAL_ADDRESS,
	};

	struct HostConnection : public NetConnection
	{
		// Nothing here yet
	};

public:
	GameplayClient(ConnectionManager& connectionManager, GameplayConnector& gameplayConnector);
	~GameplayClient();

	inline const HostConnection& getHostConnection() const  { return mHostConnection; }

	void registerAtServer();
	void startConnection(std::string_view hostIP, uint16 hostPort);

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
	GameplayConnector& mGameplayConnector;
	HostConnection mHostConnection;
	ConnectionState mConnectionState = ConnectionState::NONE;

	std::deque<ReceivedFrame> mReceivedFrames;
	uint32 mLatestFrameNumber = 0;
};
