/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
	static const constexpr int TIMEOUT_SECONDS = 30;	// Timeout after 30 seconds without getting any response despite waiting for one
	static const constexpr int STALE_SECONDS = 5 * 60;	// Stale connection after 5 minutes if there was no communication at all in that time

	enum class State
	{
		EMPTY,					// Not connected in any way
		TCP_READY,				// Has a valid TCP socket, but not further setup yet
		REQUESTED_CONNECTION,	// Sent a StartConnectionPacket, no response yet (only used on client side)
		CONNECTED,				// Fully connected
		DISCONNECTED			// Connection was lost or intentionally disconnected (see DisconnectReason)
	};

	enum class DisconnectReason
	{
		UNKNOWN,	// No reason given
		MANUAL,		// Manually disconnected
		TIMEOUT,	// Automatic disconnect after timeout (because a reliably sent packet was not confirmed for too long)
		STALE,		// Automatic disconnect after connection was not used for a while
	};

	struct SendFlags
	{
		enum Flags
		{
			NONE		= 0x00,
			UNRELIABLE	= 0x01,		// Enforce sending as a lightweight packet, without resending and confirmation from the receiver
		};
	};

public:
	static uint64 buildSenderKey(const SocketAddress& remoteAddress, uint16 remoteConnectionID);

public:
	NetConnection();
	virtual ~NetConnection();

	void clear();

	inline State getState() const				{ return mState; }
	inline uint16 getLocalConnectionID() const  { return mLocalConnectionID; }
	inline uint16 getRemoteConnectionID() const { return mRemoteConnectionID; }
	inline const SocketAddress& getRemoteAddress() const  { return mRemoteAddress; }
	inline uint64 getSenderKey() const			{ return mSenderKey; }

	UDPSocket* getUDPSocket() const;

	uint8 getLowLevelProtocolVersion() const	{ return mLowLevelProtocolVersion; }
	uint8 getHighLevelProtocolVersion() const	{ return mHighLevelProtocolVersion; }
	void setProtocolVersions(uint8 lowLevelProtocolVersion, uint8 highLevelProtocolVersion);

	void setupWithTCPSocket(ConnectionManager& connectionManager, TCPSocket& socketToMove, uint64 currentTimestamp);
	bool startConnectTo(ConnectionManager& connectionManager, const SocketAddress& remoteAddress, uint64 currentTimestamp);
	bool isConnectedTo(uint16 localConnectionID, uint16 remoteConnectionID, uint64 senderKey) const;
	void disconnect(DisconnectReason disconnectReason = DisconnectReason::MANUAL);

	bool sendPacket(highlevel::PacketBase& packet, SendFlags::Flags flags = SendFlags::NONE);
	bool sendRequest(highlevel::RequestBase& request);
	bool respondToRequest(highlevel::RequestBase& request, uint32 uniqueRequestID);

	bool readPacket(highlevel::PacketBase& packet, VectorBinarySerializer& serializer) const;

	void updateConnection(uint64 currentTimestamp);

private:
	// Called by ServerClientBase
	void acceptIncomingConnectionUDP(ConnectionManager& connectionManager, uint16 remoteConnectionID, const SocketAddress& remoteAddress, uint64 senderKey, uint64 currentTimestamp);
	void acceptIncomingConnectionTCP(ConnectionManager& connectionManager, uint16 remoteConnectionID, uint64 currentTimestamp);
	void sendAcceptConnectionPacket();
	void handleLowLevelPacket(ReceivedPacket& receivedPacket);

	// Called by RequestBsae
	void unregisterRequest(highlevel::RequestBase& request);

	// Internal use
	bool sendPacketInternal(const std::vector<uint8>& content);
	void writeLowLevelPacketContent(VectorBinarySerializer& serializer, lowlevel::PacketBase& lowLevelPacket);
	bool sendLowLevelPacket(lowlevel::PacketBase& lowLevelPacket, std::vector<uint8>& buffer);
	bool sendHighLevelPacket(highlevel::PacketBase& packet, SendFlags::Flags flags, uint32& outUniquePacketID);
	bool sendHighLevelPacket(lowlevel::HighLevelPacket& lowLevelPacket, highlevel::PacketBase& highLevelPacket, SendFlags::Flags flags, uint32& outUniquePacketID);

	void handleHighLevelPacket(ReceivedPacket& receivedPacket, const lowlevel::HighLevelPacket& highLevelPacket, VectorBinarySerializer& serializer, uint32 uniqueResponseID);
	void processExtractedHighLevelPacket(const ReceivedPacketCache::CacheItem& extracted);

private:
	State mState = State::EMPTY;
	DisconnectReason mDisconnectReason = DisconnectReason::UNKNOWN;
	ConnectionManager* mConnectionManager = nullptr;
	bool mUsingTCP = false;
	TCPSocket mTCPSocket;	// Used only if "mUsingTCP == true"

	uint16 mLocalConnectionID = 0;
	uint16 mRemoteConnectionID = 0;
	SocketAddress mRemoteAddress;
	uint64 mSenderKey = 0;
	uint8 mLowLevelProtocolVersion = 0;
	uint8 mHighLevelProtocolVersion = 0;

	uint64 mCurrentTimestamp = 0;
	uint64 mLastMessageSentTimestamp = 0;
	uint64 mLastMessageReceivedTimestamp = 0;
	uint64 mTimeoutStart = 0;
	uint64 mLast100msUpdate = 0;
	uint64 mLast1000msUpdate = 0;

	// Packet tracking
	SentPacketCache mSentPacketCache;
	ReceivedPacketCache mReceivedPacketCache;

	// Request tracking
	std::unordered_map<uint32, highlevel::RequestBase*> mOpenRequests;

	// For temporary use (these are members to avoid frequent reallocations)
	std::vector<uint8> mSendBuffer;
	std::vector<SentPacket*> mPacketsToResend;
};
