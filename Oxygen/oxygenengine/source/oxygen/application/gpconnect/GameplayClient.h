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


class GameplayClient
{
public:
	struct HostConnection : public NetConnection
	{
		// Nothing here yet
	};

public:
	explicit GameplayClient(ConnectionManager& connectionManager);
	~GameplayClient();

	inline const HostConnection& getHostConnection() const  { return mHostConnection; }

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
	HostConnection mHostConnection;

	std::deque<ReceivedFrame> mReceivedFrames;
	uint32 mLatestFrameNumber = 0;
};
