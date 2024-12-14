/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/client/EngineServerClient.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"

#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"
#include "oxygen_netcore/serverclient/ProtocolVersion.h"


EngineServerClient::SocketUsage EngineServerClient::getSocketUsage()
{
#if defined(PLATFORM_WEB)

	// Emscripten does not support UDP, so we need to use WebSockets
	return SocketUsage::WEBSOCKETS;

#else

	// UDP is the default, but TCP can be selected for testing; this should work for all platforms that support UDP as well
	//return SocketUsage::TCP;
	return SocketUsage::UDP;

#endif
}

bool EngineServerClient::resolveGameServerHostName(const std::string& hostName, std::string& outServerIP)
{
	if (getSocketUsage() != SocketUsage::WEBSOCKETS)
	{
		// For UDP/TCP, try the "gameserver" subdomain first
		//  -> This does not work for emscripten websockets, see implementation of "resolveToIp"
		//  -> It allows for having different servers for UDP/TCP, and websockets
		if (Sockets::resolveToIP("gameserver." + hostName, outServerIP))
			return true;
	}

	// Use the host name itself
	return Sockets::resolveToIP(hostName, outServerIP);
}


EngineServerClient::EngineServerClient() :
	mConnectionManager((getSocketUsage() == SocketUsage::UDP) ? &mUDPSocket : nullptr, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE)
{
#if defined(DEBUG) && 0
	// Just for testing / debugging
	mConnectionManager.mDebugSettings.mSendingPacketLoss = 0.2f;
	mConnectionManager.mDebugSettings.mReceivingPacketLoss = 0.2f;
#endif
}

EngineServerClient::~EngineServerClient()
{
}

bool EngineServerClient::setupClient()
{
	const Configuration::GameServerBase& config = Configuration::instance().mGameServerBase;
	if (config.mServerHostName.empty())
		return false;

	Sockets::startupSockets();

	if (getSocketUsage() == SocketUsage::UDP)
	{
		// Setup socket & connection manager
		if (!mUDPSocket.bindToAnyPort())
			RMX_ERROR("Socket bind to any port failed", return false);
	}

	mConnectionState = ConnectionState::NOT_CONNECTED;
	return true;
}

void EngineServerClient::updateClient(float timeElapsed)
{
	if (mServerConnection.getState() == NetConnection::State::EMPTY)
		return;

	if (mServerConnection.getState() == NetConnection::State::DISCONNECTED)
	{
		// Try connecting once again after 30 seconds
		mConnectionState = ConnectionState::FAILED;
		if (ConnectionManager::getCurrentTimestamp() > mLastConnectionAttemptTimestamp + 30000)
		{
			startConnectingToServer();
		}
	}
	else
	{
		// Check for new packets
		mConnectionManager.updateConnectionManager();
	}

	switch (mConnectionState)
	{
		case ConnectionState::CONNECTING:
		{
			// Wait until connected
			if (mServerConnection.getState() == NetConnection::State::CONNECTED)
			{
				mServerConnection.sendRequest(mGetServerFeaturesRequest);
				mConnectionState = ConnectionState::WAIT_FOR_FEATURES;
			}
			break;
		}

		case ConnectionState::WAIT_FOR_FEATURES:
		{
			switch (mGetServerFeaturesRequest.getState())
			{
				case highlevel::RequestBase::State::SUCCESS:
				{
					// Ready for actual online feature use
					mConnectionState = ConnectionState::READY;
					break;
				}

				case highlevel::RequestBase::State::FAILED:
				{
					// That failed...
					mServerConnection.clear();
					mConnectionState = ConnectionState::FAILED;
					break;
				}

				default:
					break;
			}
			break;
		}

		default:
			break;
	}
}

void EngineServerClient::connectToServer()
{
	if (mConnectionState == ConnectionState::NOT_CONNECTED || mConnectionState == ConnectionState::FAILED)
	{
		// Start connecting now
		startConnectingToServer();
	}
}

const network::GetServerFeaturesRequest::Response& EngineServerClient::getServerFeatures() const
{
	return mGetServerFeaturesRequest.mResponse;
}

bool EngineServerClient::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::ConnectToNetplayPacket::PACKET_TYPE:
		{
			// Pass on to netplay
			return GameplayConnector::instance().onReceivedGameServerPacket(evaluation);
		}
	}

	// Pass to listener
	if (nullptr == mListener)
		return false;
	return mListener->onReceivedPacket(evaluation);
}

void EngineServerClient::startConnectingToServer()
{
	SocketAddress serverAddress;
	{
		const Configuration::GameServerBase& config = Configuration::instance().mGameServerBase;
		std::string serverIP;
		if (!resolveGameServerHostName(config.mServerHostName, serverIP))
		{
			mConnectionState = ConnectionState::FAILED;
			return;
		}

		switch (getSocketUsage())
		{
			case SocketUsage::UDP:			serverAddress.set(serverIP, (uint16)config.mServerPortUDP);  break;
			case SocketUsage::TCP:			serverAddress.set(serverIP, (uint16)config.mServerPortTCP);  break;
			case SocketUsage::WEBSOCKETS:	serverAddress.set(serverIP, (uint16)config.mServerPortWSS);  break;
		}
	}

	mServerConnection.startConnectTo(mConnectionManager, serverAddress);
	mLastConnectionAttemptTimestamp = ConnectionManager::getCurrentTimestamp();
	mConnectionState = ConnectionState::CONNECTING;
}
