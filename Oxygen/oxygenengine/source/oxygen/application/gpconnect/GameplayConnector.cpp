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


namespace
{
	// TODO: This is just copied from GameClient in S3AIR code, please refactor
	bool resolveGameServerHostName(const std::string& hostName, std::string& outServerIP)
	{
	#if !defined(GAME_CLIENT_USE_WSS)
		// For UDP/TCP, try the "gameserver" subdomain first
		//  -> This does not work for emscripten websockets, see implementation of "resolveToIp"
		//  -> It allows for having different servers for UDP/TCP, and websockets
		if (Sockets::resolveToIP("gameserver." + hostName, outServerIP))
			return true;
	#endif

		// Use the host name itself
		return Sockets::resolveToIP(hostName, outServerIP);
	}
}


GameplayConnector::GameplayConnector() :
	mConnectionManager(&mUDPSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE)
{
}

GameplayConnector::~GameplayConnector()
{
	closeConnections();
}

bool GameplayConnector::setupAsHost(uint16 port, bool useIPv6)
{
	closeConnections();

	Sockets::startupSockets();
	if (!mUDPSocket.bindToPort(port, useIPv6))
	{
		RMX_ASSERT(false, "UDP socket bind to port " << port << " failed");
		return false;
	}

	mGameplayHost = new GameplayHost(mConnectionManager, *this);

	retrieveSocketExternalAddress();
	return true;
}

void GameplayConnector::startConnectToHost(std::string_view hostIP, uint16 hostPort)
{
	closeConnections();

	Sockets::startupSockets();
	if (!mUDPSocket.bindToAnyPort())
	{
		RMX_ERROR("Socket bind to any port failed", );
		return;
	}

	mGameplayClient = new GameplayClient(mConnectionManager, *this);
	mGameplayClient->startConnection(hostIP, hostPort);

	retrieveSocketExternalAddress();
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

void GameplayConnector::retrieveSocketExternalAddress()
{
	mExternalAddressQuery = ExternalAddressQuery();
	mExternalAddressQuery.mQueryID = (uint64)(1 + rand()) + ((uint64)rand() << 16) + ((uint64)rand() << 32) + ((uint64)rand() << 48);

	std::string serverIP;
	if (!resolveGameServerHostName(Configuration::instance().mGameServerBase.mServerHostName, serverIP))
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
