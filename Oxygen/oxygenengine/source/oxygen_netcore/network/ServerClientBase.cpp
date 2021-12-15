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

#include <chrono>


namespace
{
	static const constexpr uint16 LOWLEVEL_PROTOCOL_MINIMUM_VERSION = 1;
	static const constexpr uint16 LOWLEVEL_PROTOCOL_MAXIMUM_VERSION = 1;

	struct ProtocolVersionChecker
	{
		enum class Result
		{
			SUCCESS,		// Found a protocol version both can support
			ERROR_TOO_OLD,	// Remote protocol version is lower than anything we can support
			ERROR_TOO_NEW	// Remote protocol version is higher than anything we can support
		};

		static Result chooseVersion(uint8& outVersion, uint8 localMinVersion, uint8 localMaxVersion, uint8 remoteMinVersion, uint8 remoteMaxVersion)
		{
			// Use the highest version both can support
			outVersion = std::min(remoteMaxVersion, localMaxVersion);
			if (outVersion < localMinVersion)
			{
				return Result::ERROR_TOO_OLD;
			}
			else if (outVersion < remoteMinVersion)
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


uint64 ServerClientBase::getCurrentTimestamp()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool ServerClientBase::updateReceivePackets(ConnectionManager& connectionManager)
{
	// Check the socket for new packets
	if (!connectionManager.updateReceivePackets())
		return false;

	connectionManager.syncPacketQueues();

	// Handle incoming packets
	while (connectionManager.hasAnyPacket())
	{
		ReceivedPacket* receivedPacket = connectionManager.getNextReceivedPacket();
		if (nullptr == receivedPacket)
		{
			// Handled all current packets
			break;
		}

		if (nullptr == receivedPacket->mConnection)
		{
			// It's a connection start request
			handleConnectionStartPacket(connectionManager, *receivedPacket);
		}
		else
		{
			// It's packet for a connection
			receivedPacket->mConnection->handleLowLevelPacket(*receivedPacket);
		}

		// Return the packet if nobody increased the reference counter meanwhile
		//  -> E.g. the ReceivedPacketCache does this when adding a high-level packet into its queue of pending packets
		receivedPacket->decReferenceCounter();
	}
	return true;
}

void ServerClientBase::handleConnectionStartPacket(ConnectionManager& connectionManager, const ReceivedPacket& receivedPacket)
{
	VectorBinarySerializer serializer(true, receivedPacket.mContent);
	serializer.skip(2);		// Skip low level signature, it can be found in "ReceivedPacket::mLowLevelSignature"
	const uint16 remoteConnectionID = serializer.read<uint16>();
	serializer.skip(2);		// Skip the other connection ID, as it's invalid anyways (it's only in there to have all low-level packets share the same header)

	lowlevel::StartConnectionPacket packet;
	if (!packet.serializePacket(serializer, LOWLEVEL_PROTOCOL_MINIMUM_VERSION))
	{
		// Error in packet format, just ignore packet
		return;
	}

	uint8 lowLevelVersion = 0;
	uint8 highLevelVersion = 0;
	const ProtocolVersionChecker::Result resultLowLevel = ProtocolVersionChecker::chooseVersion(lowLevelVersion, LOWLEVEL_PROTOCOL_MINIMUM_VERSION, LOWLEVEL_PROTOCOL_MAXIMUM_VERSION, packet.mLowLevelMinimumProtocolVersion, packet.mLowLevelMaximumProtocolVersion);
	const ProtocolVersionChecker::Result resultHighLevel = ProtocolVersionChecker::chooseVersion(highLevelVersion, connectionManager.getHighLevelMinimumProtocolVersion(), connectionManager.getHighLevelMaximumProtocolVersion(), packet.mHighLevelMinimumProtocolVersion, packet.mHighLevelMaximumProtocolVersion);
	if (resultLowLevel != ProtocolVersionChecker::Result::SUCCESS || resultHighLevel != ProtocolVersionChecker::Result::SUCCESS)
	{
		// Send back an error
		const bool localIsTooOld = (resultLowLevel == ProtocolVersionChecker::Result::ERROR_TOO_NEW || resultHighLevel == ProtocolVersionChecker::Result::ERROR_TOO_NEW);
		lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::UNSUPPORTED_VERSION, localIsTooOld ? 1 : 0);
		connectionManager.sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
		return;
	}

	const uint64 senderKey = NetConnection::buildSenderKey(receivedPacket.mSenderAddress, remoteConnectionID);
	NetConnection* connection = connectionManager.findConnectionTo(senderKey);
	if (nullptr != connection)
	{
		if (connection->isConnectedTo(connection->getLocalConnectionID(), remoteConnectionID, senderKey))
		{
			// Resend the AcceptConnectionPacket, as it seems the last one got lost
			connection->sendAcceptConnectionPacket();
			return;
		}

		// Destroy that old connection
		connection->clear();
		destroyNetConnection(*connection);
	}
	else
	{
		// Check if another connection would reach the limit of concurrent connections
		if (connectionManager.getNumActiveConnections() + 1 >= 0x100)	// Not a hard limit, but should be good enough for now
		{
			lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::TOO_MANY_CONNECTIONS);
			connectionManager.sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
			return;
		}
	}

	// Create a new connection
	connection = createNetConnection(connectionManager, receivedPacket.mSenderAddress);
	if (nullptr != connection)
	{
		// Setup connection
		connection->setProtocolVersions(lowLevelVersion, highLevelVersion);
		connection->acceptIncomingConnection(connectionManager, remoteConnectionID, receivedPacket.mSenderAddress, senderKey);
	}
	else
	{
		// Send back an error
		lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::CONNECTION_INVALID);
		connectionManager.sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
	}
}
