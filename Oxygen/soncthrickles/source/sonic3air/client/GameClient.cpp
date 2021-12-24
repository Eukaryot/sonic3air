/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/GameClient.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen_netcore/serverclient/ProtocolVersion.h"


GameClient::GameClient() :
	mConnectionManager(mSocket, *this, network::HIGHLEVEL_MINIMUM_PROTOCOL_VERSION, network::HIGHLEVEL_MAXIMUM_PROTOCOL_VERSION),
	mGhostSync(mServerConnection),
	mUpdateCheck(mServerConnection)
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
	const ConfigurationImpl& config = ConfigurationImpl::instance();
	if (config.mGameServer.mConnectToServer && !config.mGameServer.mServerHostName.empty())
	{
		Sockets::startupSockets();

		// Setup socket & connection manager
		if (!mSocket.bindToAnyPort())
			RMX_ERROR("Socket bind to any port failed", return);

		// Start connection
		SocketAddress serverAddress;
		{
			std::string serverIP;
			if (!Sockets::resolveToIP(config.mGameServer.mServerHostName, serverIP))
				RMX_ERROR("Unable to resolve server URL " << config.mGameServer.mServerHostName, return);
			serverAddress.set(serverIP, (uint16)config.mGameServer.mServerPort);
		}
		mServerConnection.startConnectTo(mConnectionManager, serverAddress, getCurrentTimestamp());
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
		return;

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
