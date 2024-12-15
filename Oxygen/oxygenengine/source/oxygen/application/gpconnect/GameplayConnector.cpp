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

#include "oxygen_netcore/serverclient/Packets.h"
#include "oxygen_netcore/serverclient/ProtocolVersion.h"

#include "oxygen/client/EngineServerClient.h"


GameplayConnector::GameplayConnector() :
	mConnectionManager(&mUDPSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE)
{
}

GameplayConnector::~GameplayConnector()
{
	closeConnections();
}

bool GameplayConnector::setupAsHost(bool registerSessionAtServer, uint16 port, bool useIPv6)
{
	if (!restartConnection(true, port, useIPv6) || nullptr == mGameplayHost)
		return false;

	if (registerSessionAtServer)
	{
		mGameplayHost->registerAtServer();
	}
	return true;
}

void GameplayConnector::startJoinViaServer()
{
	if (!restartConnection(false) || nullptr == mGameplayClient)
		return;

	mGameplayClient->joinViaServer();
}

void GameplayConnector::startJoinDirect(std::string_view ip, uint16 port)
{
	if (!restartConnection(false) || nullptr == mGameplayClient)
		return;

	mGameplayClient->connectDirectlyToHost(ip, port);
}

void GameplayConnector::closeConnections()
{
	SAFE_DELETE(mGameplayHost);
	SAFE_DELETE(mGameplayClient);
	mExternalAddressQuery = ExternalAddressQuery();
	mUDPSocket.close();
}

void GameplayConnector::updateConnections(float deltaSeconds)
{
	if (!mUDPSocket.isValid())
		return;

	mConnectionManager.updateConnectionManager();

	if (nullptr != mGameplayHost)
	{
		mGameplayHost->updateConnection(deltaSeconds);
	}
	if (nullptr != mGameplayClient)
	{
		mGameplayClient->updateConnection(deltaSeconds);
	}
}

bool GameplayConnector::onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation)
{
	if (nullptr != mGameplayHost)
	{
		if (mGameplayHost->onReceivedGameServerPacket(evaluation))
			return true;
	}
	if (nullptr != mGameplayClient)
	{
		if (mGameplayClient->onReceivedGameServerPacket(evaluation))
			return true;
	}
	return false;
}

bool GameplayConnector::canBeginNextFrame(uint32 frameNumber)
{
	if (nullptr != mGameplayClient)
	{
		return mGameplayClient->canBeginNextFrame(frameNumber);
	}
	return true;
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

bool GameplayConnector::onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation)
{
	switch (evaluation.mLowLevelSignature)
	{
		case network::ReplyExternalAddressConnectionless::SIGNATURE:
		{
			network::ReplyExternalAddressConnectionless packet;
			if (!packet.serializePacket(evaluation.mSerializer, 1))
				return false;

			if (mExternalAddressQuery.mQueryID == packet.mQueryID)
			{
				mExternalAddressQuery.mOwnExternalIP = packet.mIP;
				mExternalAddressQuery.mOwnExternalPort = packet.mPort;
			}
			return true;
		}
	}

	return false;
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

bool GameplayConnector::restartConnection(bool asHost, uint16 hostPort, bool useIPv6)
{
	closeConnections();
	Sockets::startupSockets();

	if (asHost && hostPort != 0)
	{
		if (!mUDPSocket.bindToPort(hostPort, useIPv6))
		{
			RMX_ASSERT(false, "UDP socket bind to port " << hostPort << " failed");
			return false;
		}
	}
	else
	{
		if (!mUDPSocket.bindToAnyPort())
		{
			RMX_ERROR("Socket bind to any port failed", );
			return false;
		}
	}

	if (asHost)
	{
		mGameplayHost = new GameplayHost(mConnectionManager, *this);
	}
	else
	{
		mGameplayClient = new GameplayClient(mConnectionManager, *this);
	}

	retrieveSocketExternalAddress();
	return true;
}

void GameplayConnector::retrieveSocketExternalAddress()
{
	mExternalAddressQuery = ExternalAddressQuery();
	mExternalAddressQuery.mQueryID = (uint64)(1 + rand()) + ((uint64)rand() << 16) + ((uint64)rand() << 32) + ((uint64)rand() << 48);

	std::string serverIP;
	if (!EngineServerClient::resolveGameServerHostName(Configuration::instance().mGameServerBase.mServerHostName, serverIP))
	{
		// TODO: Error handling
		RMX_ASSERT(false, "Failed to resolve game server host name");
		return;
	}

	// Retrieve external address for the socket
	// TODO: This needs to be repeated if necessary
	network::GetExternalAddressConnectionless packet;
	packet.mQueryID = mExternalAddressQuery.mQueryID;
	mConnectionManager.sendConnectionlessLowLevelPacket(packet, SocketAddress(serverIP, Configuration::instance().mGameServerBase.mServerPortUDP), 0, 0);
}
