/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/HighLevelPacketBase.h"
#include "oxygen_netcore/network/RequestBase.h"


uint64 NetConnection::buildSenderKey(const SocketAddress& remoteAddress, uint16 remoteConnectionID)
{
	return remoteAddress.getHash() ^ remoteConnectionID;
}

NetConnection::~NetConnection()
{
	clear();
}

void NetConnection::clear()
{
	mState = State::EMPTY;

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
}

UDPSocket* NetConnection::getSocket() const
{
	return (nullptr == mConnectionManager) ? nullptr : &mConnectionManager->getSocket();
}

void NetConnection::setProtocolVersions(uint8 lowLevelProtocolVersion, uint8 highLevelProtocolVersion)
{
	mLowLevelProtocolVersion = lowLevelProtocolVersion;
	mHighLevelProtocolVersion = highLevelProtocolVersion;
}

bool NetConnection::startConnectTo(ConnectionManager& connectionManager, uint16 localConnectionID, const SocketAddress& remoteAddress)
{
	clear();

	mState = State::REQUESTED_CONNECTION;
	mConnectionManager = &connectionManager;
	mLocalConnectionID = localConnectionID;
	mRemoteConnectionID = 0;	// Not yet set
	mRemoteAddress = remoteAddress;
	mSenderKey = 0;				// Not yet set as it depends on the remote connection ID

	mConnectionManager->addConnection(*this);

	// Send a low-level message to establish the connection
	{
		std::cout << "Starting connection to " << mRemoteAddress.toString() << std::endl;

		lowlevel::StartConnectionPacket packet;
		packet.mLowLevelMinimumProtocolVersion = lowlevel::PacketBase::LOWLEVEL_MINIMUM_PROTOCOL_VERSION;
		packet.mLowLevelMaximumProtocolVersion = lowlevel::PacketBase::LOWLEVEL_MAXIMUM_PROTOCOL_VERSION;
		packet.mHighLevelMinimumProtocolVersion = mConnectionManager->getHighLevelMinimumProtocolVersion();
		packet.mHighLevelMaximumProtocolVersion = mConnectionManager->getHighLevelMaximumProtocolVersion();

		if (!sendLowLevelPacket(packet))
			return false;

		// Add to cache so it gets resent
		//  -> Use a unique packet ID of 0 for this special case
		mSentPacketCache.addPacket(0, mSendBuffer, mCurrentTimestamp);
	}
	return true;
}

bool NetConnection::isConnectedTo(uint16 localConnectionID, uint16 remoteConnectionID, uint64 senderKey) const
{
	return (nullptr != mConnectionManager && mState == State::CONNECTED && localConnectionID == mLocalConnectionID && remoteConnectionID == mRemoteConnectionID && senderKey == mSenderKey);
}

void NetConnection::acceptIncomingConnection(ConnectionManager& connectionManager, uint16 localConnectionID, uint16 remoteConnectionID, const SocketAddress& remoteAddress, uint64 senderKey)
{
	clear();

	mState = State::CONNECTED;
	mConnectionManager = &connectionManager;
	mLocalConnectionID = localConnectionID;
	mRemoteConnectionID = remoteConnectionID;
	mRemoteAddress = remoteAddress;
	mSenderKey = senderKey;
	RMX_ASSERT(senderKey == buildSenderKey(mRemoteAddress, mRemoteConnectionID), "Previously calculated sender key is the wrong one");

	mConnectionManager->addConnection(*this);

	// Send back a response
	sendAcceptConnectionPacket();
}

void NetConnection::sendAcceptConnectionPacket()
{
	lowlevel::AcceptConnectionPacket packet;
	packet.mLowLevelProtocolVersion = mLowLevelProtocolVersion;
	packet.mHighLevelProtocolVersion = mHighLevelProtocolVersion;
	sendLowLevelPacket(packet);
}

bool NetConnection::sendPacket(highlevel::PacketBase& packet)
{
	uint32 unused;
	return sendHighLevelPacket(packet, 0, unused);
}

bool NetConnection::sendRequest(highlevel::RequestBase& request)
{
	if (nullptr != request.mRegisteredAtConnection)
	{
		RMX_ASSERT(false, "Request can't get sent twice");
		return false;
	}

	request.mHasResponse = false;

	// Send query packet
	lowlevel::RequestQueryPacket highLevelPacket;
	if (!sendHighLevelPacket(highLevelPacket, request.getQueryPacket(), 0, request.mUniqueRequestID))
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
	return sendHighLevelPacket(highLevelPacket, request.getResponsePacket(), 0, unused);
}

bool NetConnection::readPacket(highlevel::PacketBase& packet, VectorBinarySerializer& serializer) const
{
	return packet.serializePacket(serializer, mHighLevelProtocolVersion);
}

void NetConnection::updateConnection(uint64 currentTimestamp)
{
	mCurrentTimestamp = currentTimestamp;

	// Update resending
	mItemsToResend.clear();
	mSentPacketCache.updateResend(mItemsToResend, currentTimestamp);

	for (const SentPacketCache::CacheItem* item : mItemsToResend)
	{
		sendPacketInternal(item->mContent);
	}

	// TODO: Check for timeout of the connection
	//  -> This happens when we did not receive any packets for a while despite waiting for responses

	// TODO: Send a heartbeat every now and then
	//  -> But only if a heartbeat is even needed
	//  -> E.g. it can be omitted for a connection to a public server that does not send other packets than direct responses

}

void NetConnection::unregisterRequest(highlevel::RequestBase& request)
{
	mOpenRequests.erase(request.mUniqueRequestID);
}

bool NetConnection::sendPacketInternal(const std::vector<uint8>& content)
{
	#ifdef DEBUG
	{
		if (mConnectionManager->mDebugSettings.mSendingPacketLoss > 0.0f)
		{
			if (randomf() < mConnectionManager->mDebugSettings.mSendingPacketLoss)
			{
				// Act as if the packet was sent successfully
				return true;
			}
		}
	}
	#endif

	mLastMessageSentTimestamp = mCurrentTimestamp;
	return getSocket()->sendData(mSendBuffer, mRemoteAddress);
}

void NetConnection::writeLowLevelPacketContent(VectorBinarySerializer& serializer, lowlevel::PacketBase& lowLevelPacket)
{
	// Write shared header for all low-level packets
	serializer.write(lowLevelPacket.getSignature());
	serializer.write(mLocalConnectionID);
	serializer.write(mRemoteConnectionID);
	lowLevelPacket.serializePacket(serializer, mLowLevelProtocolVersion);
}

bool NetConnection::sendLowLevelPacket(lowlevel::PacketBase& lowLevelPacket)
{
	// Write low-level packet header
	mSendBuffer.clear();
	VectorBinarySerializer serializer(false, mSendBuffer);
	writeLowLevelPacketContent(serializer, lowLevelPacket);

	// And send it
	return sendPacketInternal(mSendBuffer);
}

bool NetConnection::sendHighLevelPacket(highlevel::PacketBase& highLevelPacket, uint8 flags, uint32& outUniquePacketID)
{
	// Build the low-level packet header for a generic high-level packet
	//  -> This header has no special members of its own, only the shared ones
	lowlevel::HighLevelPacket lowLevelPacket;
	return sendHighLevelPacket(lowLevelPacket, highLevelPacket, flags, outUniquePacketID);
}

bool NetConnection::sendHighLevelPacket(lowlevel::HighLevelPacket& lowLevelPacket, highlevel::PacketBase& highLevelPacket, uint8 flags, uint32& outUniquePacketID)
{
	lowLevelPacket.mPacketType = highLevelPacket.getPacketType();
	lowLevelPacket.mPacketFlags = flags;

	// TODO: Do not use a unique packet ID if the packet (instance or type?) is not using tracking at all, but set a value of 0
	lowLevelPacket.mUniquePacketID = mSentPacketCache.getNextUniquePacketID();

	// Write low-level packet header
	mSendBuffer.clear();
	VectorBinarySerializer serializer(false, mSendBuffer);
	writeLowLevelPacketContent(serializer, lowLevelPacket);

	// Now for the high-level packet content
	highLevelPacket.serializePacket(serializer, mHighLevelProtocolVersion);

	// And send it
	if (!sendPacketInternal(mSendBuffer))
		return false;

	if (lowLevelPacket.mUniquePacketID != 0)
	{
		// Add the packet to the cache, so it can be resent if needed
		mSentPacketCache.addPacket(lowLevelPacket.mUniquePacketID, mSendBuffer, mCurrentTimestamp);
	}

	outUniquePacketID = lowLevelPacket.mUniquePacketID;
	return true;
}

void NetConnection::handleLowLevelPacket(ReceivedPacket& receivedPacket)
{
	mLastMessageReceivedTimestamp = mCurrentTimestamp;	// TODO: It would be nice to use the actual timestamp of receiving the packet here, which happened previously already

	VectorBinarySerializer serializer(true, receivedPacket.mContent);
	serializer.skip(6);		// Skip low level signature and connection IDs, they got evaluated already
	if (serializer.getRemaining() <= 0)
		return;

	switch (receivedPacket.mLowLevelSignature)
	{
		case lowlevel::AcceptConnectionPacket::SIGNATURE:
		{
			lowlevel::AcceptConnectionPacket packet;
			if (!packet.serializePacket(serializer, lowlevel::PacketBase::LOWLEVEL_MINIMUM_PROTOCOL_VERSION))
				return;

			if (mState != State::REQUESTED_CONNECTION)
			{
				// Seems like something went wrong during establishing a connection, or maybe this was just a duplicate AcceptConnectionPacket
				return;
			}

			// Check if protocol versions are really supported
			if (packet.mLowLevelProtocolVersion < lowlevel::PacketBase::LOWLEVEL_MINIMUM_PROTOCOL_VERSION || packet.mLowLevelProtocolVersion > lowlevel::PacketBase::LOWLEVEL_MAXIMUM_PROTOCOL_VERSION ||
				packet.mHighLevelProtocolVersion < mConnectionManager->getHighLevelMinimumProtocolVersion() || packet.mHighLevelProtocolVersion > mConnectionManager->getHighLevelMaximumProtocolVersion())
			{
				std::cout << "Received accept connection packet with unsupported protocol version (low-level = " << packet.mLowLevelProtocolVersion << ", high-level = " << packet.mLowLevelProtocolVersion << ")" << std::endl;
				// TODO: Send back an error?
				return;
			}
			setProtocolVersions(packet.mLowLevelProtocolVersion, packet.mHighLevelProtocolVersion);

			std::cout << "Connection established to " << mRemoteAddress.toString() << std::endl;

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
			lowlevel::HighLevelPacket highLevelPacket;
			if (!highLevelPacket.serializePacket(serializer, mLowLevelProtocolVersion))
				return;

			if (mState != State::CONNECTED)
			{
				std::cout << "Received high-level packet while not connected from " << mRemoteAddress.toString() << std::endl;
				return;
			}

			// Continue with specialized handling
			handleHighLevelPacket(receivedPacket, highLevelPacket, serializer, 0);
			return;
		}

		case lowlevel::RequestResponsePacket::SIGNATURE:
		{
			lowlevel::RequestResponsePacket requestResponsePacket;
			if (!requestResponsePacket.serializePacket(serializer, mLowLevelProtocolVersion))
				return;

			if (mState != State::CONNECTED)
			{
				std::cout << "Received high-level packet while not connected from " << mRemoteAddress.toString() << std::endl;
				return;
			}

			// Continue with specialized handling
			handleHighLevelPacket(receivedPacket, requestResponsePacket, serializer, requestResponsePacket.mUniqueRequestID);
			return;
		}

		case lowlevel::ReceiveConfirmationPacket::SIGNATURE:
		{
			lowlevel::ReceiveConfirmationPacket packet;
			if (!packet.serializePacket(serializer, mLowLevelProtocolVersion))
				return;

			// Packet was confirmed by the receiver, so remove it from the cache for re-sending
			mSentPacketCache.onPacketReceiveConfirmed(packet.mUniquePacketID);
			return;
		}
	}
}

void NetConnection::handleHighLevelPacket(ReceivedPacket& receivedPacket, const lowlevel::HighLevelPacket& highLevelPacket, VectorBinarySerializer& serializer, uint32 uniqueResponseID)
{
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
			sendLowLevelPacket(packet);
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
		}
	}
	else
	{
		// Not tracked: Simply forward it as-is to the listener
		ReceivedPacketEvaluation evaluation(*this, highLevelPacket.mPacketType, serializer);
		mConnectionManager->getListener().onReceivedPacket(evaluation);
	}
}

void NetConnection::processExtractedHighLevelPacket(const ReceivedPacketCache::CacheItem& extracted)
{
	VectorBinarySerializer newSerializer(true, extracted.mReceivedPacket->mContent);
	newSerializer.skip(extracted.mHeaderSize);

	switch (extracted.mReceivedPacket->mLowLevelSignature)
	{
		case lowlevel::HighLevelPacket::SIGNATURE:
		default:
		{
			// Generic high-level packet
			ReceivedPacketEvaluation evaluation(*this, extracted.mPacketHeader.mPacketType, newSerializer);
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
					request.mHasResponse = true;

					// Also inform the listener
					ReceivedRequestEvaluation evaluation(*this, request);
					mConnectionManager->getListener().onReceivedRequestResponse(evaluation);
				}
				else
				{
					// TODO: Signal to the request that it failed
				}
				mOpenRequests.erase(it);
			}
			break;
		}
	}

	// The packet can be returned now
	extracted.mReceivedPacket->returnToDump();
}
