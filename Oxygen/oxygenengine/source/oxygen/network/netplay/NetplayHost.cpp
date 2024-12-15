/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/network/netplay/NetplayHost.h"
#include "oxygen/network/netplay/NetplayManager.h"
#include "oxygen/application/input/ControlsIn.h"

#include "oxygen/network/EngineServerClient.h"


NetplayHost::NetplayHost(ConnectionManager& connectionManager, NetplayManager& netplayManager) :
	mConnectionManager(connectionManager),
	mNetplayManager(netplayManager)
{
}

NetplayHost::~NetplayHost()
{
	// Destroy all connections to clients
	for (size_t k = 0; k < mPlayerConnections.size(); ++k)
	{
		// TODO: This just destroys the connection, but doesn't tell the client about it
		mPlayerConnections[k]->clear();
		delete mPlayerConnections[k];
	}
	mPlayerConnections.clear();
}

void NetplayHost::registerAtServer()
{
	// Request game server connection
	EngineServerClient::instance().connectToServer();
	mState = State::CONNECT_TO_SERVER;
}

NetConnection* NetplayHost::createNetConnection(const SocketAddress& senderAddress)
{
	PlayerConnection* connection = new PlayerConnection();

	// TODO: Set player index accordingly, it should take into account the other connections' player indices and use a free one
	connection->mPlayerIndex = (uint8)mPlayerConnections.size() + 1;

	mPlayerConnections.push_back(connection);
	return connection;
}

void NetplayHost::destroyNetConnection(NetConnection& connection)
{
	vectorRemoveAll(mPlayerConnections, &connection);
}

void NetplayHost::updateConnection(float deltaSeconds)
{
	EngineServerClient& engineServerClient = EngineServerClient::instance();

	switch (mState)
	{
		case State::NONE:
		{
			// Switch to running as soon as the first client connects directly
			if (!mPlayerConnections.empty())
			{
				mState = State::RUNNING;
			}
			break;
		}

		case State::CONNECT_TO_SERVER:
		{
			// Request game server connection
			engineServerClient.connectToServer();

			// Wait until the server connection is established, server features were queried, and the game socket's external address was retrieved
			if (!engineServerClient.hasReceivedServerFeatures() || mNetplayManager.getExternalAddressQuery().mOwnExternalIP.empty())
				break;

			// TODO: Why not check the server features?

			// Register at game server
			mRegistrationRequest = network::RegisterForNetplayRequest();
			mRegistrationRequest.mQuery.mIsHost = true;
			mRegistrationRequest.mQuery.mSessionID = 0x12345;	// TODO: This is just for testing and should be replaced by a random ID
			mRegistrationRequest.mQuery.mGameSocketExternalIP = mNetplayManager.getExternalAddressQuery().mOwnExternalIP;
			mRegistrationRequest.mQuery.mGameSocketExternalPort = mNetplayManager.getExternalAddressQuery().mOwnExternalPort;
			engineServerClient.getServerConnection().sendRequest(mRegistrationRequest);

			mState = State::REGISTERED;
			break;
		}

		case State::REGISTERED:
		{
			// Check registration
			if (mRegistrationRequest.hasResponse())
			{
				if (!mRegistrationRequest.mResponse.mSuccess)
				{
					mState = State::FAILED;
					break;
				}
			}
			break;
		}

		case State::PUNCHTHROUGH:
		{
			// TODO: Regularly send a new "PunchthroughConnectionlessPacket", until receiving an incoming connection, or timeout

			// TODO: Support multiple clients here... we probably have to track a state for each client individually

			if (!mPlayerConnections.empty())
			{
				mState = State::RUNNING;
			}
			break;
		}

		default:
			break;
	}
}

bool NetplayHost::onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::ConnectToNetplayPacket::PACKET_TYPE:
		{
			network::ConnectToNetplayPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			switch (packet.mConnectionType)
			{
				case network::NetplayConnectionType::PUNCHTHROUGH:
				{
					// Send some packets towards the client, until receiving a response
					// TODO: This is just a single one... :/
					network::PunchthroughConnectionlessPacket punchthroughPacket;
					punchthroughPacket.mQueryID = (uint32)packet.mSessionID;
					punchthroughPacket.mSenderReceivedPackets = false;

					mConnectionManager.sendConnectionlessLowLevelPacket(punchthroughPacket, SocketAddress(packet.mConnectToIP, packet.mConnectToPort), 0, 0);
					mState = State::PUNCHTHROUGH;
					break;
				}
			}

			return true;
		}
	}

	return false;
}

void NetplayHost::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	std::vector<PlayerConnection*> activeConnections;
	{
		// First filter out connections that became invalid
		std::vector<PlayerConnection*> connectionsToRemove;
		for (PlayerConnection* playerConnection : mPlayerConnections)
		{
			if (playerConnection->getState() == NetConnection::State::CONNECTED)
			{
				activeConnections.push_back(playerConnection);
			}
			else if (playerConnection->getState() == NetConnection::State::DISCONNECTED || playerConnection->getState() == NetConnection::State::EMPTY)
			{
				connectionsToRemove.push_back(playerConnection);
			}
		}
		for (PlayerConnection* connection : connectionsToRemove)
		{
			destroyNetConnection(*connection);
		}
	}

	if (activeConnections.empty())
		return;

	// Add new frame to the history
	{
		mInputHistory.emplace_back();
		InputFrame& newInputFrame = mInputHistory.back();

		// Apply host's local input (local player 1 only)
		newInputFrame.mInputsByPlayer[0] = controlsIn.getInputFromController(0);

		// Apply last received input from the clients
		for (PlayerConnection* playerConnection : activeConnections)
		{
			if (playerConnection->mPlayerIndex >= 0 && playerConnection->mPlayerIndex < 4)
			{
				const uint16 input = playerConnection->mLastReceivedInput;
				newInputFrame.mInputsByPlayer[playerConnection->mPlayerIndex] = input;
			}
		}

		// Limit inputs history to a reasonable number of frames
		while (mInputHistory.size() > 30)
			mInputHistory.pop_front();
	}

	// Apply input locally
	{
		const InputFrame& inputFrame = mInputHistory.back();
		for (int playerIndex = 0; playerIndex < 2; ++playerIndex)
		{
			controlsIn.injectInput(playerIndex, inputFrame.mInputsByPlayer[playerIndex]);
		}
	}

	const size_t numPlayers = activeConnections.size() + 1;
	const size_t numFrames = std::min<size_t>(mInputHistory.size(), 30);

	// Build packet to send to the clients
	GameStateIncrementPacket packet;
	packet.mNumPlayers = (uint8)numPlayers;
	packet.mNumFrames = (uint8)numFrames;
	packet.mFrameNumber = frameNumber;
	packet.mInputs.resize(packet.mNumPlayers * packet.mNumFrames);

	// Copy input data from players
	{
		uint16* input = &packet.mInputs[0];
		for (auto it = mInputHistory.rbegin(); it != mInputHistory.rend(); ++it)
		{
			for (int playerIndex = 0; playerIndex < (int)numPlayers; ++playerIndex)
			{
				*input = it->mInputsByPlayer[playerIndex];
				++input;
			}
		}
	}

	for (PlayerConnection* playerConnection : activeConnections)
	{
		playerConnection->sendPacket(packet);
	}
}

bool NetplayHost::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case PlayerInputIncrementPacket::PACKET_TYPE:
		{
			PlayerInputIncrementPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			if (!packet.mInputs.empty())
			{
				PlayerConnection& connection = static_cast<PlayerConnection&>(evaluation.mConnection);

				if (packet.mFrameNumber > connection.mLastReceivedFrameNumber)		// Ignore out-dated packets
				{
					connection.mLastReceivedFrameNumber = packet.mFrameNumber;
					connection.mLastReceivedInput = packet.mInputs.back();
				}
			}

			return true;
		}
	}

	return false;
}
