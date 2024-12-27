/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/network/netplay/NetplayManager.h"
#include "oxygen/network/netplay/NetplayClient.h"
#include "oxygen/network/netplay/NetplayHost.h"
#include "oxygen/network/EngineServerClient.h"

#include "oxygen_netcore/serverclient/Packets.h"
#include "oxygen_netcore/serverclient/ProtocolVersion.h"


#define USE_IPv6 true


NetplayManager::NetplayManager() :
	mConnectionManager(&mUDPSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE),
	mExternalAddressQuery(mConnectionManager, USE_IPv6)
{
	mUseIPv6 = USE_IPv6;

#if defined(DEBUG) && 0
	// Just for testing / debugging
	mConnectionManager.mDebugSettings.mSendingPacketLoss = 0.0f;
	mConnectionManager.mDebugSettings.mReceivingPacketLoss = 0.0f;
	mConnectionManager.mDebugSettings.mReceivingDelayAverage = 0.0f;
	mConnectionManager.mDebugSettings.mReceivingDelayVariance = 0.0f;
#endif
}

NetplayManager::~NetplayManager()
{
	closeConnections();
}

bool NetplayManager::setupAsHost(bool registerSessionAtServer, uint16 port)
{
	if (!restartConnection(true, port) || nullptr == mNetplayHost)
		return false;

	if (registerSessionAtServer)
	{
		mNetplayHost->registerAtServer();
	}
	return true;
}

void NetplayManager::startJoinViaServer()
{
	if (!restartConnection(false) || nullptr == mNetplayClient)
		return;

	mNetplayClient->joinViaServer();
}

void NetplayManager::startJoinDirect(std::string_view ip, uint16 port)
{
	if (!restartConnection(false) || nullptr == mNetplayClient)
		return;

	mNetplayClient->connectDirectlyToHost(ip, port);
}

void NetplayManager::closeConnections()
{
	SAFE_DELETE(mNetplayHost);
	SAFE_DELETE(mNetplayClient);
	mExternalAddressQuery.reset();
	mUDPSocket.close();
}

void NetplayManager::updateConnections(float deltaSeconds)
{
	if (!mUDPSocket.isValid())
		return;

	mConnectionManager.updateConnectionManager();

	mExternalAddressQuery.updateQuery(deltaSeconds);

	if (nullptr != mNetplayHost)
	{
		mNetplayHost->updateConnection(deltaSeconds);
	}
	if (nullptr != mNetplayClient)
	{
		mNetplayClient->updateConnection(deltaSeconds);
	}
}

bool NetplayManager::onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation)
{
	if (nullptr != mNetplayHost)
	{
		if (mNetplayHost->onReceivedGameServerPacket(evaluation))
			return true;
	}
	if (nullptr != mNetplayClient)
	{
		if (mNetplayClient->onReceivedGameServerPacket(evaluation))
			return true;
	}
	return false;
}

bool NetplayManager::canBeginNextFrame(uint32 frameNumber)
{
	if (nullptr != mNetplayClient)
	{
		return mNetplayClient->canBeginNextFrame(frameNumber);
	}
	return true;
}

void NetplayManager::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	if (nullptr != mNetplayHost)
	{
		mNetplayHost->onFrameUpdate(controlsIn, frameNumber);
	}
	if (nullptr != mNetplayClient)
	{
		mNetplayClient->onFrameUpdate(controlsIn, frameNumber);
	}
}

NetConnection* NetplayManager::createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress)
{
	if (nullptr != mNetplayHost)
	{
		// Accept incoming connection
		return mNetplayHost->createNetConnection(senderAddress);
	}
	else
	{
		// Reject connection
		return nullptr;
	}
}

void NetplayManager::destroyNetConnection(NetConnection& connection)
{
	if (nullptr != mNetplayHost)
	{
		mNetplayHost->destroyNetConnection(connection);
	}

	delete &connection;
}

bool NetplayManager::onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation)
{
	if (mExternalAddressQuery.onReceivedConnectionlessPacket(evaluation))
		return true;

	return false;
}

bool NetplayManager::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	if (nullptr != mNetplayHost)
	{
		return mNetplayHost->onReceivedPacket(evaluation);
	}
	if (nullptr != mNetplayClient)
	{
		return mNetplayClient->onReceivedPacket(evaluation);
	}
	return false;
}

bool NetplayManager::restartConnection(bool asHost, uint16 hostPort)
{
	closeConnections();
	Sockets::startupSockets();

	const Sockets::ProtocolFamily protocolFamily = mUseIPv6 ? Sockets::ProtocolFamily::IPv6 : Sockets::ProtocolFamily::IPv4;
	if (asHost && hostPort != 0)
	{
		if (!mUDPSocket.bindToPort(hostPort, protocolFamily))
		{
			RMX_ASSERT(false, "UDP socket bind to port " << hostPort << " failed");
			return false;
		}
	}
	else
	{
		if (!mUDPSocket.bindToAnyPort(protocolFamily))
		{
			RMX_ERROR("Socket bind to any port failed", );
			return false;
		}
	}

	if (asHost)
	{
		mNetplayHost = new NetplayHost(mConnectionManager, *this);
	}
	else
	{
		mNetplayClient = new NetplayClient(mConnectionManager, *this);
	}

	mExternalAddressQuery.startQuery();
	return true;
}
