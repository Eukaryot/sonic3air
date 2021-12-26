/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/internal/SentPacketCache.h"
#include "oxygen_netcore/network/internal/ReceivedPacket.h"
#include "oxygen_netcore/network/VersionRange.h"

namespace lowlevel
{
	struct PacketBase;
}
struct ConnectionListenerInterface;


class ConnectionManager
{
friend class NetConnection;

public:
	struct DebugSettings
	{
		float mSendingPacketLoss = 0.0f;	// Fraction of "lost" packets in sending
		float mReceivingPacketLoss = 0.0f;	// Fraction of "lost" packets in receiving
	};
	DebugSettings mDebugSettings;

public:
	ConnectionManager(UDPSocket& socket, ConnectionListenerInterface& listener, VersionRange<uint8> highLevelProtocolVersionRange);

	inline UDPSocket& getSocket() const  { return mSocket; }
	inline ConnectionListenerInterface& getListener() const  { return mListener; }
	inline size_t getNumActiveConnections() const			 { return mActiveConnections.size(); }

	inline VersionRange<uint8> getHighLevelProtocolVersionRange() const  { return mHighLevelProtocolVersionRange; }

	void updateConnections(uint64 currentTimestamp);
	bool updateReceivePackets();	// TODO: This is meant to be executed by a thread later on

	void syncPacketQueues();

	inline bool hasAnyPacket() const  { return !mReceivedPackets.mSyncedQueue.empty(); }
	ReceivedPacket* getNextReceivedPacket();

	bool sendPacketData(const std::vector<uint8>& data, const SocketAddress& remoteAddress);
	bool sendConnectionlessLowLevelPacket(lowlevel::PacketBase& lowLevelPacket, const SocketAddress& remoteAddress, uint16 localConnectionID, uint16 remoteConnectionID);

	NetConnection* findConnectionTo(uint64 senderKey) const;

protected:
	// Only meant to be called from the NetConnection
	void addConnection(NetConnection& connection);
	void removeConnection(NetConnection& connection);
	SentPacket& rentSentPacket();

	// Internal
	uint16 getFreeLocalConnectionID();

private:
	struct SyncedPacketQueue
	{
		std::deque<ReceivedPacket*> mWorkerQueue;	// Used by the worker thread that adds packets
		std::deque<ReceivedPacket*> mSyncedQueue;	// Used by the main thread that reads packets
		ReceivedPacket::Dump mToBeReturned;
	};

private:
	UDPSocket& mSocket;
	ConnectionListenerInterface& mListener;
	VersionRange<uint8> mHighLevelProtocolVersionRange = { 1, 1 };

	std::unordered_map<uint16, NetConnection*> mActiveConnections;		// Using local connection ID as key
	std::vector<NetConnection*> mActiveConnectionsLookup;				// Using the lowest n bits of the local connection ID as index
	uint16 mBitmaskForActiveConnectionsLookup = 0;
	std::unordered_map<uint64, NetConnection*> mConnectionsBySender;	// Using a sender key (= hash for the sender address + remote connection ID) as key
	SyncedPacketQueue mReceivedPackets;

	RentableObjectPool<SentPacket> mSentPacketPool;
	RentableObjectPool<ReceivedPacket> mReceivedPacketPool;
};
