/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/gpconnect/GameplayClient.h"
#include "oxygen/application/gpconnect/GPCPackets.h"
#include "oxygen/application/input/ControlsIn.h"


GameplayClient::GameplayClient(ConnectionManager& connectionManager) :
	mConnectionManager(connectionManager)
{
}

GameplayClient::~GameplayClient()
{
	// TODO: This just destroys the connection, but doesn't tell the host about it
	mHostConnection.clear();
}

void GameplayClient::startConnection(std::string_view hostIP, uint16 hostPort, uint64 timestamp)
{
	SocketAddress serverAddress(hostIP, hostPort);
	mHostConnection.startConnectTo(mConnectionManager, serverAddress, timestamp);
}

void GameplayClient::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	if (mHostConnection.getState() != NetConnection::State::CONNECTED)
		return;

	// Get current input state and send it to the host
	PlayerInputIncrementPacket packet;
	packet.mFrameNumber = frameNumber;
	packet.mNumFrames = 1;
	packet.mInputs.push_back(controlsIn.getInputPad(0));

	mHostConnection.sendPacket(packet);
}

bool GameplayClient::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case GameStateIncrementPacket::PACKET_TYPE:
		{
			GameStateIncrementPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			// TODO:
			//  - Evaluate the packet and store its input history for all players
			//  - Use that input data for the simulation
			//  - Control simulation speed (incl. possible stops) depending on the last received frame number

			return true;
		}
	}

	return false;
}
