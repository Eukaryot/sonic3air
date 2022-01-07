/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/GameClient.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen_netcore/serverclient/ProtocolVersion.h"


GameClient::GameClient() :
	mConnectionManager(mSocket, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE),
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
	Sockets::shutdownSockets();
}

void GameClient::setupClient()
{
	const ConfigurationImpl::GameServer& config = ConfigurationImpl::instance().mGameServer;
	if (config.mConnectToServer && !config.mServerHostName.empty())
	{
		Sockets::startupSockets();

		// Setup socket & connection manager
		if (!mSocket.bindToAnyPort())
			RMX_ERROR("Socket bind to any port failed", return);

		// Start connection
		startConnectingToServer(getCurrentTimestamp());
		mState = State::STARTED;
	}
}

void GameClient::updateClient(float timeElapsed)
{
	if (mServerConnection.getState() == NetConnection::State::EMPTY)
		return;

	// Check for new packets
	updateReceivePackets(mConnectionManager);

	const uint64 currentTimestamp = getCurrentTimestamp();
	mConnectionManager.updateConnections(currentTimestamp);

	if (mServerConnection.getState() != NetConnection::State::CONNECTED)
	{
		if (mServerConnection.getState() == NetConnection::State::DISCONNECTED)
		{
			// Try connecting once again after a minute
			if (currentTimestamp > mLastConnectionAttemptTimestamp + 60000)
			{
				startConnectingToServer(currentTimestamp);
			}
		}
		return;
	}

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
			// Regular update for the update check, so we stay updated on updates
			mGhostSync.performUpdate();
			mUpdateCheck.performUpdate();
			break;
		}

		default:
			break;
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
		if (!Sockets::resolveToIP(config.mServerHostName, serverIP))
			RMX_ERROR("Unable to resolve server URL " << config.mServerHostName, return);
		serverAddress.set(serverIP, (uint16)config.mServerPort);
	}
	mServerConnection.startConnectTo(mConnectionManager, serverAddress, currentTimestamp);
	mLastConnectionAttemptTimestamp = currentTimestamp;
}
