/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/LagStopwatch.h"
#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/HighLevelPacketBase.h"
#include "oxygen_netcore/network/RequestBase.h"


uint64 NetConnection::buildSenderKey(const SocketAddress& remoteAddress, uint16 remoteConnectionID)
{
	return remoteAddress.getHash() ^ remoteConnectionID;
}

NetConnection::NetConnection() :
	mWebSocketClient(*this)
{
}

NetConnection::~NetConnection()
{
	clear();
}

void NetConnection::clear()
{
	mState = State::EMPTY;
	mTimeoutStart = 0;

	for (auto& pair : mOpenRequests)
	{
		pair.second->mRegisteredAtConnection = nullptr;
	}
	mOpenRequests.clear();

	if (nullptr != mConnectionManager)
	{
		mConnectionManager->removeConnection(*this);
		mConnectionManager = nullptr;
	}

	mSentPacketCache.clear();
	mReceivedPacketCache.clear();

	mTCPSocket.close();
	mWebSocketClient.clear();
}

UDPSocket* NetConnection::getUDPSocket() const
{
	return (nullptr == mConnectionManager) ? nullptr : mConnectionManager->getUDPSocket();
}

void NetConnection::setProtocolVersions(uint8 lowLevelProtocolVersion, uint8 highLevelProtocolVersion)
{
	mLowLevelProtocolVersion = lowLevelProtocolVersion;
	mHighLevelProtocolVersion = highLevelProtocolVersion;
}

void NetConnection::setupWithTCPSocket(ConnectionManager& connectionManager, TCPSocket& socketToMove, uint64 currentTimestamp)
{
	clear();

	mState = State::TCP_READY;
	mConnectionManager = &connectionManager;

	mSocketType = NetConnection::SocketType::TCP_SOCKET;
	mTCPSocket.swapWith(socketToMove);
	mRemoteAddress = mTCPSocket.getRemoteAddress();

	mCurrentTimestamp = currentTimestamp;
	mLastMessageReceivedTimestamp = mCurrentTimestamp;	// Set here just to start with a valid timestamp

	mConnectionManager->addConnection(*this);
}

bool NetConnection::startConnectTo(ConnectionManager& connectionManager, const SocketAddress& remoteAddress, uint64 currentTimestamp)
{
	clear();

	mState = State::REQUESTED_CONNECTION;
	mConnectionManager = &connectionManager;
	mLocalConnectionID = 0;			// Not yet set, see "addConnection" below
	mRemoteConnectionID = 0;		// Not yet set
	mRemoteAddress = remoteAddress;
	mSenderKey = 0;					// Not yet set as it depends on the remote connection ID

	mCurrentTimestamp = currentTimestamp;
	mLastMessageReceivedTimestamp = mCurrentTimestamp;	// Set here just to start with a valid timestamp

	// Use TCP if UDP is not available
	if (connectionManager.hasUDPSocket())
	{
		mSocketType = NetConnection::SocketType::UDP_SOCKET;
	}
	else if (mWebSocketClient.isAvailable())
	{
		if (!mWebSocketClient.connectTo(remoteAddress))
			return false;

		mSocketType = NetConnection::SocketType::WEB_SOCKET;

		// Register connection, but don't call "finishStartConnect" yet
		mConnectionManager->addConnection(*this);
		return true;
	}
	else
	{
		// Establish a TCP connection first
		//  -> Note that this is a blocking call!
		const bool success = mTCPSocket.connectTo(remoteAddress.getIP(), remoteAddress.getPort());
		if (!success)
			return false;

		mSocketType = NetConnection::SocketType::TCP_SOCKET;
	}

	// Register connection; this will also set the local connection ID
	mConnectionManager->addConnection(*this);

	return finishStartConnect();
}

bool NetConnection::isConnectedTo(uint16 localConnectionID, uint16 remoteConnectionID, uint64 senderKey) const
{
	return (nullptr != mConnectionManager && mState == State::CONNECTED && localConnectionID == mLocalConnectionID && remoteConnectionID == mRemoteConnectionID && senderKey == mSenderKey);
}

void NetConnection::disconnect(DisconnectReason disconnectReason)
{
	clear();
	mState = State::DISCONNECTED;
	mDisconnectReason = disconnectReason;
}

bool NetConnection::sendPacket(highlevel::PacketBase& packet, SendFlags::Flags flags)
{
	uint32 unused;
	return sendHighLevelPacket(packet, flags, unused);
}

bool NetConnection::sendRequest(highlevel::RequestBase& request)
{
	if (nullptr != request.mRegisteredAtConnection)
	{
		// Request was already sent before and is still an open request; invalidate it
		request.mRegisteredAtConnection->unregisterRequest(request);
	}

	request.mState = highlevel::RequestBase::State::SENT;

	// Send query packet
	lowlevel::RequestQueryPacket highLevelPacket;
	if (!sendHighLevelPacket(highLevelPacket, request.getQueryPacket(), SendFlags::NONE, request.mUniqueRequestID))
		return false;

	// Register here
	mOpenRequests[request.mUniqueRequestID] = &request;
	request.mRegisteredAtConnection = this;
	return true;
}

bool NetConnection::respondToRequest(highlevel::RequestBase& request, uint32 uniqueRequestID)
{
	lowlevel::RequestResponsePacket highLevelPacket;
	highLevelPacket.mUniqueRequestID = uniqueRequestID;
	uint32 unused;
	return sendHighLevelPacket(highLevelPacket, request.getResponsePacket(), SendFlags::NONE, unused);
}

bool NetConnection::readPacket(highlevel::PacketBase& packet, VectorBinarySerializer& serializer) const
{
	return packet.serializePacket(serializer, mHighLevelProtocolVersion);
}

void NetConnection::updateConnection(uint64 currentTimestamp)
{
	mCurrentTimestamp = currentTimestamp;

	// Updates that need to be called only every 100 milliseconds
	if (mCurrentTimestamp >= mLast100msUpdate + 100)
	{
		mLast100msUpdate = mCurrentTimestamp;

		// Update resending
		mPacketsToResend.clear();
		mSentPacketCache.updateResend(mPacketsToResend, currentTimestamp);

		for (const SentPacket* sentPacket : mPacketsToResend)
		{
			sendPacketInternal(sentPacket->mContent);
		}

		// Updates that need to be called only every second
		if (mCurrentTimestamp >= mLast1000msUpdate + 1000)
		{
			mLast1000msUpdate = mCurrentTimestamp;

			// Update timeout
			if (mSentPacketCache.hasUnconfirmedPackets() && mTimeoutStart != 0)
			{
				if (mCurrentTimestamp >= mTimeoutStart + TIMEOUT_SECONDS * 1000)
				{
					// Trigger timeout
					RMX_LOG_INFO("Disconnect due to timeout");
					disconnect(DisconnectReason::TIMEOUT);
					return;
				}
			}
			else
			{
				// Not waiting for any confirmations, so reset timeout
				mTimeoutStart = mCurrentTimestamp;
			}

			// Even if no timeout is running, check if this connection is stale (i.e. there was no communication for some minutes)
			if (mCurrentTimestamp >= mLastMessageReceivedTimestamp + STALE_SECONDS * 1000)
			{
				// Trigger disconnect
				RMX_LOG_INFO("Disconnect due to stale connection");
				disconnect(DisconnectReason::STALE);
				return;
			}
		}

		// TODO: Send a heartbeat every now and then
		//  -> But only if a heartbeat is even needed (the client should send one regularly, the server only sends a response back)

	}
}

void NetConnection::acceptIncomingConnectionUDP(ConnectionManager& connectionManager, uint16 remoteConnectionID, const SocketAddress& remoteAddress, uint64 senderKey, uint64 currentTimestamp)
{
	clear();

	mState = State::CONNECTED;
	mConnectionManager = &connectionManager;
	mLocalConnectionID = 0;			// Not yet set, see "addConnection" below
	mRemoteConnectionID = remoteConnectionID;
	mRemoteAddress = remoteAddress;
	mSenderKey = senderKey;
	RMX_ASSERT(senderKey == buildSenderKey(mRemoteAddress, mRemoteConnectionID), "Previously calculated sender key is the wrong one");

	mCurrentTimestamp = currentTimestamp;
	mLastMessageReceivedTimestamp = mCurrentTimestamp;	// Because we just received a packet

	RMX_LOG_INFO("Accepting connection via UDP from " << mRemoteAddress.toLoggedString());
	mConnectionManager->addConnection(*this);	// This will also set the local connection ID

	// Send back a response
	sendAcceptConnectionPacket();
}

void NetConnection::acceptIncomingConnectionTCP(ConnectionManager& connectionManager, uint16 remoteConnectionID, uint64 currentTimestamp)
{
	mState = State::CONNECTED;
	mRemoteConnectionID = remoteConnectionID;

	mCurrentTimestamp = currentTimestamp;
	mLastMessageReceivedTimestamp = mCurrentTimestamp;	// Because we just received a packet

	RMX_LOG_INFO("Accepting connection via TCP from " << mRemoteAddress.toLoggedString());

	// Send back a response
	sendAcceptConnectionPacket();
}

void NetConnection::sendAcceptConnectionPacket()
{
	lowlevel::AcceptConnectionPacket packet;
	packet.mLowLevelProtocolVersion = mLowLevelProtocolVersion;
	packet.mHighLevelProtocolVersion = mHighLevelProtocolVersion;
	sendLowLevelPacket(packet, mSendBuffer);
}

void NetConnection::handleLowLevelPacket(ReceivedPacket& receivedPacket)
{
	// Reset timeout whenever any packet got received
	mTimeoutStart = mCurrentTimestamp;
	mLastMessageReceivedTimestamp = mCurrentTimestamp;	// TODO: It would be nice to use the actual timestamp of receiving the packet here, which happened previously already

	VectorBinarySerializer serializer(true, receivedPacket.mContent);
	serializer.skip(6);		// Skip low level signature and connection IDs, they got evaluated already
	if (serializer.getRemaining() <= 0)
		return;

	switch (receivedPacket.mLowLevelSignature)
	{
		case lowlevel::AcceptConnectionPacket::SIGNATURE:
		{
			LAG_STOPWATCH("AcceptConnectionPacket", 1000);
			lowlevel::AcceptConnectionPacket packet;
			if (!packet.serializePacket(serializer, lowlevel::PacketBase::LOWLEVEL_PROTOCOL_VERSIONS.mMinimum))
				return;

			if (mState != State::REQUESTED_CONNECTION)
			{
				// Seems like something went wrong during establishing a connection, or maybe this was just a duplicate AcceptConnectionPacket
				return;
			}

			// Check if protocol versions are really supported
			if (!lowlevel::PacketBase::LOWLEVEL_PROTOCOL_VERSIONS.contains(packet.mLowLevelProtocolVersion) ||
				!mConnectionManager->getHighLevelProtocolVersionRange().contains(packet.mHighLevelProtocolVersion))
			{
				RMX_LOG_INFO("Received accept connection packet with unsupported protocol version (low-level = " << packet.mLowLevelProtocolVersion << ", high-level = " << packet.mLowLevelProtocolVersion << ")");
				// TODO: Send back an error?
				return;
			}
			setProtocolVersions(packet.mLowLevelProtocolVersion, packet.mHighLevelProtocolVersion);

			RMX_LOG_INFO("Connection established to " << mRemoteAddress.toLoggedString());

			// Set remote connection ID, as it was not known before
			//  -> Unfortunately, we have to get in a somewhat awkward way, as it was skipped in deserialization before
			mRemoteConnectionID = *(uint16*)serializer.getBufferPointer(2);
			mSenderKey = buildSenderKey(mRemoteAddress, mRemoteConnectionID);
			mState = State::CONNECTED;

			// Stop resending the StartConnectionPacket
			mSentPacketCache.onPacketReceiveConfirmed(0);
			return;
		}

		case lowlevel::HighLevelPacket::SIGNATURE:
		case lowlevel::RequestQueryPacket::SIGNATURE:	// Can be treated the same way, as it does not have any additional members
		{
			LAG_STOPWATCH("HighLevelPacket", 1000);
			lowlevel::HighLevelPacket highLevelPacket;
			if (!highLevelPacket.serializePacket(serializer, mLowLevelProtocolVersion))
				return;

			if (mState != State::CONNECTED)
			{
				RMX_LOG_INFO("Received high-level packet while not connected from " << mRemoteAddress.toLoggedString());
				return;
			}

			// Continue with specialized handling
			handleHighLevelPacket(receivedPacket, highLevelPacket, serializer, 0);
			return;
		}

		case lowlevel::RequestResponsePacket::SIGNATURE:
		{
			LAG_STOPWATCH("RequestResponsePacket", 1000);
			lowlevel::RequestResponsePacket requestResponsePacket;
			if (!requestResponsePacket.serializePacket(serializer, mLowLevelProtocolVersion))
				return;

			if (mState != State::CONNECTED)
			{
				RMX_LOG_INFO("Received high-level packet while not connected from " << mRemoteAddress.toLoggedString());
				return;
			}

			// Continue with specialized handling
			handleHighLevelPacket(receivedPacket, requestResponsePacket, serializer, requestResponsePacket.mUniqueRequestID);
			return;
		}

		case lowlevel::ReceiveConfirmationPacket::SIGNATURE:
		{
			LAG_STOPWATCH("ReceiveConfirmationPacket", 1000);
			lowlevel::ReceiveConfirmationPacket packet;
			if (!packet.serializePacket(serializer, mLowLevelProtocolVersion))
				return;

			// Packet was confirmed by the receiver, so remove it from the cache for re-sending
			mSentPacketCache.onPacketReceiveConfirmed(packet.mUniquePacketID);
			return;
		}
	}
}

void NetConnection::unregisterRequest(highlevel::RequestBase& request)
{
	RMX_ASSERT(this == request.mRegisteredAtConnection, "Unregistering request at the wrong connection");
	request.mRegisteredAtConnection = nullptr;
	mOpenRequests.erase(request.mUniqueRequestID);
}

bool NetConnection::receivedWebSocketPacket(const std::vector<uint8>& content)
{
	if (nullptr == mConnectionManager)
		return false;

	mConnectionManager->receivedPacketInternal(content, mRemoteAddress, this);
	return true;
}

bool NetConnection::finishStartConnect()
{
	// Send a low-level message to establish the connection
	RMX_LOG_INFO("Starting connection to " << mRemoteAddress.toLoggedString());

	// Get a new packet instance to fill
	SentPacket& sentPacket = mConnectionManager->rentSentPacket();

	// Build packet
	lowlevel::StartConnectionPacket packet;
	packet.mLowLevelProtocolVersionRange = lowlevel::PacketBase::LOWLEVEL_PROTOCOL_VERSIONS;
	packet.mHighLevelProtocolVersionRange = mConnectionManager->getHighLevelProtocolVersionRange();

	// And send it
	if (!sendLowLevelPacket(packet, sentPacket.mContent))
	{
		sentPacket.returnToPool();
		return false;
	}

	// Add the packet to the cache, so it can be resent if needed
	mSentPacketCache.addPacket(sentPacket, mCurrentTimestamp, true);
	return true;
}

bool NetConnection::sendPacketInternal(const std::vector<uint8>& content)
{
	if (nullptr == mConnectionManager)
		return false;

	mLastMessageSentTimestamp = mCurrentTimestamp;

	switch (mSocketType)
	{
		case NetConnection::SocketType::UDP_SOCKET:
		{
			LAG_STOPWATCH("sendPacketInternal UDP", 500);
			return mConnectionManager->sendUDPPacketData(content, mRemoteAddress);
		}

		case NetConnection::SocketType::TCP_SOCKET:
		{
			LAG_STOPWATCH(mIsWebSocketServer ? "sendPacketInternal TCP-web" : "sendPacketInternal TCP", 500);
			if (content.size() >= 1000)
				RMX_LOG_INFO("Stopwatch related info: content size was " << content.size());
			return mConnectionManager->sendTCPPacketData(content, mTCPSocket, mIsWebSocketServer);
		}

		case NetConnection::SocketType::WEB_SOCKET:
		{
			return mWebSocketClient.sendPacket(content);
		}
	}
	return false;
}

void NetConnection::writeLowLevelPacketContent(VectorBinarySerializer& serializer, lowlevel::PacketBase& lowLevelPacket)
{
	// Write shared header for all low-level packets
	serializer.write(lowLevelPacket.getSignature());
	serializer.write(mLocalConnectionID);
	serializer.write(mRemoteConnectionID);
	lowLevelPacket.serializePacket(serializer, mLowLevelProtocolVersion);
}

bool NetConnection::sendLowLevelPacket(lowlevel::PacketBase& lowLevelPacket, std::vector<uint8>& buffer)
{
	// Write low-level packet header
	buffer.clear();
	VectorBinarySerializer serializer(false, buffer);
	writeLowLevelPacketContent(serializer, lowLevelPacket);

	// And send it
	return sendPacketInternal(buffer);
}

bool NetConnection::sendHighLevelPacket(highlevel::PacketBase& highLevelPacket, SendFlags::Flags flags, uint32& outUniquePacketID)
{
	// Build the low-level packet header for a generic high-level packet
	//  -> This header has no special members of its own, only the shared ones
	lowlevel::HighLevelPacket lowLevelPacket;
	return sendHighLevelPacket(lowLevelPacket, highLevelPacket, flags, outUniquePacketID);
}

bool NetConnection::sendHighLevelPacket(lowlevel::HighLevelPacket& lowLevelPacket, highlevel::PacketBase& highLevelPacket, SendFlags::Flags flags, uint32& outUniquePacketID)
{
	LAG_STOPWATCH("# sendHighLevelPacket", 500);
	if (nullptr == mConnectionManager)
		return false;

	lowLevelPacket.mPacketType = highLevelPacket.getPacketType();
	lowLevelPacket.mPacketFlags = 0;

	if (highLevelPacket.isReliablePacket() && (flags & SendFlags::UNRELIABLE) == 0)
	{
		lowLevelPacket.mUniquePacketID = mSentPacketCache.getNextUniquePacketID();

		// Get a new packet instance to fill
		SentPacket& sentPacket = mConnectionManager->rentSentPacket();

		// Write low-level packet header
		sentPacket.mContent.clear();
		VectorBinarySerializer serializer(false, sentPacket.mContent);
		writeLowLevelPacketContent(serializer, lowLevelPacket);

		// Now for the high-level packet content
		highLevelPacket.serializePacket(serializer, mHighLevelProtocolVersion);

		// And send it
		if (!sendPacketInternal(sentPacket.mContent))
		{
			sentPacket.returnToPool();
			return false;
		}

		// Add the packet to the cache, so it can be resent if needed
		mSentPacketCache.addPacket(sentPacket, mCurrentTimestamp);
	}
	else
	{
		lowLevelPacket.mUniquePacketID = 0;

		// Write low-level packet header
		mSendBuffer.clear();
		VectorBinarySerializer serializer(false, mSendBuffer);
		writeLowLevelPacketContent(serializer, lowLevelPacket);

		// Now for the high-level packet content
		highLevelPacket.serializePacket(serializer, mHighLevelProtocolVersion);

		// And send it
		if (!sendPacketInternal(mSendBuffer))
			return false;
	}

	outUniquePacketID = lowLevelPacket.mUniquePacketID;
	return true;
}

void NetConnection::handleHighLevelPacket(ReceivedPacket& receivedPacket, const lowlevel::HighLevelPacket& highLevelPacket, VectorBinarySerializer& serializer, uint32 uniqueResponseID)
{
	const std::string signatureString = "HighLevel_" + std::to_string(highLevelPacket.mPacketType);
	LAG_STOPWATCH(signatureString.c_str(), 1000);

	// Is this a tracked packet at all?
	const bool isTracked = (highLevelPacket.mUniquePacketID != 0);
	if (isTracked)
	{
		// In any case, send a confirmation to tell the sender that the tracked packet was received
		//  -> This way the sender knows it does not need to re-send it
		// TODO: It might make sense to send back a single confirmation for all tracked packets received in this update round, lowering overhead in case we got multiple of them at once
		{
			lowlevel::ReceiveConfirmationPacket packet;
			packet.mUniquePacketID = highLevelPacket.mUniquePacketID;
			sendLowLevelPacket(packet, mSendBuffer);
		}

		// Add to / check against queue of received packets
		const bool wasEnqueued = mReceivedPacketCache.enqueuePacket(receivedPacket, highLevelPacket, serializer, uniqueResponseID);
		if (!wasEnqueued)
		{
			// The packet is a duplicate we received already before, so ignore it
			return;
		}

		// Try extracting packets; possibly more than one
		ReceivedPacketCache::CacheItem extracted;
		while (mReceivedPacketCache.extractPacket(extracted))
		{
			processExtractedHighLevelPacket(extracted);

			// Remove the reference we took over from the ReceivedPacketCache
			extracted.mReceivedPacket->decReferenceCounter();
		}
	}
	else
	{
		// Not tracked: Simply forward it as-is to the listener
		ReceivedPacketEvaluation evaluation(*this, highLevelPacket.mPacketType, serializer, 0);
		mConnectionManager->getListener().onReceivedPacket(evaluation);
	}
}

void NetConnection::processExtractedHighLevelPacket(const ReceivedPacketCache::CacheItem& extracted)
{
	LAG_STOPWATCH("processExtractedHighLevelPacket", 1000);

	VectorBinarySerializer newSerializer(true, extracted.mReceivedPacket->mContent);
	newSerializer.skip(extracted.mHeaderSize);

	switch (extracted.mReceivedPacket->mLowLevelSignature)
	{
		case lowlevel::HighLevelPacket::SIGNATURE:
		default:
		{
			// Generic high-level packet
			ReceivedPacketEvaluation evaluation(*this, extracted.mPacketHeader.mPacketType, newSerializer, extracted.mPacketHeader.mUniquePacketID);
			mConnectionManager->getListener().onReceivedPacket(evaluation);
			break;
		}

		case lowlevel::RequestQueryPacket::SIGNATURE:
		{
			// Request query
			ReceivedQueryEvaluation evaluation(*this, extracted.mPacketHeader.mPacketType, newSerializer, extracted.mPacketHeader.mUniquePacketID);
			if (!mConnectionManager->getListener().onReceivedRequestQuery(evaluation))
			{
				// TODO: Send back an error packet, this request won't receive a response
				//  -> This needs to be a tracked high-level packet
			}
			break;
		}

		case lowlevel::RequestResponsePacket::SIGNATURE:
		{
			// Response to a request
			const auto it = mOpenRequests.find(extracted.mUniqueRequestID);
			if (it != mOpenRequests.end())
			{
				highlevel::RequestBase& request = *it->second;
				if (request.getResponsePacket().serializePacket(newSerializer, mHighLevelProtocolVersion))
				{
					request.mState = highlevel::RequestBase::State::SUCCESS;

					// Also inform the listener
					ReceivedRequestEvaluation evaluation(*this, request);
					mConnectionManager->getListener().onReceivedRequestResponse(evaluation);
				}
				else
				{
					request.mState = highlevel::RequestBase::State::FAILED;

					// Also inform the listener
					ReceivedRequestEvaluation evaluation(*this, request);
					mConnectionManager->getListener().onReceivedRequestError(evaluation);
				}

				it->second->mRegisteredAtConnection = nullptr;
				mOpenRequests.erase(it);
			}
			break;
		}
	}
}
