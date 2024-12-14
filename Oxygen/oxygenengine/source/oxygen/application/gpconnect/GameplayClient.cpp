/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/gpconnect/GameplayClient.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/gpconnect/GPCPackets.h"
#include "oxygen/application/input/ControlsIn.h"

#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"
#include "oxygen_netcore/serverclient/Packets.h"


GameplayClient::GameplayClient(ConnectionManager& connectionManager, GameplayConnector& gameplayConnector) :
	mConnectionManager(connectionManager),
	mGameplayConnector(gameplayConnector)
{
}

GameplayClient::~GameplayClient()
{
	// TODO: This just destroys the connection, but doesn't tell the host about it
	mHostConnection.clear();
}

void GameplayClient::registerAtServer()
{
	// TODO: This should not use the host connection, but the normal game server connection (which unfortunately only exists in the S3AIR code...)
	SocketAddress serverAddress("127.0.0.1", 21094);
	mHostConnection.startConnectTo(mConnectionManager, serverAddress);

	mConnectionState = ConnectionState::CONNECT_TO_SERVER;
}

void GameplayClient::startConnection(std::string_view hostIP, uint16 hostPort)
{
	SocketAddress hostAddress(hostIP, hostPort);
	mHostConnection.startConnectTo(mConnectionManager, hostAddress);
}

bool GameplayClient::canBeginNextFrame(uint32 frameNumber)
{
	// Don't proceed beyond the latest received frame number
	if (frameNumber > mLatestFrameNumber)
		return false;

	return true;
}

void GameplayClient::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	if (mHostConnection.getState() != NetConnection::State::CONNECTED)
		return;

	// Get current input state and send it to the host
	{
		PlayerInputIncrementPacket packet;
		packet.mFrameNumber = frameNumber;
		packet.mNumFrames = 1;
		packet.mInputs.push_back(controlsIn.getInputFromController(0));

		mHostConnection.sendPacket(packet);
	}

	// Inject input from what we received from host
	if (!mReceivedFrames.empty())
	{
		const int indexFromBack = mLatestFrameNumber - frameNumber;
		if (indexFromBack >= 0 && indexFromBack < (int)mReceivedFrames.size())
		{
			auto it = mReceivedFrames.rbegin();
			for (int k = 0; k < indexFromBack; ++k)
				++it;

			for (int playerIndex = 0; playerIndex < 2; ++playerIndex)
			{
				controlsIn.injectInput(playerIndex, it->mInputByPlayer[playerIndex]);
			}
		}
	}
}

bool GameplayClient::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		// TODO: This packet is expected from the normal game server connection, not the host connection!
		case network::ConnectToNetplayPacket::PACKET_TYPE:
		{
			network::ConnectToNetplayPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			// Connect to the given address
			mHostConnection.startConnectTo(mConnectionManager, SocketAddress(packet.mConnectToIP, packet.mConnectToPort));
			return true;
		}

		case GameStateIncrementPacket::PACKET_TYPE:
		{
			GameStateIncrementPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			if (packet.mFrameNumber > mLatestFrameNumber)
			{
				// Evaluate the packet and store its input history for all players
				const size_t currentFramesGap = packet.mFrameNumber - mLatestFrameNumber;
				const size_t numFramesToCopy = std::min<size_t>(currentFramesGap, packet.mNumFrames);
				if (numFramesToCopy < currentFramesGap)
				{
					// Throw away old frames, to not create an actual gap between frames in the queue
					mReceivedFrames.clear();
				}

				const size_t inputsBaseIndex = packet.mNumFrames - numFramesToCopy;

				for (int k = 0; k < (int)numFramesToCopy; ++k)
				{
					mReceivedFrames.emplace_back();
					ReceivedFrame& newFrame = mReceivedFrames.back();
					for (int playerIndex = 0; playerIndex < packet.mNumPlayers; ++playerIndex)
					{
						newFrame.mInputByPlayer[playerIndex] = packet.mInputs[inputsBaseIndex + k + packet.mNumFrames * playerIndex];
					}
					// Player inputs not included in the packet stay at 0
				}

				mLatestFrameNumber = packet.mFrameNumber;
			}

			return true;
		}
	}

	return false;
}
