/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/LagStopwatch.h"
#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/internal/WebSocketWrapper.h"


namespace
{
	static const constexpr VersionRange<uint8> LOWLEVEL_PROTOCOL_VERSION_RANGE { 1, 1 };
	static const constexpr size_t MAX_NUM_ACTIVE_CONNECTIONS = 1024;	// Not a hard limit, but should be good enough for now

	struct ProtocolVersionChecker
	{
		enum class Result
		{
			SUCCESS,		// Found a protocol version both can support
			ERROR_TOO_OLD,	// Remote protocol version is lower than anything we can support
			ERROR_TOO_NEW	// Remote protocol version is higher than anything we can support
		};

		static Result chooseVersion(uint8& outVersion, VersionRange<uint8> localVersionRange, VersionRange<uint8> remoteVersionRange)
		{
			// Use the highest version both can support
			outVersion = std::min(remoteVersionRange.mMaximum, localVersionRange.mMaximum);
			if (outVersion < localVersionRange.mMinimum)
			{
				return Result::ERROR_TOO_OLD;
			}
			else if (outVersion < remoteVersionRange.mMinimum)
			{
				return Result::ERROR_TOO_NEW;
			}
			else
			{
				return Result::SUCCESS;
			}
		}
	};
}


uint64 ConnectionManager::getCurrentTimestamp()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

ConnectionManager::ConnectionManager(UDPSocket* udpSocket, TCPSocket* tcpListenSocket, ConnectionListenerInterface& listener, VersionRange<uint8> highLevelProtocolVersionRange) :
	mUDPSocket(udpSocket),
	mTCPListenSocket(tcpListenSocket),
	mListener(listener),
	mHighLevelProtocolVersionRange(highLevelProtocolVersionRange)
{
	mActiveConnections.reserve(16);
}

ConnectionManager::~ConnectionManager()
{
	terminateAllConnections();
}

bool ConnectionManager::updateConnectionManager()
{
	// Check the socket for new packets (this could be done by a thread instead)
	bool anyActivity = false;
	{
		LAG_STOPWATCH("updateReceivePacketsInternal", 2000);
		anyActivity = updateReceivePacketsInternal();
	}

	if (anyActivity)
	{
		// Sync received packets so we can access them (relevant when using a thread)
		{
			LAG_STOPWATCH("syncPacketQueues", 2000);
			syncPacketQueues();
		}

		// Handle incoming TCP connections
		{
			LAG_STOPWATCH("TCP connections", 2000);
			std::list<TCPSocket>& incomingTCPConnections = getIncomingTCPConnections();
			while (!incomingTCPConnections.empty())
			{
				// Create a new connection
				NetConnection* connection = mListener.createNetConnection(*this, SocketAddress());	// TODO: Fill in the actual sender address, in case some implementation wants to use it
				if (nullptr != connection)
				{
					// Setup connection
					connection->setupWithTCPSocket(*this, incomingTCPConnections.front());
				}
				incomingTCPConnections.pop_front();
			}
		}

		// Handle incoming packets
		{
			LAG_STOPWATCH("Incoming packets", 2000);
			while (true)
			{
				const ReceivedPacket* receivedPacket = getNextReceivedPacket();
				if (nullptr == receivedPacket)
				{
					// Handled all current packets
					break;
				}

				// Actual handling depending on packet type and connection
				handleReceivedPacket(*receivedPacket);

				// Return the packet if nobody increased the reference counter meanwhile
				//  -> E.g. the ReceivedPacketCache does this when adding a high-level packet into its queue of pending packets
				LAG_STOPWATCH("decReferenceCounter", 500);
				receivedPacket->decReferenceCounter();
			}
		}
	}

	// Update connections
	{
		LAG_STOPWATCH("updateConnections", 2000);
		updateConnections(getCurrentTimestamp());
	}

	// Done
	return anyActivity;
}

bool ConnectionManager::sendUDPPacketData(const std::vector<uint8>& data, const SocketAddress& remoteAddress)
{
#ifdef DEBUG
	// Simulate packet loss
	if (mDebugSettings.mSendingPacketLoss > 0.0f && randomf() < mDebugSettings.mSendingPacketLoss)
	{
		// Act as if the packet was sent successfully
		return true;
	}
#endif

	RMX_ASSERT(nullptr != mUDPSocket, "No UDP socket set");
	return mUDPSocket->sendData(data, remoteAddress);
}

bool ConnectionManager::sendTCPPacketData(const std::vector<uint8>& data, TCPSocket& socket, bool isWebSocketServer)
{
#ifdef DEBUG
	// Simulate packet loss
	if (mDebugSettings.mSendingPacketLoss > 0.0f && randomf() < mDebugSettings.mSendingPacketLoss)
	{
		// Act as if the packet was sent successfully
		return true;
	}
#endif

	if (isWebSocketServer)
	{
		static std::vector<uint8> wrappedData;
		WebSocketWrapper::wrapDataToSendToClient(data, wrappedData);
		return socket.sendData(wrappedData);
	}
	else
	{
		return socket.sendData(data);
	}
}

bool ConnectionManager::sendConnectionlessLowLevelPacket(lowlevel::PacketBase& lowLevelPacket, const SocketAddress& remoteAddress, uint16 localConnectionID, uint16 remoteConnectionID)
{
	// Write low-level packet header
	static std::vector<uint8> sendBuffer;
	sendBuffer.clear();

	VectorBinarySerializer serializer(false, sendBuffer);
	serializer.write(lowLevelPacket.getSignature());
	serializer.write(localConnectionID);
	serializer.write(remoteConnectionID);
	lowLevelPacket.serializePacket(serializer, lowlevel::PacketBase::LOWLEVEL_PROTOCOL_VERSIONS.mMinimum);

	return sendUDPPacketData(sendBuffer, remoteAddress);
}

void ConnectionManager::terminateAllConnections()
{
	while (!mActiveConnections.empty())
	{
		mActiveConnections.begin()->second->disconnect(NetConnection::DisconnectReason::MANUAL_LOCAL);
	}
}

void ConnectionManager::addConnection(NetConnection& connection)
{
	// Create a local connection ID
	const ConnectionsProvider::Entry& newEntry = mConnectionsProvider.createHandle();
	RMX_CHECK(ConnectionsProvider::isValidEntry(newEntry), "Error in connection management: Could not assign a valid connection ID", return);

	const uint16 localConnectionID = newEntry.mHandle;
	connection.mLocalConnectionID = localConnectionID;

	newEntry.mContent = &connection;
	mActiveConnections[localConnectionID] = &connection;
	mConnectionsBySender[connection.getSenderKey()] = &connection;

	if (connection.mSocketType == NetConnection::SocketType::TCP_SOCKET)
	{
		// Register as a TCP connection & socket to be polled regularly
		mTCPNetConnections.push_back(&connection);
	}
}

void ConnectionManager::removeConnection(NetConnection& connection)
{
	// This method gets called from "NetConnection::clear", so it's safe to assume the connection gets cleaned up internally already
	mActiveConnections.erase(connection.getLocalConnectionID());
	mConnectionsBySender.erase(connection.getSenderKey());
	mConnectionsProvider.destroyHandle(connection.getLocalConnectionID());

	// TODO: Maybe reduce size of "mActiveConnectionsLookup" again if there's only few connections left - and if this does not produce any conflicts

	if (connection.mSocketType == NetConnection::SocketType::TCP_SOCKET)
	{
		// Unregister again
		for (size_t index = 0; index < mTCPNetConnections.size(); ++index)
		{
			if (&connection == mTCPNetConnections[index])
			{
				mTCPNetConnections.erase(mTCPNetConnections.begin() + index);
				break;
			}
		}
	}
}

SentPacket& ConnectionManager::rentSentPacket()
{
	SentPacket& sentPacket = mSentPacketPool.rentObject();
	sentPacket.initializeWithPool(mSentPacketPool);
	return sentPacket;
}

ReceivedPacket& ConnectionManager::createNewReceivedPacket(const std::vector<uint8>& buffer, uint16 lowLevelSignature, const SocketAddress& senderAddress, NetConnection* connection)
{
	ReceivedPacket& receivedPacket = mReceivedPacketPool.rentObject();
	receivedPacket.mContent = buffer;
	receivedPacket.mLowLevelSignature = lowLevelSignature;
	receivedPacket.mSenderAddress = senderAddress;
	receivedPacket.mConnection = connection;

#ifdef DEBUG
	receivedPacket.mDelayedDeliveryTime = 0;
	if (mDebugSettings.mReceivingDelayAverage > 0.0f)
	{
		const float delay = mDebugSettings.mReceivingDelayAverage + (randomf() * 2.0f - 1.0f) * mDebugSettings.mReceivingDelayVariance;
		if (delay > 0.0f)
			receivedPacket.mDelayedDeliveryTime = getCurrentTimestamp() + roundToInt(delay * 1000.0f);
	}
#endif

	mReceivedPackets.mWorkerQueue.emplace_back(&receivedPacket);
	return receivedPacket;
}

void ConnectionManager::receivedPacketInternal(const std::vector<uint8>& buffer, const SocketAddress& senderAddress, NetConnection* connection)
{
	// Ignore too small packets
	if (buffer.size() < 6)
		return;

#ifdef DEBUG
	// Simulate packet loss
	if (mDebugSettings.mReceivingPacketLoss > 0.0f && randomf() < mDebugSettings.mReceivingPacketLoss)
		return;
#endif

	// Received a packet, check its header
	VectorBinarySerializer serializer(true, buffer);
	const uint16 lowLevelSignature = serializer.read<uint16>();
	const uint16 remoteConnectionID = serializer.read<uint16>();
	const uint16 localConnectionID = serializer.read<uint16>();

	if (localConnectionID == 0)
	{
		// Connectionless packet (including start connection packet)

		// Store for later evaluation (in "getNextReceivedPacket")
		createNewReceivedPacket(buffer, lowLevelSignature, senderAddress, connection);
	}
	else
	{
		// Packet is associated with a connection

		// We already know the connection for TCP, but not for UDP
		if (nullptr == connection)
		{
			// Find the connection in our list of active connections
			NetConnection** ptr = mConnectionsProvider.resolveHandle(localConnectionID);
			if (nullptr != ptr)
				connection = *ptr;
		}

		if (nullptr == connection)
		{
			// Unknown connection
			if (lowLevelSignature == lowlevel::ErrorPacket::SIGNATURE)
			{
				// It's an error packet, don't send anything back
			}
			else
			{
				// Send back an error packet
				lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::CONNECTION_INVALID);
				sendConnectionlessLowLevelPacket(errorPacket, senderAddress, 0, remoteConnectionID);
			}
		}
		else
		{
			if (connection->getRemoteConnectionID() != remoteConnectionID && lowLevelSignature != lowlevel::AcceptConnectionPacket::SIGNATURE)
			{
				// Unknown connection
				if (lowLevelSignature == lowlevel::ErrorPacket::SIGNATURE)
				{
					// Evaluate the error packet
					lowlevel::ErrorPacket errorPacket;
					errorPacket.serializePacket(serializer, lowlevel::PacketBase::LOWLEVEL_PROTOCOL_VERSIONS.mMinimum);
					switch (errorPacket.mErrorCode)
					{
						case lowlevel::ErrorPacket::ErrorCode::CONNECTION_INVALID:
						{
							// Invalidate connection
							connection->disconnect(NetConnection::DisconnectReason::MANUAL_REMOTE);
							return;
						}

						default:
							break;
					}
				}
				else
				{
					// Send back an error packet
					lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::CONNECTION_INVALID);
					sendConnectionlessLowLevelPacket(errorPacket, senderAddress, 0, remoteConnectionID);
				}
			}
			else
			{
				// Store for later evaluation (in "getNextReceivedPacket")
				createNewReceivedPacket(buffer, lowLevelSignature, senderAddress, connection);
			}
		}
	}
}

void ConnectionManager::updateConnections(uint64 currentTimestamp)
{
	for (const ConnectionsProvider::Entry& entry : mConnectionsProvider.getEntries())
	{
		if (ConnectionsProvider::isValidEntry(entry))
		{
			// Update connection
			NetConnection& connection = *entry.mContent;
			connection.updateConnection(currentTimestamp);
		}
	}
}

bool ConnectionManager::updateReceivePacketsInternal()
{
	bool anyActivity = !mReceivedPackets.mWorkerQueue.empty();

	// Update UDP
	if (nullptr != mUDPSocket)
	{
		for (int runs = 0; runs < 10; ++runs)
		{
			// Receive next packet
			static UDPSocket::ReceiveResult received;
			const bool success = mUDPSocket->receiveNonBlocking(received);
			if (!success)
			{
				// TODO: Handle error in socket
				return false;
			}

			if (received.mBuffer.empty())
			{
				// Nothing to do at the moment
				break;
			}

			anyActivity = true;
			receivedPacketInternal(received.mBuffer, received.mSenderAddress, nullptr);
		}
	}

	// Update TCP listen socket (server only)
	if (nullptr != mTCPListenSocket)
	{
		// Accept new connections
		for (int runs = 0; runs < 3; ++runs)
		{
			TCPSocket newSocket;
			if (!mTCPListenSocket->acceptConnection(newSocket))
				break;

			RMX_LOG_INFO("Accepted TCP connection");
			anyActivity = true;
			mIncomingTCPConnections.emplace_back();
			mIncomingTCPConnections.back().swapWith(newSocket);
		}
	}

	// Update net connections' TCP sockets
	for (NetConnection* connection : mTCPNetConnections)
	{
		// Receive next packet
		static TCPSocket::ReceiveResult received;
		const bool success = connection->mTCPSocket.receiveNonBlocking(received);
		if (!success)
		{
			// TODO: Handle error in socket
			return false;
		}

		if (!received.mBuffer.empty())
		{
			anyActivity = true;
			if (connection->getState() == NetConnection::State::TCP_READY)
			{
				String webSocketKey;
				if (WebSocketWrapper::handleWebSocketHttpHeader(received.mBuffer, webSocketKey))
				{
					String response;
					WebSocketWrapper::getWebSocketHttpResponse(webSocketKey, response);

					connection->mIsWebSocketServer = true;
					connection->mTCPSocket.sendData((const uint8*)response.getData(), response.length());
					continue;
				}
			}

			if (connection->mIsWebSocketServer)
			{
				if (WebSocketWrapper::processReceivedClientPacket(received.mBuffer))
				{
					receivedPacketInternal(received.mBuffer, connection->getRemoteAddress(), connection);
				}
			}
			else
			{
				receivedPacketInternal(received.mBuffer, connection->getRemoteAddress(), connection);
			}
		}
	}

	return anyActivity;
}

void ConnectionManager::syncPacketQueues()
{
	// TODO: Lock mutex, so the worker thread stops briefly
	//  -> This is needed for the worker queue, and also for the "mReceivedPacketPool"

	// First sync queues
#if DEBUG
	{
		const uint64 currentTime = getCurrentTimestamp();
		std::vector<ReceivedPacket*> delayedPackets;

		while (!mReceivedPackets.mWorkerQueue.empty())
		{
			ReceivedPacket* receivedPacket = mReceivedPackets.mWorkerQueue.front();
			if (receivedPacket->mDelayedDeliveryTime <= currentTime)
			{
				mReceivedPackets.mSyncedQueue.push_back(receivedPacket);
			}
			else
			{
				delayedPackets.push_back(receivedPacket);
			}
			mReceivedPackets.mWorkerQueue.pop_front();
		}

		for (ReceivedPacket* delayedPacket : delayedPackets)
		{
			mReceivedPackets.mWorkerQueue.push_back(delayedPacket);
		}
	}
#else
	{
		while (!mReceivedPackets.mWorkerQueue.empty())
		{
			mReceivedPackets.mSyncedQueue.push_back(mReceivedPackets.mWorkerQueue.front());
			mReceivedPackets.mWorkerQueue.pop_front();
		}
	}
#endif

	// Cleanup packets previously marked to be returned
	{
		for (const ReceivedPacket* receivedPacket : mReceivedPackets.mToBeReturned.mPackets)
		{
			mReceivedPacketPool.returnObject(*const_cast<ReceivedPacket*>(receivedPacket));
		}
		mReceivedPackets.mToBeReturned.mPackets.clear();
	}

	// TODO: Mutex can be unlocked here
}

ReceivedPacket* ConnectionManager::getNextReceivedPacket()
{
	if (mReceivedPackets.mSyncedQueue.empty())
		return nullptr;

	ReceivedPacket* receivedPacket = mReceivedPackets.mSyncedQueue.front();
	mReceivedPackets.mSyncedQueue.pop_front();

	// Packet initialization:
	//  - Set the "dump" instance that packets get returned to when the reference counter reaches zero
	//  - Start with a reference count of 1
	receivedPacket->initializeWithDump(&mReceivedPackets.mToBeReturned);
	return receivedPacket;
}

void ConnectionManager::handleReceivedPacket(const ReceivedPacket& receivedPacket)
{
	if (receivedPacket.mLowLevelSignature == lowlevel::StartConnectionPacket::SIGNATURE)
	{
		// It's a connection start request
		LAG_STOPWATCH("handleStartConnectionPacket", 500);
		handleStartConnectionPacket(receivedPacket);
	}
	else if (nullptr == receivedPacket.mConnection)
	{
		// It's a connectionless packet
		VectorBinarySerializer serializer(true, receivedPacket.mContent);
		serializer.skip(6);		// Skip low level signature and connection IDs, they got evaluated already

		ConnectionlessPacketEvaluation evaluation(*this, receivedPacket.mSenderAddress, receivedPacket.mLowLevelSignature, serializer);
		mListener.onReceivedConnectionlessPacket(evaluation);
	}
	else
	{
		// It's a packet for a connection
		LAG_STOPWATCH("handleLowLevelPacket", 500);
		receivedPacket.mConnection->handleLowLevelPacket(receivedPacket);
	}
}

void ConnectionManager::handleStartConnectionPacket(const ReceivedPacket& receivedPacket)
{
	VectorBinarySerializer serializer(true, receivedPacket.mContent);
	serializer.skip(2);		// Skip low level signature, it can be found in "ReceivedPacket::mLowLevelSignature" and was checked already
	const uint16 remoteConnectionID = serializer.read<uint16>();
	serializer.skip(2);		// Skip the other connection ID, as it's invalid anyways (it's only in there to have all low-level packets share the same header)

	lowlevel::StartConnectionPacket packet;
	if (!packet.serializePacket(serializer, LOWLEVEL_PROTOCOL_VERSION_RANGE.mMinimum))
	{
		// Error in packet format, just ignore packet
		return;
	}

	uint8 lowLevelVersion = 0;
	uint8 highLevelVersion = 0;
	const ProtocolVersionChecker::Result resultLowLevel = ProtocolVersionChecker::chooseVersion(lowLevelVersion, LOWLEVEL_PROTOCOL_VERSION_RANGE, packet.mLowLevelProtocolVersionRange);
	const ProtocolVersionChecker::Result resultHighLevel = ProtocolVersionChecker::chooseVersion(highLevelVersion, getHighLevelProtocolVersionRange(), packet.mHighLevelProtocolVersionRange);
	if (resultLowLevel != ProtocolVersionChecker::Result::SUCCESS || resultHighLevel != ProtocolVersionChecker::Result::SUCCESS)
	{
		// Send back an error
		const bool localIsTooOld = (resultLowLevel == ProtocolVersionChecker::Result::ERROR_TOO_NEW || resultHighLevel == ProtocolVersionChecker::Result::ERROR_TOO_NEW);
		lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::UNSUPPORTED_VERSION, localIsTooOld ? 1 : 0);
		sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
		return;
	}

	// In case of a TCP connection, the NetConnection pointer was already set
	NetConnection* connection = receivedPacket.mConnection;
	if (nullptr != connection)
	{
		// Setup connection
		connection->setProtocolVersions(lowLevelVersion, highLevelVersion);
		connection->acceptIncomingConnectionTCP(*this, remoteConnectionID);
	}
	else
	{
		const uint64 senderKey = NetConnection::buildSenderKey(receivedPacket.mSenderAddress, remoteConnectionID);
		connection = findConnectionTo(senderKey);
		if (nullptr != connection)
		{
			if (connection->isConnectedTo(connection->getLocalConnectionID(), remoteConnectionID, senderKey))
			{
				// Resend the AcceptConnectionPacket, as it seems the last one got lost
				//  -> But do this only if this connection did not receive any packets with a unique packet ID yet
				//  -> This check is added to prevent a situation where a client randomly uses an old connection ID again,
				//      and the associated old connection on server side did already receive unique packet IDs,
				//      which leads to the client sending packets with unique packet IDs that the server rejects as duplicates
				if (!connection->receivedAnyUniquePacketIDs())
				{
					connection->sendAcceptConnectionPacket();
				}
				return;
			}

			// Destroy that old connection
			connection->clear();
			mListener.destroyNetConnection(*connection);
		}
		else
		{
			// Check if another connection would reach the limit of concurrent connections
			if (getNumActiveConnections() + 1 >= MAX_NUM_ACTIVE_CONNECTIONS)
			{
				lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::TOO_MANY_CONNECTIONS);
				sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
				return;
			}
		}

		// Create a new connection
		connection = mListener.createNetConnection(*this, receivedPacket.mSenderAddress);
		if (nullptr != connection)
		{
			// Setup connection
			connection->setProtocolVersions(lowLevelVersion, highLevelVersion);
			connection->acceptIncomingConnectionUDP(*this, remoteConnectionID, receivedPacket.mSenderAddress, senderKey);
		}
		else
		{
			// Send back an error
			lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::CONNECTION_INVALID);
			sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
		}
	}
}

NetConnection* ConnectionManager::findConnectionTo(uint64 senderKey) const
{
	const auto it = mConnectionsBySender.find(senderKey);
	return (it == mConnectionsBySender.end()) ? nullptr : it->second;
}
