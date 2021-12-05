/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/NetConnection.h"


namespace
{
	static const constexpr uint16 LOWLEVEL_PROTOCOL_MINIMUM_VERSION = 1;
	static const constexpr uint16 LOWLEVEL_PROTOCOL_MAXIMUM_VERSION = 1;
}


uint16 ServerClientBase::ConnectionIDProvider::getNextID()
{
	const uint16 result = mNextID;
	++mNextID;
	if (mNextID == 0)	// Skip the zero
		++mNextID;
	return result;
}


bool ServerClientBase::updateReceivePackets(ConnectionManager& connectionManager)
{
	// Check the socket for new packets
	if (!connectionManager.updateReceivePackets())
	{
		return false;
	}

	connectionManager.syncPacketQueues();

	// Handle incoming packets
	while (connectionManager.hasAnyPacket())
	{
		const ConnectionManager::ReceivedPacket* receivedPacket = connectionManager.getNextPacket();
		if (nullptr == receivedPacket)
			break;

		VectorBinarySerializer serializer(true, receivedPacket->mContent);
		serializer.skip(2);		// Skip low level signature, it can be found in "ReceivedPacket::mLowLevelSignature"
		if (nullptr == receivedPacket->mConnection)
		{
			// It's a connection start request
			const uint16 remoteConnectionID = serializer.read<uint16>();
			serializer.skip(2);		// Skip the other connection ID, as it's invalid anyways (it's only in there to have all low-level packets share the same header)

			lowlevel::StartConnectionPacket packet;
			if (packet.serializeContent(serializer))
			{
				if (packet.mLowLevelProtocolVersion < LOWLEVEL_PROTOCOL_MINIMUM_VERSION || packet.mLowLevelProtocolVersion > LOWLEVEL_PROTOCOL_MAXIMUM_VERSION)
				{
					// TODO: Handle unsupported protocol version
					continue;
				}

				const uint64 senderKey = NetConnection::buildSenderKey(receivedPacket->mSenderAddress, remoteConnectionID);
				if (connectionManager.hasConnectionTo(senderKey))
				{
					// Ignore packet, we already have a connection established
					continue;
				}

				// Create a new connection
				const uint16 localConnectionID = mConnectionIDProvider.getNextID();
				NetConnection* connection = createNetConnection(connectionManager, receivedPacket->mSenderAddress);
				if (nullptr != connection)
				{
					connection->acceptIncomingConnection(connectionManager, localConnectionID, remoteConnectionID, receivedPacket->mSenderAddress, senderKey);
				}
				else
				{
					// TODO: Send back an error
				}
			}
		}
		else
		{
			// It's packet for a connection
			serializer.skip(4);		// Skip the connection IDs, they got evaluated already
			if (serializer.getRemaining() > 0)
			{
				receivedPacket->mConnection->handleLowLevelPacket(receivedPacket->mLowLevelSignature, serializer);
			}
		}
	}

	return true;
}
