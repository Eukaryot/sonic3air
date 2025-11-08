/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#define RMX_LIB

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/RequestBase.h"
#include "oxygen_netcore/serverclient/Packets.h"
#include "oxygen_netcore/serverclient/ProtocolVersion.h"

#include "PrivatePackets.h"
#include "Shared.h"

#include <thread>


static const bool CLIENT_USE_IPv6 = false;
static const bool CLIENT_USE_UDP  = true;

#if 1
	static const std::string SERVER_NAME = "gameserver.sonic3air.org";
#else
	static const std::string SERVER_NAME = (SERVER_PROTOCOL_FAMILY >= Sockets::ProtocolFamily::IPv6) ? "::1" : "127.0.0.1";
#endif


class TestClient : public ConnectionListenerInterface
{
public:
	enum class RunMode
	{
		CHECK_SERVER,	// Check if game server is running and responds to packets
		TESTING_STUFF	// Just for development, like testing new features
	};

public:
	bool runClient(RunMode runMode);

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override
	{
		// Do not allow incoming connections
		return nullptr;
	}

	virtual void destroyNetConnection(NetConnection& connection) override
	{
		RMX_ASSERT(false, "This should never get called");
	}

	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override
	{
		switch (evaluation.mPacketType)
		{
			case network::ChannelMessagePacket::PACKET_TYPE:
			{
				network::ChannelMessagePacket packet;
				if (!evaluation.readPacket(packet))
					return false;

				std::string msg;
				msg.resize(packet.mMessage.size() + 1);
				for (size_t k = 0; k < packet.mMessage.size(); ++k)
					msg[k] = (char)packet.mMessage[k];
				msg.back() = 0;

				RMX_LOG_INFO("Received message: \"" << msg << "\" from player " << rmx::hexString(packet.mSendingPlayerID));
				return true;
			}
		}
		return false;
	}

private:
	void updateClient();

private:
	enum class State
	{
		NONE,
		WAITING_FOR_CONNECTION,
		EXTERNAL_ADDRESS_REQUEST_SENT,
		JOIN_REQUEST_SENT,
		READY_TO_SEND_MESSAGE,
		MESSAGE_SENT,
		DONE_SUCCESS,
		DONE_FAILURE
	};

private:
	RunMode mRunMode = RunMode::CHECK_SERVER;
	State mState = State::NONE;
	NetConnection mConnection;
	network::GetExternalAddressRequest mGetExternalAddressRequest;
	network::JoinChannelRequest mJoinChannelRequest;
};


bool TestClient::runClient(RunMode runMode)
{
	mRunMode = runMode;

	// Setup for either UDP or TCP usage
	UDPSocket udpSocket;
	UDPSocket* udpSocketToUse = nullptr;
	if (CLIENT_USE_UDP)
	{
		if (!udpSocket.bindToAnyPort(CLIENT_USE_IPv6 ? Sockets::ProtocolFamily::IPv6 : Sockets::ProtocolFamily::IPv4))
			RMX_ERROR("Socket bind to any port failed", return false);
		udpSocketToUse = &udpSocket;
	}

	ConnectionManager connectionManager(udpSocketToUse, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE);
#ifdef DEBUG
	setupDebugSettings(connectionManager.mDebugSettings);
#endif

	// Connect to server
	SocketAddress serverAddress;
	{
		std::string serverIP;
		if (!Sockets::resolveToIP(SERVER_NAME, serverIP, CLIENT_USE_IPv6))
			RMX_ERROR("Unable to resolve server name " << SERVER_NAME, return false);
		serverAddress.set(serverIP, connectionManager.hasUDPSocket() ? UDP_SERVER_PORT : TCP_SERVER_PORT);
	}
	if (!mConnection.startConnectTo(connectionManager, serverAddress))
		RMX_ERROR("Starting a connection failed", return false);

	mState = State::WAITING_FOR_CONNECTION;

	uint64 lastTimestamp = ConnectionManager::getCurrentTimestamp();
	uint64 lastMessageTimestamp = 0;
	while (true)
	{
		if (mConnection.getState() == NetConnection::State::EMPTY || mConnection.getState() == NetConnection::State::DISCONNECTED)
			return false;

		const uint64 currentTimestamp = ConnectionManager::getCurrentTimestamp();
		const uint64 millisecondsElapsed = currentTimestamp - lastTimestamp;
		lastTimestamp = currentTimestamp;

		// Check for new packets
		if (!connectionManager.updateConnectionManager())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		// Update client state
		updateClient();

		// Stop on success or failure
		if (mState >= State::DONE_SUCCESS)
			break;
	}

	return (mState == State::DONE_SUCCESS);
}

void TestClient::updateClient()
{
	switch (mState)
	{
		case State::WAITING_FOR_CONNECTION:
		{
			if (mConnection.getState() == NetConnection::State::CONNECTED)
			{
				// Send request to get own external address
				mConnection.sendRequest(mGetExternalAddressRequest);
				mState = State::EXTERNAL_ADDRESS_REQUEST_SENT;
			}
			break;
		}

		case State::EXTERNAL_ADDRESS_REQUEST_SENT:
		{
			if (mGetExternalAddressRequest.hasResponse())
			{
				RMX_LOG_INFO("Received own external address: IP = " << mGetExternalAddressRequest.mResponse.mIP << ", Port = " << mGetExternalAddressRequest.mResponse.mPort);

				if (mRunMode == RunMode::CHECK_SERVER)
				{
					mState = State::DONE_SUCCESS;
					break;
				}

				// Send request to join a channel
				mJoinChannelRequest.mQuery.mChannelName = "test-channel";
				mJoinChannelRequest.mQuery.mChannelHash = (uint32)rmx::getMurmur2_64(mJoinChannelRequest.mQuery.mChannelName);
				mConnection.sendRequest(mJoinChannelRequest);
				mState = State::JOIN_REQUEST_SENT;
			}
			break;
		}

		case State::JOIN_REQUEST_SENT:
		{
			if (mJoinChannelRequest.hasResponse())
			{
				mState = State::READY_TO_SEND_MESSAGE;
			}
			break;
		}

		case State::READY_TO_SEND_MESSAGE:
		{
			network::BroadcastChannelMessagePacket packet;
			packet.mChannelHash = (uint32)rmx::getMurmur2_64(std::string("test-channel"));
			const std::string msg = "Message_" + rmx::hexString(rand(), "");
			packet.mMessage.resize(msg.length());
			for (size_t k = 0; k < msg.length(); ++k)
				packet.mMessage[k] = (uint8)msg[k];

			mConnection.sendPacket(packet);
			RMX_LOG_INFO("Sent message: \"" << msg << "\"");

			mState = State::MESSAGE_SENT;
			break;
		}

		case State::MESSAGE_SENT:
		{
			// TODO: Send another message after a while?
			break;
		}
	}
}


int main(int argc, char** argv)
{
	randomize();
	rmx::Logging::addLogger(*new rmx::StdCoutLogger());
	Sockets::startupSockets();

	bool success = false;
	{
		TestClient client;
		success = client.runClient(TestClient::RunMode::CHECK_SERVER);
	}

	Sockets::shutdownSockets();
	const char* message = (success ? "Server check successful" : "Server check failed!");
	RMX_LOG_INFO(message);
	RMX_CHECK(success, "Server check failed!", );
	return success ? 0 : 1;
}
