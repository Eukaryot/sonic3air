/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"

namespace lowlevel
{
	struct PacketBase;
}
class ConnectionManager;


// UDP-based virtual connection
class NetConnection
{
public:
	enum class State
	{
		EMPTY,
		REQUESTED_CONNECTION,
		CONNECTED,
		DISCONNECTED
	};

public:
	static uint64 buildSenderKey(const SocketAddress& remoteAddress, uint16 remoteConnectionID);

public:
	inline State getState() const    { return mState; }
	inline uint16 getLocalConnectionID() const  { return mLocalConnectionID; }
	inline uint16 getRemoteConnectionID() const { return mRemoteConnectionID; }
	inline const SocketAddress& getRemoteAddress() const  { return mRemoteAddress; }
	inline uint64 getSenderKey() const  { return mSenderKey; }

	UDPSocket* getSocket() const;

	bool startConnectTo(ConnectionManager& connectionManager, uint16 localConnectionID, uint16 highLevelProtocolVersion, const SocketAddress& remoteAddress);
	void acceptIncomingConnection(ConnectionManager& connectionManager, uint16 localConnectionID, uint16 remoteConnectionID, const SocketAddress& remoteAddress, uint64 senderKey);

	void handleLowLevelPacket(uint16 lowLevelSignature, VectorBinarySerializer& serializer);

//	void send(const PacketBase& packet);

	void updateConnection(float timeElapsed);

private:
	bool sendLowLevelPacket(lowlevel::PacketBase& packet);

private:
	State mState = State::EMPTY;
	ConnectionManager* mConnectionManager = nullptr;
	uint16 mLocalConnectionID = 0;
	uint16 mRemoteConnectionID = 0;
	SocketAddress mRemoteAddress;
	uint64 mSenderKey = 0;

	float mTimeSinceLastMessageSent = 0.0f;
	float mTimeSinceLastMessageReceived = 0.0f;

	std::vector<uint8> mSendBuffer;
};


class ConnectionIDProvider
{
public:
	inline uint16 getNextID()
	{
		const uint16 result = mNextID;
		++mNextID;
		if (mNextID == 0)	// Skip the zero
			++mNextID;
		return result;
	}

private:
	uint16 mNextID = 1;
};
