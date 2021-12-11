/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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


struct ReceivedPacketEvaluation
{
	NetConnection& mConnection;
	uint32 mPacketType;
	VectorBinarySerializer& mSerializer;

	inline ReceivedPacketEvaluation(NetConnection& connection, uint32 packetType, VectorBinarySerializer& serializer) :
		mConnection(connection), mPacketType(packetType), mSerializer(serializer)
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
	virtual void onReceivedPacket(ReceivedPacketEvaluation& evaluation) = 0;
	virtual bool onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation) { return false; }
	virtual void onReceivedRequestResponse(ReceivedRequestEvaluation& evaluation) {}
};
