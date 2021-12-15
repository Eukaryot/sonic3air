/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/RequestBase.h"
#include "oxygen_netcore/network/internal/ReceivedPacketCache.h"
#include "oxygen_netcore/network/internal/SentPacketCache.h"

class ConnectionManager;


// UDP-based virtual connection
class NetConnection
{
friend class ServerClientBase;
friend class ConnectionManager;
friend class highlevel::RequestBase;

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
	virtual ~NetConnection();

	void clear();

	inline State getState() const				{ return mState; }
	inline uint16 getLocalConnectionID() const  { return mLocalConnectionID; }
	inline uint16 getRemoteConnectionID() const { return mRemoteConnectionID; }
	inline const SocketAddress& getRemoteAddress() const  { return mRemoteAddress; }
	inline uint64 getSenderKey() const			{ return mSenderKey; }

	UDPSocket* getSocket() const;

	uint8 getLowLevelProtocolVersion() const	{ return mLowLevelProtocolVersion; }
	uint8 getHighLevelProtocolVersion() const	{ return mHighLevelProtocolVersion; }
	void setProtocolVersions(uint8 lowLevelProtocolVersion, uint8 highLevelProtocolVersion);

	bool startConnectTo(ConnectionManager& connectionManager, const SocketAddress& remoteAddress);
	bool isConnectedTo(uint16 localConnectionID, uint16 remoteConnectionID, uint64 senderKey) const;
	void acceptIncomingConnection(ConnectionManager& connectionManager, uint16 remoteConnectionID, const SocketAddress& remoteAddress, uint64 senderKey);
	void sendAcceptConnectionPacket();

	bool sendPacket(highlevel::PacketBase& packet);
	bool sendRequest(highlevel::RequestBase& request);
	bool respondToRequest(highlevel::RequestBase& request, uint32 uniqueRequestID);

	bool readPacket(highlevel::PacketBase& packet, VectorBinarySerializer& serializer) const;

	void updateConnection(uint64 currentTimestamp);

private:
	void unregisterRequest(highlevel::RequestBase& request);

	bool sendPacketInternal(const std::vector<uint8>& content);
	void writeLowLevelPacketContent(VectorBinarySerializer& serializer, lowlevel::PacketBase& lowLevelPacket);
	bool sendLowLevelPacket(lowlevel::PacketBase& lowLevelPacket, std::vector<uint8>& buffer);
	bool sendHighLevelPacket(highlevel::PacketBase& packet, uint8 flags, uint32& outUniquePacketID);
	bool sendHighLevelPacket(lowlevel::HighLevelPacket& lowLevelPacket, highlevel::PacketBase& highLevelPacket, uint8 flags, uint32& outUniquePacketID);

	void handleLowLevelPacket(ReceivedPacket& receivedPacket);		// To be called by ServerClientBase
	void handleHighLevelPacket(ReceivedPacket& receivedPacket, const lowlevel::HighLevelPacket& highLevelPacket, VectorBinarySerializer& serializer, uint32 uniqueResponseID);
	void processExtractedHighLevelPacket(const ReceivedPacketCache::CacheItem& extracted);

private:
	State mState = State::EMPTY;
	ConnectionManager* mConnectionManager = nullptr;
	uint16 mLocalConnectionID = 0;
	uint16 mRemoteConnectionID = 0;
	SocketAddress mRemoteAddress;
	uint64 mSenderKey = 0;
	uint8 mLowLevelProtocolVersion = 0;
	uint8 mHighLevelProtocolVersion = 0;

	uint64 mCurrentTimestamp = 0;
	uint64 mLastMessageSentTimestamp = 0;
	uint64 mLastMessageReceivedTimestamp = 0;

	// Packet tracking
	SentPacketCache mSentPacketCache;
	ReceivedPacketCache mReceivedPacketCache;

	// Request tracking
	std::unordered_map<uint32, highlevel::RequestBase*> mOpenRequests;

	// For temporary use (these are members to avoid frequent reallocations)
	std::vector<uint8> mSendBuffer;
	std::vector<SentPacket*> mPacketsToResend;
};
