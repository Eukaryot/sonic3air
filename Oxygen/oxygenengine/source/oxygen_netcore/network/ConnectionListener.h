/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/NetConnection.h"

namespace highlevel
{
	class RequestBase;
}


// The following are helper structs that serve two main purposes:
//  - They combine multiple parameters into one struct, which makes it easier to later add or modify which parameters are passed on
//     -> They could also be used to return more information (than just the bool return value) from the listener to the caller
//  - They also feature some easy-of-use functions to simplify the code in the listener implementation


struct ConnectionlessPacketEvaluation
{
	ConnectionManager& mConnectionManager;
	SocketAddress mSenderAddress;
	uint16 mLowLevelSignature = 0;
	VectorBinarySerializer& mSerializer;

	inline ConnectionlessPacketEvaluation(ConnectionManager& connectionManager, const SocketAddress& senderAddress, uint16 lowLevelSignature, VectorBinarySerializer& serializer) :
		mConnectionManager(connectionManager), mSenderAddress(senderAddress), mLowLevelSignature(lowLevelSignature), mSerializer(serializer)
	{}
};


struct ReceivedPacketEvaluation
{
	NetConnection& mConnection;
	uint32 mPacketType;
	uint32 mUniquePacketID;
	VectorBinarySerializer& mSerializer;

	inline ReceivedPacketEvaluation(NetConnection& connection, uint32 packetType, VectorBinarySerializer& serializer, uint32 uniquePacketID) :
		mConnection(connection), mPacketType(packetType), mSerializer(serializer), mUniquePacketID(uniquePacketID)
	{}

	bool readPacket(highlevel::PacketBase& packet) const
	{
		return mConnection.readPacket(packet, mSerializer);
	}
};


struct ReceivedQueryEvaluation
{
	NetConnection& mConnection;
	uint32 mPacketType;
	VectorBinarySerializer& mSerializer;
	uint32 mUniqueRequestID;

	inline ReceivedQueryEvaluation(NetConnection& connection, uint32 packetType, VectorBinarySerializer& serializer, uint32 uniqueRequestID) :
		mConnection(connection), mPacketType(packetType), mSerializer(serializer), mUniqueRequestID(uniqueRequestID)
	{}

	template<typename T>
	bool readQuery(T& request) const
	{
		return mConnection.readPacket(request.mQuery, mSerializer);
	}

	bool respond(highlevel::RequestBase& request) const
	{
		return mConnection.respondToRequest(request, mUniqueRequestID);
	}
};


struct ReceivedRequestEvaluation
{
	NetConnection& mConnection;
	highlevel::RequestBase& mRequest;

	inline ReceivedRequestEvaluation(NetConnection& connection, highlevel::RequestBase& request) :
		mConnection(connection), mRequest(request)
	{}
};


struct ConnectionListenerInterface
{
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) = 0;
	virtual void destroyNetConnection(NetConnection& connection) = 0;

	virtual bool onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation)	{ return false; }
	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation)						{ return false; }
	virtual bool onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation)				{ return false; }
	virtual void onReceivedRequestResponse(ReceivedRequestEvaluation& evaluation)			{}
	virtual void onReceivedRequestError(ReceivedRequestEvaluation& evaluation)				{}
};
