/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/LagStopwatch.h"
#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/NetConnection.h"


namespace
{
	static const constexpr VersionRange<uint8> LOWLEVEL_PROTOCOL_VERSION_RANGE { 1, 1 };

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


uint64 ServerClientBase::getCurrentTimestamp()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool ServerClientBase::updateReceivePackets(ConnectionManager& connectionManager)
{
	// Check the socket for new packets
	{
		LAG_STOPWATCH("connectionManager.updateReceivePackets", 2000);
		if (!connectionManager.updateReceivePackets())
			return false;
	}

	{
		LAG_STOPWATCH("connectionManager.syncPacketQueues", 2000);
		connectionManager.syncPacketQueues();
	}

	// Handle incoming TCP connections
	{
		LAG_STOPWATCH("TCP connections", 2000);
		std::list<TCPSocket>& incomingTCPConnections = connectionManager.getIncomingTCPConnections();
		while (!incomingTCPConnections.empty())
		{
			// Create a new connection
			NetConnection* connection = createNetConnection(connectionManager, SocketAddress());	// TODO: Fill in the actual sender address, in case some implementation wants to use it
			if (nullptr != connection)
			{
				// Setup connection
				connection->setupWithTCPSocket(connectionManager, incomingTCPConnections.front(), getCurrentTimestamp());
			}
			incomingTCPConnections.pop_front();
		}
	}

	// Handle incoming packets
	{
		LAG_STOPWATCH("Incoming packets", 2000);
		while (connectionManager.hasAnyPacket())
		{
			ReceivedPacket* receivedPacket = connectionManager.getNextReceivedPacket();
			if (nullptr == receivedPacket)
			{
				// Handled all current packets
				break;
			}

			if (receivedPacket->mLowLevelSignature == lowlevel::StartConnectionPacket::SIGNATURE)
			{
				// It's a connection start request
				LAG_STOPWATCH("handleConnectionStartPacket", 500);
				handleConnectionStartPacket(connectionManager, *receivedPacket);
			}
			else
			{
				// It's packet for a connection
				RMX_ASSERT(nullptr != receivedPacket->mConnection, "Expected an assigned connection");
				LAG_STOPWATCH("handleLowLevelPacket", 500);
				receivedPacket->mConnection->handleLowLevelPacket(*receivedPacket);
			}

			// Return the packet if nobody increased the reference counter meanwhile
			//  -> E.g. the ReceivedPacketCache does this when adding a high-level packet into its queue of pending packets
			LAG_STOPWATCH("decReferenceCounter", 500);
			receivedPacket->decReferenceCounter();
		}
	}

	return true;
}

void ServerClientBase::handleConnectionStartPacket(ConnectionManager& connectionManager, const ReceivedPacket& receivedPacket)
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
	const ProtocolVersionChecker::Result resultHighLevel = ProtocolVersionChecker::chooseVersion(highLevelVersion, connectionManager.getHighLevelProtocolVersionRange(), packet.mHighLevelProtocolVersionRange);
	if (resultLowLevel != ProtocolVersionChecker::Result::SUCCESS || resultHighLevel != ProtocolVersionChecker::Result::SUCCESS)
	{
		// Send back an error
		const bool localIsTooOld = (resultLowLevel == ProtocolVersionChecker::Result::ERROR_TOO_NEW || resultHighLevel == ProtocolVersionChecker::Result::ERROR_TOO_NEW);
		lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::UNSUPPORTED_VERSION, localIsTooOld ? 1 : 0);
		connectionManager.sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
		return;
	}

	// In case of a TCP connection, the NetConnection pointer was already set
	NetConnection* connection = receivedPacket.mConnection;
	if (nullptr != connection)
	{
		// Setup connection
		connection->setProtocolVersions(lowLevelVersion, highLevelVersion);
		connection->acceptIncomingConnectionTCP(connectionManager, remoteConnectionID, getCurrentTimestamp());
	}
	else
	{
		const uint64 senderKey = NetConnection::buildSenderKey(receivedPacket.mSenderAddress, remoteConnectionID);
		connection = connectionManager.findConnectionTo(senderKey);
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
			connection->acceptIncomingConnectionUDP(connectionManager, remoteConnectionID, receivedPacket.mSenderAddress, senderKey, getCurrentTimestamp());
		}
		else
		{
			// Send back an error
			lowlevel::ErrorPacket errorPacket(lowlevel::ErrorPacket::ErrorCode::CONNECTION_INVALID);
			connectionManager.sendConnectionlessLowLevelPacket(errorPacket, receivedPacket.mSenderAddress, 0, remoteConnectionID);
		}
	}
}
