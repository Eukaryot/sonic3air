/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/server/Server.h"

#include "oxygen_netcore/serverclient/ProtocolVersion.h"

#include "PrivatePackets.h"
#include "Shared.h"

#include <thread>


void Server::runServer()
{
	// Setup sockets & connection manager
	UDPSocket udpSocket;
	if (!udpSocket.bindToPort(UDP_SERVER_PORT))
		RMX_ERROR("UDP socket bind to port " << UDP_SERVER_PORT << " failed", return);
	RMX_LOG_INFO("UDP socket bound to port " << UDP_SERVER_PORT);

	TCPSocket tcpListenSocket;
	if (!tcpListenSocket.setupServer(TCP_SERVER_PORT))
		RMX_ERROR("TCP socket bind to port " << TCP_SERVER_PORT << " failed", return);
	RMX_LOG_INFO("TCP socket bound to port " << TCP_SERVER_PORT);

	ConnectionManager connectionManager(&udpSocket, &tcpListenSocket, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE);
#ifdef DEBUG
	setupDebugSettings(connectionManager.mDebugSettings);
#endif
	RMX_LOG_INFO("Ready for connections");

	// Prepare cached data
	{
		// Fill in available features
		mCachedServerFeaturesRequest.mResponse.mFeatures.emplace_back(network::GetServerFeaturesRequest::Response::Feature("app-update-check", 1, 1));
		mCachedServerFeaturesRequest.mResponse.mFeatures.emplace_back(network::GetServerFeaturesRequest::Response::Feature("channel-broadcasting", 1, 1));
	}

	// Setup sub-systems
	mVirtualDirectory.startup();

	// Prepare timing
	uint64 lastTimestamp = getCurrentTimestamp();
	mLastCleanupTimestamp = lastTimestamp;

	// Run the main loop
	while (true)
	{
		const uint64 currentTimestamp = getCurrentTimestamp();
		const uint64 millisecondsElapsed = currentTimestamp - lastTimestamp;
		lastTimestamp = currentTimestamp;

		// Check for new packets
		if (!updateReceivePackets(connectionManager))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		connectionManager.updateConnections(currentTimestamp);

		// Perform cleanup regularly
		if (currentTimestamp - mLastCleanupTimestamp > 5000)	// Every 5 seconds
		{
			performCleanup();
			mLastCleanupTimestamp = currentTimestamp;
		}
	}
}

NetConnection* Server::createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress)
{
	while (true)
	{
		const uint32 playerID = (uint32)(rand() & 0xff) + ((uint32)(rand() % 0xff) << 8) + ((uint32)(rand() % 0xff) << 16) + ((uint32)(rand() % 0xff) << 24);
		if (mNetConnectionsByPlayerID.count(playerID) == 0)
		{
			ServerNetConnection& connection = mNetConnectionPool.createObject(playerID);
			mNetConnectionsByPlayerID[playerID] = &connection;
			RMX_LOG_INFO("Created new connection with player ID " << playerID << " (now " << mNetConnectionsByPlayerID.size() << " total connections)");
			return &connection;
		}
	}
	return nullptr;
}

void Server::destroyNetConnection(NetConnection& connection)
{
	ServerNetConnection& serverNetConnection = static_cast<ServerNetConnection&>(connection);
	RMX_LOG_INFO("Removing connection with player ID " << serverNetConnection.getPlayerID() << " (now " << (mNetConnectionsByPlayerID.size() - 1) << " total connections)");

	serverNetConnection.unregisterPlayer();
	mNetConnectionsByPlayerID.erase(serverNetConnection.getPlayerID());
	mNetConnectionPool.destroyObject(serverNetConnection);
}

bool Server::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	// Go through sub-systems
	if (mChannels.onReceivedPacket(evaluation))
		return true;

	// Failed
	return false;
}

bool Server::onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::GetServerFeaturesRequest::Query::PACKET_TYPE:
		{
			// Re-use the already prepared request instance
			network::GetServerFeaturesRequest& request = mCachedServerFeaturesRequest;
			if (!evaluation.readQuery(request))
				return false;

			// Nothing more to change, the response is already filled in
			return evaluation.respond(request);
		}

		case network::AppUpdateCheckRequest::Query::PACKET_TYPE:
		{
			using Request = network::AppUpdateCheckRequest;
			Request request;
			if (!evaluation.readQuery(request))
				return false;

			// TODO: Properly implement this
			//  -> Maybe pass it on to a sub-system handling this kind of requests
			request.mResponse.mHasUpdate = false;
			if (request.mQuery.mAppName == "sonic3air")
			{
				uint32 latestVersion = 0;
				const char* updateURL = nullptr;
				if (request.mQuery.mPlatform == "windows" && request.mQuery.mReleaseChannel == "test")
				{
					// Test builds for Windows
					latestVersion = 0x22021300;
					updateURL = "https://github.com/Eukaryot/sonic3air/releases";
				}
				else
				{
					// Stable builds
					if (request.mQuery.mPlatform == "windows" || request.mQuery.mPlatform == "linux" || request.mQuery.mPlatform == "mac" ||
						request.mQuery.mPlatform == "android" || request.mQuery.mPlatform == "web")
					{
						if (request.mQuery.mReleaseChannel == "stable" || request.mQuery.mReleaseChannel == "preview" || request.mQuery.mReleaseChannel == "test")
						{
							latestVersion = 0x21092800;
						}
					}
					else if (request.mQuery.mPlatform == "switch")
					{
						if (request.mQuery.mReleaseChannel == "stable" || request.mQuery.mReleaseChannel == "preview" || request.mQuery.mReleaseChannel == "test")
						{
							latestVersion = 0x21091200;
						}
					}
				}

				if (latestVersion != 0 && request.mQuery.mInstalledAppVersion < latestVersion)
				{
					request.mResponse.mHasUpdate = true;
					request.mResponse.mAvailableAppVersion = latestVersion;
					request.mResponse.mAvailableContentVersion = latestVersion;
					request.mResponse.mUpdateInfoURL = (nullptr == updateURL ) ? "https://sonic3air.org" : updateURL;
				}
			}
			return evaluation.respond(request);
		}
	}

	// Go through sub-systems
	if (mChannels.onReceivedRequestQuery(evaluation))
		return true;

	// Failed
	return false;
}

void Server::performCleanup()
{
	// Check for disconnected and empty connection instances
	std::vector<NetConnection*> connectionsToRemove;
	for (auto& pair : mNetConnectionsByPlayerID)
	{
		if (pair.second->getState() == NetConnection::State::DISCONNECTED || pair.second->getState() == NetConnection::State::EMPTY)
		{
			connectionsToRemove.push_back(pair.second);
		}
	}
	for (NetConnection* connection : connectionsToRemove)
	{
		destroyNetConnection(*connection);
	}
}
