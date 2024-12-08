/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/input/ControlsIn.h"

#include "oxygen_netcore/serverclient/ProtocolVersion.h"


GameplayConnector::GameplayConnector() :
	mConnectionManager(&mUDPSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE)
{
}

GameplayConnector::~GameplayConnector()
{
	closeConnections();
}

bool GameplayConnector::setupAsHost()
{
	if (mRole != Role::NONE)
		closeConnections();

	Sockets::startupSockets();
	if (!mUDPSocket.bindToPort(DEFAULT_PORT, USE_IPV6))
	{
		RMX_ASSERT(false, "UDP socket bind to port " << DEFAULT_PORT << " failed");
		return false;
	}

	mRole = Role::HOST;
	mLastCleanupTimestamp = getCurrentTimestamp();

	return true;
}

void GameplayConnector::startConnectToHost(std::string_view hostIP, uint16 hostPort)
{
	if (mRole != Role::NONE)
		closeConnections();

	Sockets::startupSockets();
	if (!mUDPSocket.bindToAnyPort())
		RMX_ERROR("Socket bind to any port failed", return);

	mRole = Role::CLIENT;

	SocketAddress serverAddress(hostIP, hostPort);
	mConnectionToHost.startConnectTo(mConnectionManager, serverAddress, getCurrentTimestamp());
}

void GameplayConnector::closeConnections()
{
	switch (mRole)
	{
		case Role::HOST:
		{
			// Destroy all connections to clients
			for (size_t k = 0; k < mConnectsToClients.size(); ++k)
			{
				// TODO: This just destroys the connection, but doesn't tell the client about it
				mConnectsToClients[k]->clear();
				delete mConnectsToClients[k];
			}
			mConnectsToClients.clear();
			mConnectionToHost.clear();
			break;
		}

		case Role::CLIENT:
		{
			// TODO: This just destroys the connection, but doesn't tell the host about it
			mConnectionToHost.clear();
			break;
		}
	}

	mUDPSocket.close();
	mRole = Role::NONE;
}

void GameplayConnector::updateConnections(float deltaSeconds)
{
	if (mRole == Role::NONE)
		return;

	updateReceivePackets(mConnectionManager);
	mConnectionManager.updateConnections(getCurrentTimestamp());

	// Perform cleanup regularly
	const uint64 currentTimestamp = getCurrentTimestamp();
	if (currentTimestamp - mLastCleanupTimestamp > 5000)	// Every 5 seconds
	{
		performCleanup();
		mLastCleanupTimestamp = currentTimestamp;
	}
}

void GameplayConnector::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber, bool& inputWasInjected)
{
	switch (mRole)
	{
		case Role::HOST:
		{
			// Inject input states from the clients
			controlsIn.injectInput(0, mLastGameStateIncrementPacket.mInput);	// TODO: Make this more flexible, it shouldn't always be player 1
			inputWasInjected = true;
			break;
		}

		case Role::CLIENT:
		{
			// Get current input state and send it to the host
			if (mConnectionToHost.getState() != NetConnection::State::CONNECTED)
				break;

			GameStateIncrementPacket packet;
			packet.mFrameNumber = frameNumber;
			packet.mInput = controlsIn.getInputPad(0);	// TODO: Make this more flexible, it shouldn't always be player 1

			mConnectionToHost.sendPacket(packet);
			break;
		}
	}
}

NetConnection* GameplayConnector::createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress)
{
	if (mRole == Role::HOST)
	{
		// Accept incoming connection
		Connection* connection = new Connection();
		mConnectsToClients.push_back(connection);
		return connection;
	}
	else
	{
		// Reject connection
		return nullptr;
	}
}

void GameplayConnector::destroyNetConnection(NetConnection& connection)
{
	vectorRemoveAll(mConnectsToClients, &connection);
	delete &connection;
}

bool GameplayConnector::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case GameStateIncrementPacket::PACKET_TYPE:
		{
			GameStateIncrementPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			if (mRole == Role::HOST)
			{
				mLastGameStateIncrementPacket = packet;
			}
			return true;
		}
	}

	return false;
}

void GameplayConnector::performCleanup()
{
	// Check for disconnected and empty connection instances
	std::vector<Connection*> connectionsToRemove;
	for (Connection* connection : mConnectsToClients)
	{
		if (connection->getState() == NetConnection::State::DISCONNECTED || connection->getState() == NetConnection::State::EMPTY)
		{
			connectionsToRemove.push_back(connection);
		}
	}
	for (Connection* connection : connectionsToRemove)
	{
		destroyNetConnection(*connection);
	}
}
