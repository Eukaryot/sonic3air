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


class TestClient : public ConnectionListenerInterface
{
public:
	void runClient();

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
	};

	State mState = State::NONE;
	NetConnection mConnection;
	network::GetExternalAddressRequest mGetExternalAddressRequest;
	network::JoinChannelRequest mJoinChannelRequest;
};


void TestClient::runClient()
{
	// Switch between UDP and TCP usage
#if 1
	UDPSocket udpSocket;
	if (!udpSocket.bindToAnyPort(USE_IPV6))
		RMX_ERROR("Socket bind to any port failed", return);
	ConnectionManager connectionManager(&udpSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE);
#else
	ConnectionManager connectionManager(nullptr, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE);
#endif

#ifdef DEBUG
	setupDebugSettings(connectionManager.mDebugSettings);
#endif

	// Connect to server
	SocketAddress serverAddress;
	{
		std::string serverIP;
		if (!Sockets::resolveToIP(SERVER_NAME, serverIP))
			RMX_ERROR("Unable to resolve server name " << SERVER_NAME, return);
		serverAddress.set(serverIP, connectionManager.hasUDPSocket() ? UDP_SERVER_PORT : TCP_SERVER_PORT);
	}
	if (!mConnection.startConnectTo(connectionManager, serverAddress))
		RMX_ERROR("Starting a connection failed", return);

	mState = State::WAITING_FOR_CONNECTION;

	uint64 lastTimestamp = ConnectionManager::getCurrentTimestamp();
	uint64 lastMessageTimestamp = 0;
	while (true)
	{
		if (mConnection.getState() == NetConnection::State::EMPTY)
			break;

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
	}
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
	{
		TestClient client;
		client.runClient();
	}
	Sockets::shutdownSockets();
	return 0;
}
