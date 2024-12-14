/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/gpconnect/GameplayHost.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/input/ControlsIn.h"

#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"


namespace
{
	// TODO: Replace this dummy
	struct DummyPacket : public lowlevel::PacketBase
	{
		static const constexpr uint16 SIGNATURE = 0x0013;
		virtual uint16 getSignature() const override  { return SIGNATURE; }
		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override {}
	};
}


GameplayHost::GameplayHost(ConnectionManager& connectionManager, GameplayConnector& gameplayConnector) :
	mConnectionManager(connectionManager),
	mGameplayConnector(gameplayConnector)
{
}

GameplayHost::~GameplayHost()
{
	// Destroy all connections to clients
	for (size_t k = 0; k < mPlayerConnections.size(); ++k)
	{
		// TODO: This just destroys the connection, but doesn't tell the client about it
		mPlayerConnections[k]->clear();
		delete mPlayerConnections[k];
	}
	mPlayerConnections.clear();
}

NetConnection* GameplayHost::createNetConnection(const SocketAddress& senderAddress)
{
	PlayerConnection* connection = new PlayerConnection();

	// TODO: Set player index accordingly, it should rather start at 1 and take into account the other connections' player indices
	connection->mPlayerIndex = (uint8)mPlayerConnections.size();

	mPlayerConnections.push_back(connection);
	return connection;
}

void GameplayHost::destroyNetConnection(NetConnection& connection)
{
	vectorRemoveAll(mPlayerConnections, &connection);
}

void GameplayHost::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	std::vector<PlayerConnection*> activeConnections;
	{
		// First filter out connections that became invalid
		std::vector<PlayerConnection*> connectionsToRemove;
		for (PlayerConnection* playerConnection : mPlayerConnections)
		{
			if (playerConnection->getState() == NetConnection::State::CONNECTED)
			{
				activeConnections.push_back(playerConnection);
			}
			else if (playerConnection->getState() == NetConnection::State::DISCONNECTED || playerConnection->getState() == NetConnection::State::EMPTY)
			{
				connectionsToRemove.push_back(playerConnection);
			}
		}
		for (PlayerConnection* connection : connectionsToRemove)
		{
			destroyNetConnection(*connection);
		}
	}

	if (activeConnections.empty())
		return;

	size_t maxNumFrames = 0;

	// Inject input states from the clients
	for (PlayerConnection* playerConnection : activeConnections)
	{
		// Apply last received input for that player
		const uint16 input = playerConnection->mLastReceivedInput;
		controlsIn.injectInput(playerConnection->mPlayerIndex, input);

		playerConnection->mAppliedInputs.push_back(input);

		// Limit applied inputs history to 30 frames
		while (playerConnection->mAppliedInputs.size() > 30)
			playerConnection->mAppliedInputs.pop_front();

		maxNumFrames = std::max(maxNumFrames, playerConnection->mAppliedInputs.size());
	}

	// Build packet to send to the clients
	GameStateIncrementPacket packet;
	packet.mNumPlayers = (uint8)activeConnections.size();
	packet.mNumFrames = (uint8)maxNumFrames;
	packet.mFrameNumber = frameNumber;
	packet.mInputs.resize(packet.mNumPlayers * packet.mNumFrames);

	if (maxNumFrames > 0)
	{
		// Copy input data from players
		uint16* input = &packet.mInputs[0];
		for (PlayerConnection* playerConnection : activeConnections)
		{
			for (size_t k = 0; k < maxNumFrames - playerConnection->mAppliedInputs.size(); ++k)
			{
				*input = 0;
				++input;
			}
			for (auto it = playerConnection->mAppliedInputs.begin(); it != playerConnection->mAppliedInputs.end(); ++it)
			{
				*input = *it;
				++input;
			}
		}
	}

	for (PlayerConnection* playerConnection : activeConnections)
	{
		playerConnection->sendPacket(packet);
	}
}

bool GameplayHost::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		// TODO: This packet is expected from the normal game server connection, not the host connection!
		case network::ConnectToNetplayPacket::PACKET_TYPE:
		{
			network::ConnectToNetplayPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			switch (packet.mConnectionType)
			{
				case network::NetplayConnectionType::PUNCHTHROUGH:
				{
					// Send some packets towards the client, until receiving a response
					// TODO: This is just a single one... :/
					DummyPacket dummy;
					mConnectionManager.sendConnectionlessLowLevelPacket(dummy, SocketAddress(packet.mConnectToIP, packet.mConnectToPort), 0, 0);
					break;
				}
			}
			
			return true;
		}

		case PlayerInputIncrementPacket::PACKET_TYPE:
		{
			PlayerInputIncrementPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			if (!packet.mInputs.empty())
			{
				PlayerConnection& connection = static_cast<PlayerConnection&>(evaluation.mConnection);

				if (packet.mFrameNumber > connection.mLastReceivedFrameNumber)		// Ignore out-dated packets
				{
					connection.mLastReceivedFrameNumber = packet.mFrameNumber;
					connection.mLastReceivedInput = packet.mInputs.back();
				}
			}

			return true;
		}
	}

	return false;
}
