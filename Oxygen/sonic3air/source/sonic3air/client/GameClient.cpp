/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/GameClient.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen_netcore/serverclient/ProtocolVersion.h"


namespace
{
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


GameClient::GameClient() :
#if defined(GAME_CLIENT_USE_UDP)
	mConnectionManager(&mUDPSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE),
#else
	mConnectionManager(nullptr, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE),
#endif
	mGhostSync(*this),
	mUpdateCheck(*this)
{
#if defined(DEBUG) && 0
	// Just for testing / debugging
	mConnectionManager.mDebugSettings.mSendingPacketLoss = 0.2f;
	mConnectionManager.mDebugSettings.mReceivingPacketLoss = 0.2f;
#endif
}

GameClient::~GameClient()
{
}

void GameClient::setupClient()
{
	const ConfigurationImpl::GameServer& config = ConfigurationImpl::instance().mGameServer;
	if (!config.mServerHostName.empty())
	{
		Sockets::startupSockets();

	#if defined(GAME_CLIENT_USE_UDP)
		// Setup socket & connection manager
		if (!mUDPSocket.bindToAnyPort())
			RMX_ERROR("Socket bind to any port failed", return);
	#endif

		mState = State::STARTED;
	}
}

void GameClient::updateClient(float timeElapsed)
{
	const uint64 currentTimestamp = getCurrentTimestamp();
	if (mServerConnection.getState() != NetConnection::State::EMPTY)
	{
		// Check for new packets
		updateReceivePackets(mConnectionManager);
		mConnectionManager.updateConnections(currentTimestamp);
	}

	if (mServerConnection.getState() != NetConnection::State::CONNECTED)
	{
		updateNotConnected(currentTimestamp);
	}
	else
	{
		updateConnected();
	}

	// Regular update for the sub-systems
	mGhostSync.performUpdate();
	mUpdateCheck.performUpdate();
}

void GameClient::connectToServer()
{
	if (mConnectionState == ConnectionState::NOT_CONNECTED || mConnectionState == ConnectionState::FAILED)
	{
		// Start connecting now
		startConnectingToServer(getCurrentTimestamp());
	}
}

bool GameClient::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	if (mGhostSync.onReceivedPacket(evaluation))
		return true;
	return false;
}

void GameClient::startConnectingToServer(uint64 currentTimestamp)
{
	SocketAddress serverAddress;
	{
		const ConfigurationImpl::GameServer& config = ConfigurationImpl::instance().mGameServer;
		std::string serverIP;
		if (!resolveGameServerHostName(config.mServerHostName, serverIP))
		{
			mConnectionState = ConnectionState::FAILED;
			return;
		}

	#if defined(GAME_CLIENT_USE_UDP)
		serverAddress.set(serverIP, (uint16)config.mServerPortUDP);
	#elif defined(GAME_CLIENT_USE_TCP)
		serverAddress.set(serverIP, (uint16)config.mServerPortTCP);
	#elif defined(GAME_CLIENT_USE_WSS)
		serverAddress.set(serverIP, (uint16)config.mServerPortWSS);
	#endif
	}

	mServerConnection.startConnectTo(mConnectionManager, serverAddress, currentTimestamp);
	mLastConnectionAttemptTimestamp = currentTimestamp;
	mConnectionState = ConnectionState::CONNECTING;
}

void GameClient::updateNotConnected(uint64 currentTimestamp)
{
	// TODO: This method certainly needs to handle more cases

	switch (mConnectionState)
	{
		case ConnectionState::CONNECTING:
		{
			// Wait until connected
			break;
		}

		case ConnectionState::ESTABLISHED:
		{
			// Connection was lost
			mConnectionState = ConnectionState::NOT_CONNECTED;
			break;
		}

		default:
			break;
	}

	if (mServerConnection.getState() == NetConnection::State::DISCONNECTED)
	{
		// Try connecting once again after 30 seconds
		if (currentTimestamp > mLastConnectionAttemptTimestamp + 30000)
		{
			startConnectingToServer(currentTimestamp);
		}
	}
}

void GameClient::updateConnected()
{
	// Currently connected to server
	mConnectionState = ConnectionState::ESTABLISHED;

	switch (mState)
	{
		case State::STARTED:
		{
			// First ask the server about its features
			switch (mGetServerFeaturesRequest.getState())
			{
				case highlevel::RequestBase::State::NONE:
				{
					// Send the request
					mServerConnection.sendRequest(mGetServerFeaturesRequest);
					break;
				}

				case highlevel::RequestBase::State::SUCCESS:
				{
					// Evaluate response
					mGhostSync.evaluateServerFeaturesResponse(mGetServerFeaturesRequest);
					mUpdateCheck.evaluateServerFeaturesResponse(mGetServerFeaturesRequest);

					// Ready for actual online feature use
					mState = State::READY;
					break;
				}

				default:
					break;
			}
			break;
		}

		case State::READY:
		{
			break;
		}

		default:
			break;
	}
}
