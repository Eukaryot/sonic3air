/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/network/netplay/NetplayClient.h"
#include "oxygen/network/netplay/NetplayManager.h"
#include "oxygen/network/netplay/NetplayPackets.h"
#include "oxygen/network/EngineServerClient.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/simulation/SimulationState.h"

#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"
#include "oxygen_netcore/serverclient/Packets.h"


NetplayClient::NetplayClient(ConnectionManager& connectionManager, NetplayManager& netplayManager) :
	mConnectionManager(connectionManager),
	mNetplayManager(netplayManager)
{
}

NetplayClient::~NetplayClient()
{
	// TODO: Call this only if netplay game was actually active
	EngineMain::getDelegate().onStopNetplayGame(false);

	// TODO: This just destroys the connection, but doesn't tell the host about it
	mHostConnection.clear();
}

void NetplayClient::joinViaServer()
{
	// Request game server connection
	EngineServerClient::instance().connectToServer();
	mState = State::CONNECT_TO_SERVER;
}

void NetplayClient::connectDirectlyToHost(std::string_view ip, uint16 port)
{
	mHostConnection.startConnectTo(mConnectionManager, SocketAddress(ip, port));
	mState = State::CONNECT_TO_HOST;
}

void NetplayClient::updateConnection(float deltaSeconds)
{
	EngineServerClient& engineServerClient = EngineServerClient::instance();

	switch (mState)
	{
		case State::CONNECT_TO_SERVER:
		{
			// Keep requesting game server connection
			engineServerClient.connectToServer();

			// Wait until the server connection is established, server features were queried, and the game socket's external address was retrieved
			if (!engineServerClient.hasReceivedServerFeatures() || mNetplayManager.getExternalAddressQuery().mOwnExternalIP.empty())
				break;

			// TODO: Why not check the server features?

			// Register at game server
			mRegistrationRequest = network::RegisterForNetplayRequest();
			mRegistrationRequest.mQuery.mIsHost = false;
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

		case State::CONNECT_TO_HOST:
		{
			if (mHostConnection.getState() == NetConnection::State::CONNECTED)
			{
				mState = State::CONNECTED;
			}
			break;
		}

		default:
			break;
	}
}

bool NetplayClient::onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::ConnectToNetplayPacket::PACKET_TYPE:
		{
			network::ConnectToNetplayPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			// Connect to the given address
			mHostConnection.startConnectTo(mConnectionManager, SocketAddress(packet.mConnectToIP, packet.mConnectToPort));
			mState = State::CONNECT_TO_HOST;
			return true;
		}
	}

	return false;
}

bool NetplayClient::canBeginNextFrame(uint32 frameNumber)
{
	// Don't proceed beyond the latest received frame number
	if (frameNumber >= mNextFrameNumber)
		return false;

	return true;
}

void NetplayClient::onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber)
{
	if (mHostConnection.getState() != NetConnection::State::CONNECTED)
		return;

	// Get current input state and send it to the host
	{
		PlayerInputIncrementPacket packet;
		packet.mFrameNumber = frameNumber;
		packet.mNumFrames = 1;
		packet.mInputs.push_back(controlsIn.getInputFromController(0));

		mHostConnection.sendPacket(packet, NetConnection::SendFlags::UNRELIABLE);
	}

	// Inject input from what we received from host
	if (!mReceivedFrames.empty())
	{
		const int indexFromBack = mNextFrameNumber - frameNumber - 1;
		if (indexFromBack >= 0 && indexFromBack < (int)mReceivedFrames.size())
		{
			auto it = mReceivedFrames.rbegin();
			for (int k = 0; k < indexFromBack; ++k)
				++it;

			controlsIn.injectInputs(it->mInputsByPlayer, MAX_PLAYERS);
		}
	}
}

bool NetplayClient::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case StartGamePacket::PACKET_TYPE:
		{
			StartGamePacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			startGame(packet);
			return true;
		}

		case GameStateIncrementPacket::PACKET_TYPE:
		{
			GameStateIncrementPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			if (packet.mFrameNumber < mNextFrameNumber)
				return true;

			// Evaluate the packet and store its input history for all players
			const size_t currentFramesGap = packet.mFrameNumber - mNextFrameNumber + 1;
			const size_t numFramesToCopy = std::min<size_t>(currentFramesGap, packet.mNumFrames);
			if (numFramesToCopy < currentFramesGap)
			{
				// Throw away old frames, to not create an actual gap between frames in the queue
				RMX_ERROR("Warning: Complete game state desync!", );
				mReceivedFrames.clear();
			}

			for (int k = 0; k < (int)numFramesToCopy; ++k)
			{
				mReceivedFrames.emplace_back();
				ReceivedFrame& newFrame = mReceivedFrames.back();
				const uint16* input = &packet.mInputs[packet.mNumPlayers * (packet.mNumFrames - numFramesToCopy + k)];
				for (int playerIndex = 0; playerIndex < std::min<int>(packet.mNumPlayers, MAX_PLAYERS); ++playerIndex)
				{
					newFrame.mInputsByPlayer[playerIndex] = *input;
					++input;
				}
				// Player inputs not included in the packet stay at 0

				// Checksum for debugging
				mInputChecksum = rmx::addToFNV1a_32(mInputChecksum, reinterpret_cast<uint8*>(newFrame.mInputsByPlayer), sizeof(newFrame.mInputsByPlayer));
				const int frameNumber = packet.mFrameNumber - (int)numFramesToCopy + k + 1;
				if (frameNumber % 200 == 0)
				{
					mRegularInputChecksum = mInputChecksum;
					mRegularChecksumFrameNumber = frameNumber;
				}
			}

			mNextFrameNumber = packet.mFrameNumber + 1;

			// Update latency
			mCurrentLatency = packet.mFrameNumber - packet.mLastClientReceivedFrame;

			return true;
		}
	}

	return false;
}

bool NetplayClient::onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation)
{
	switch (evaluation.mLowLevelSignature)
	{
		case network::PunchthroughConnectionlessPacket::SIGNATURE:
		{
			network::PunchthroughConnectionlessPacket packet;
			if (!packet.serializePacket(evaluation.mSerializer, 1))
				return false;

			mReceivedPunchthroughPacketSender = evaluation.mSenderAddress;
			return true;
		}
	}

	return false;
}

uint32 NetplayClient::getRegularInputChecksum(int& outFrameNumber) const
{
	outFrameNumber = mRegularChecksumFrameNumber;
	return mRegularInputChecksum;
}

void NetplayClient::startGame(const StartGamePacket& packet)
{
	// Read RNG state from packet
	uint64* rngState = Application::instance().getSimulation().getSimulationState().getRandomNumberGenerator().accessState();
	for (int k = 0; k < 4; ++k)
		rngState[k] = packet.mRNGState[k];

	// Load game settings
	VectorBinarySerializer serializer(true, packet.mSerializedGameSettings);
	EngineMain::getDelegate().serializeGameSettings(serializer);

	// Setup input checksum tracking
	mInputChecksum = rmx::startFNV1a_32();
	mRegularInputChecksum = mInputChecksum;
	mRegularChecksumFrameNumber = 0;

	mState = State::GAME_RUNNING;

	EngineMain::getDelegate().onStartNetplayGame(false);
}
