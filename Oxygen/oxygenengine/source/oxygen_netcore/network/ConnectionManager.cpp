/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/NetConnection.h"


ConnectionManager::ConnectionManager(UDPSocket& socket, ConnectionListenerInterface& listener, uint8 highLevelMinimumProtocolVersion, uint8 highLevelMaximumProtocolVersion) :
	mSocket(socket),
	mListener(listener),
	mHighLevelMinimumProtocolVersion(highLevelMinimumProtocolVersion),
	mHighLevelMaximumProtocolVersion(highLevelMaximumProtocolVersion)
{
}

void ConnectionManager::addConnection(NetConnection& connection)
{
	mActiveConnections[connection.getLocalConnectionID()] = &connection;
	mConnectionsBySender[connection.getSenderKey()] = &connection;
}

void ConnectionManager::removeConnection(NetConnection& connection)
{
	mActiveConnections.erase(connection.getLocalConnectionID());
	mConnectionsBySender.erase(connection.getSenderKey());

	// TODO: Remove all queued packets for this connection
}

void ConnectionManager::updateConnections(uint64 currentTimestamp)
{
	// TODO: An unordered_map is not really optimal for iterating
	for (auto& pair : mActiveConnections)
	{
		pair.second->updateConnection(currentTimestamp);
	}
}

bool ConnectionManager::updateReceivePackets()
{
	for (int runs = 0; runs < 10; ++runs)
	{
		// Receive next packet
		static UDPSocket::ReceiveResult received;
		const bool success = mSocket.receiveNonBlocking(received);
		if (!success)
		{
			// TODO: Handle error in socket
			return false;
		}

		if (received.mBuffer.empty())
		{
			// Nothing to do at the moment; tell the caller if any packet was received at all
			return (runs > 0);
		}

		// Ignore too small packets
		if (received.mBuffer.size() < 6)
			continue;

		// Received a packet, check its signature
		VectorBinarySerializer serializer(true, received.mBuffer);
		const uint16 lowLevelSignature = serializer.read<uint16>();
		if (lowLevelSignature == lowlevel::StartConnectionPacket::SIGNATURE)
		{
			// Store for later evaluation
			ReceivedPacket& receivedPacket = mReceivedPacketPool.rentObject();
			receivedPacket.mContent = received.mBuffer;
			receivedPacket.mLowLevelSignature = lowLevelSignature;
			receivedPacket.mSenderAddress = received.mSenderAddress;
			receivedPacket.mConnection = nullptr;
			mReceivedPackets.mWorkerQueue.emplace_back(&receivedPacket);
		}
		else
		{
			// TODO: Explicitly check the known = valid signature types here?

			const uint16 remoteConnectionID = serializer.read<uint16>();
			const uint16 localConnectionID = serializer.read<uint16>();
			const auto it = mActiveConnections.find(localConnectionID);
			if (it == mActiveConnections.end())
			{
				// Unknown connection
				// TODO: Send back an error packet - or better enqueue a notice to send one (if this method is executed by a thread)
			}
			else
			{
				NetConnection& connection = *it->second;
				if (connection.getRemoteConnectionID() != remoteConnectionID && lowLevelSignature != lowlevel::AcceptConnectionPacket::SIGNATURE)
				{
					// Unknown connection
					// TODO: Send back an error packet - or better enqueue a notice to send one (if this method is executed by a thread)
				}
				else
				{
					// Store for later evaluation
					ReceivedPacket& receivedPacket = mReceivedPacketPool.rentObject();
					receivedPacket.mContent = received.mBuffer;
					receivedPacket.mLowLevelSignature = lowLevelSignature;
					receivedPacket.mSenderAddress = received.mSenderAddress;
					receivedPacket.mConnection = &connection;
					mReceivedPackets.mWorkerQueue.emplace_back(&receivedPacket);
				}
			}
		}
	}
	return true;
}

void ConnectionManager::syncPacketQueues()
{
	// TODO: Lock mutex, so the worker thread stops briefly
	//  -> This is needed for the worker queue, and also for the "mReceivedPacketPool"

	// First sync queues
	{
		while (!mReceivedPackets.mWorkerQueue.empty())
		{
			mReceivedPackets.mSyncedQueue.push_back(mReceivedPackets.mWorkerQueue.front());
			mReceivedPackets.mWorkerQueue.pop_front();
		}
	}

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

	// Set the "dump" instance that packets get returned to when they have been processed completely
	receivedPacket->setDump(&mReceivedPackets.mToBeReturned);
	return receivedPacket;
}

NetConnection* ConnectionManager::findConnectionTo(uint64 senderKey) const
{
	const auto it = mConnectionsBySender.find(senderKey);
	return (it == mConnectionsBySender.end()) ? nullptr : it->second;
}
