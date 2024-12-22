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
#include "oxygen/network/EngineServerClient.h"
#include "oxygen/application/input/ControlsIn.h"


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
	mHostState = HostState::CONNECT_TO_SERVER;
}

NetConnection* NetplayHost::createNetConnection(const SocketAddress& senderAddress)
{
	// Remove from connecting players
	for (size_t k = 0; k < mConnectingPlayers.size(); ++k)
	{
		const ConnectingPlayer& player = mConnectingPlayers[k];
		if (player.mRemoteAddress == senderAddress)
		{
			mConnectingPlayers.erase(mConnectingPlayers.begin() + k);
			break;
		}
	}

	// Create new connection
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

	switch (mHostState)
	{
		case HostState::NONE:
			break;

		case HostState::CONNECT_TO_SERVER:
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

			mHostState = HostState::REGISTERED;
			break;
		}

		case HostState::REGISTERED:
		{
			// Check registration
			if (mRegistrationRequest.hasResponse())
			{
				if (!mRegistrationRequest.mResponse.mSuccess)
				{
					mHostState = HostState::FAILED;
					break;
				}
			}

		#if 1
			// Switch to running as soon as the first client connects directly
			if (!mPlayerConnections.empty() && mPlayerConnections[0]->getState() == NetConnection::State::CONNECTED)
			{
				startGame();
			}
		#endif
			break;
		}

		default:
			break;
	}

	// Update connecting players
	for (size_t k = 0; k < mConnectingPlayers.size(); ++k)
	{
		ConnectingPlayer& player = mConnectingPlayers[k];
		player.mSendTimer += deltaSeconds;
		if (player.mSendTimer < 0.1f)
			continue;

		if (player.mSentPackets < 20)
		{
			sendPunchthroughPacket(player);
		}
		else
		{
			mConnectingPlayers.erase(mConnectingPlayers.begin() + k);
			--k;
		}
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
					ConnectingPlayer& player = vectorAdd(mConnectingPlayers);
					player.mRemoteAddress = SocketAddress(packet.mConnectToIP, packet.mConnectToPort);
					player.mSessionID = packet.mSessionID;

					sendPunchthroughPacket(player);
					break;
				}

                default:
                    break;
			}

			return true;
		}
	}

	return false;
}

void NetplayHost::startGame()
{
	for (PlayerConnection* connection : mPlayerConnections)
	{
		StartGamePacket packet;
		// TODO: Fill this in, and the other values as well
		//packet.mGameBuildVersion = BUILD_NUMBER;

		connection->sendPacket(packet, NetConnection::SendFlags::NONE, &connection->mStartGamePacketID);
	}

	EngineMain::getDelegate().onStartNetplayGame();

	mHostState = HostState::GAME_RUNNING;
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

void NetplayHost::sendPunchthroughPacket(ConnectingPlayer& player)
{
	network::PunchthroughConnectionlessPacket punchthroughPacket;
	punchthroughPacket.mQueryID = (uint32)player.mSessionID;
	punchthroughPacket.mSenderReceivedPackets = false;

	mConnectionManager.sendConnectionlessLowLevelPacket(punchthroughPacket, player.mRemoteAddress, 0, 0);

	++player.mSentPackets;
	player.mSendTimer = 0.0f;
}
