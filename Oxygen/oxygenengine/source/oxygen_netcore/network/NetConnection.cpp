/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/LowLevelPackets.h"


uint64 NetConnection::buildSenderKey(const SocketAddress& remoteAddress, uint16 remoteConnectionID)
{
	return remoteAddress.getHash() ^ remoteConnectionID;
}

UDPSocket* NetConnection::getSocket() const
{
	return (nullptr == mConnectionManager) ? nullptr : &mConnectionManager->getSocket();
}

bool NetConnection::startConnectTo(ConnectionManager& connectionManager, uint16 localConnectionID, uint16 highLevelProtocolVersion, const SocketAddress& remoteAddress)
{
	if (nullptr != mConnectionManager)
	{
		mConnectionManager->removeConnection(*this);
		mConnectionManager = nullptr;
	}

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
		packet.mLowLevelProtocolVersion = 1;		// Low-level protocol version is for now fixed at 1
		packet.mHighLevelProtocolVersion = highLevelProtocolVersion;

		return sendLowLevelPacket(packet);
	}
}

void NetConnection::acceptIncomingConnection(ConnectionManager& connectionManager, uint16 localConnectionID, uint16 remoteConnectionID, const SocketAddress& remoteAddress, uint64 senderKey)
{
	if (nullptr != mConnectionManager)
	{
		mConnectionManager->removeConnection(*this);
		mConnectionManager = nullptr;
	}

	mState = State::CONNECTED;
	mConnectionManager = &connectionManager;
	mLocalConnectionID = localConnectionID;
	mRemoteConnectionID = remoteConnectionID;
	mRemoteAddress = remoteAddress;
	mSenderKey = senderKey;
	RMX_ASSERT(senderKey == buildSenderKey(mRemoteAddress, mRemoteConnectionID), "Previously calculated sender key is the wrong one");

	mConnectionManager->addConnection(*this);

	// Send back a response
	{
		std::cout << "Connection accepted from " << mRemoteAddress.toString() << std::endl;

		lowlevel::AcceptConnectionPacket packet;
		packet.mLowLevelProtocolVersion = 1;
		packet.mHighLevelProtocolVersion = 1;	// TODO
		sendLowLevelPacket(packet);
	}
}

void NetConnection::handleLowLevelPacket(uint16 lowLevelSignature, VectorBinarySerializer& serializer)
{
	switch (lowLevelSignature)
	{
		case lowlevel::AcceptConnectionPacket::SIGNATURE:
		{
			lowlevel::AcceptConnectionPacket packet;
			if (!packet.serializeContent(serializer))
				return;

			if (mState != State::REQUESTED_CONNECTION)
			{
				// Seems like something went wrong during establishing a connection, or maybe this was just a duplicate AcceptConnectionPacket
				return;
			}

			// TODO: Evaluate the packet versions

			std::cout << "Connection established to " << mRemoteAddress.toString() << std::endl;

			// Set remote connection ID, as it was not known before
			//  -> Unfortunately, we have to get in a somewhat awkward way, as it was skipped in deserialization before
			mRemoteConnectionID = *(uint16*)serializer.getBufferPointer(2);
			mSenderKey = buildSenderKey(mRemoteAddress, mRemoteConnectionID);
			mState = State::CONNECTED;
			return;
		}
	}
}

void NetConnection::updateConnection(float timeElapsed)
{
	// TODO: Check for timeout
	// TODO: Send a heartbeat every now and then

}

bool NetConnection::sendLowLevelPacket(lowlevel::PacketBase& packet)
{
	mSendBuffer.clear();
	VectorBinarySerializer serializer(false, mSendBuffer);

	// Write shared header for all low-level packets
	serializer.write(packet.getSignature());
	serializer.write(mLocalConnectionID);
	serializer.write(mRemoteConnectionID);

	packet.serializeContent(serializer);
	return getSocket()->sendData(mSendBuffer, mRemoteAddress);
}
