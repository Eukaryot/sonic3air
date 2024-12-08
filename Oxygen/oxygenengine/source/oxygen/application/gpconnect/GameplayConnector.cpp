/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/gpconnect/GameplayClient.h"
#include "oxygen/application/gpconnect/GameplayHost.h"

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
	closeConnections();

	Sockets::startupSockets();
	if (!mUDPSocket.bindToPort(DEFAULT_PORT, USE_IPV6))
	{
		RMX_ASSERT(false, "UDP socket bind to port " << DEFAULT_PORT << " failed");
		return false;
	}

	mGameplayHost = new GameplayHost(mConnectionManager);
	return true;
}

void GameplayConnector::startConnectToHost(std::string_view hostIP, uint16 hostPort)
{
	closeConnections();

	Sockets::startupSockets();
	if (!mUDPSocket.bindToAnyPort())
		RMX_ERROR("Socket bind to any port failed", return);

	mGameplayClient = new GameplayClient(mConnectionManager);
	mGameplayClient->startConnection(hostIP, hostPort, getCurrentTimestamp());
}

void GameplayConnector::closeConnections()
{
	SAFE_DELETE(mGameplayHost);
	SAFE_DELETE(mGameplayClient);
	mUDPSocket.close();
}

void GameplayConnector::updateConnections(float deltaSeconds)
{
	if (!mUDPSocket.isValid())
		return;

	updateReceivePackets(mConnectionManager);
	mConnectionManager.updateConnections(getCurrentTimestamp());
}

void GameplayConnector::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	if (nullptr != mGameplayHost)
	{
		mGameplayHost->onFrameUpdate(controlsIn, frameNumber);
	}
	if (nullptr != mGameplayClient)
	{
		mGameplayClient->onFrameUpdate(controlsIn, frameNumber);
	}
}

NetConnection* GameplayConnector::createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress)
{
	if (nullptr != mGameplayHost)
	{
		// Accept incoming connection
		return mGameplayHost->createNetConnection(senderAddress);
	}
	else
	{
		// Reject connection
		return nullptr;
	}
}

void GameplayConnector::destroyNetConnection(NetConnection& connection)
{
	if (nullptr != mGameplayHost)
	{
		mGameplayHost->destroyNetConnection(connection);
	}

	delete &connection;
}

bool GameplayConnector::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	if (nullptr != mGameplayHost)
	{
		return mGameplayHost->onReceivedPacket(evaluation);
	}
	if (nullptr != mGameplayClient)
	{
		return mGameplayClient->onReceivedPacket(evaluation);
	}
	return false;
}
