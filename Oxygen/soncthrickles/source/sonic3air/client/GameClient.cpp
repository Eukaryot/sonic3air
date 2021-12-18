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
	mUpdateCheck(mServerConnection)
{
}

GameClient::~GameClient()
{
	Sockets::shutdownSockets();
}

void GameClient::setupClient()
{
	const ConfigurationImpl& config = ConfigurationImpl::instance();
	if (config.mGameServer.mConnectToServer && !config.mGameServer.mServerURL.empty())
	{
		Sockets::startupSockets();

		// Setup socket & connection manager
		if (!mSocket.bindToAnyPort())
			RMX_ERROR("Socket bind to any port failed", return);

		// Start connection
		mServerConnection.startConnectTo(mConnectionManager, SocketAddress(config.mGameServer.mServerURL, (uint16)config.mGameServer.mServerPort));
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
					mUpdateCheck.evaluateServerFeaturesResponse(mGetServerFeaturesRequest);

					// Ready for actual online feature use
					mState = State::READY;
					break;
				}
			}
			break;
		}

		case State::READY:
		{
			// Regular update for the update check, so we stay updated on updates
			mUpdateCheck.performUpdate();
			break;
		}
	}
}
