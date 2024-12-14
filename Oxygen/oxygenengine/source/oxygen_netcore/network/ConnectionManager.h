/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/internal/SentPacketCache.h"
#include "oxygen_netcore/network/internal/ReceivedPacket.h"
#include "oxygen_netcore/network/VersionRange.h"
#include "oxygen_netcore/base/HandleProvider.h"

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
	static uint64 getCurrentTimestamp();

public:
	ConnectionManager(UDPSocket* udpSocket, TCPSocket* tcpListenSocket, ConnectionListenerInterface& listener, VersionRange<uint8> highLevelProtocolVersionRange);

	inline bool hasUDPSocket() const			  { return (nullptr != mUDPSocket); }
	inline UDPSocket* getUDPSocket() const		  { return mUDPSocket; }
	inline TCPSocket* getTCPListenSocket() const  { return mTCPListenSocket; }

	inline ConnectionListenerInterface& getListener() const  { return mListener; }
	inline size_t getNumActiveConnections() const			 { return mActiveConnections.size(); }

	inline VersionRange<uint8> getHighLevelProtocolVersionRange() const  { return mHighLevelProtocolVersionRange; }

	bool updateConnectionManager();

	bool sendUDPPacketData(const std::vector<uint8>& data, const SocketAddress& remoteAddress);
	bool sendTCPPacketData(const std::vector<uint8>& data, TCPSocket& socket, bool isWebSocketServer);

	bool sendConnectionlessLowLevelPacket(lowlevel::PacketBase& lowLevelPacket, const SocketAddress& remoteAddress, uint16 localConnectionID, uint16 remoteConnectionID);

protected:
	// Only meant to be called from NetConnection
	void addConnection(NetConnection& connection);
	void removeConnection(NetConnection& connection);
	SentPacket& rentSentPacket();

	void receivedPacketInternal(const std::vector<uint8>& buffer, const SocketAddress& senderAddress, NetConnection* connection);

private:
	struct SyncedPacketQueue
	{
		std::deque<ReceivedPacket*> mWorkerQueue;	// Used by the worker thread that adds packets
		std::deque<ReceivedPacket*> mSyncedQueue;	// Used by the main thread that reads packets
		ReceivedPacket::Dump mToBeReturned;
	};

	typedef HandleProvider<uint16, NetConnection*, 16> ConnectionsProvider;

private:
	void updateConnections(uint64 currentTimestamp);
	bool updateReceivePacketsInternal();	// TODO: This is meant to be executed by a thread later on

	void syncPacketQueues();

	inline bool hasAnyPacket() const  { return !mReceivedPackets.mSyncedQueue.empty(); }
	ReceivedPacket* getNextReceivedPacket();
	std::list<TCPSocket>& getIncomingTCPConnections()  { return mIncomingTCPConnections; }

	void handleReceivedPacket(const ReceivedPacket& receivedPacket);
	void handleStartConnectionPacket(const ReceivedPacket& receivedPacket);
	NetConnection* findConnectionTo(uint64 senderKey) const;

private:
	UDPSocket* mUDPSocket = nullptr;		// Only set if UDP is used (or both UDP and TCP)
	TCPSocket* mTCPListenSocket = nullptr;	// Only set if TCP is used (or both UDP and TCP)
	ConnectionListenerInterface& mListener;
	VersionRange<uint8> mHighLevelProtocolVersionRange = { 1, 1 };

	std::unordered_map<uint16, NetConnection*> mActiveConnections;		// Using local connection ID as key
	std::unordered_map<uint64, NetConnection*> mConnectionsBySender;	// Using a sender key (= hash for the sender address + remote connection ID) as key
	ConnectionsProvider mConnectionsProvider;

	std::vector<NetConnection*> mTCPNetConnections;

	SyncedPacketQueue mReceivedPackets;
	std::list<TCPSocket> mIncomingTCPConnections;

	RentableObjectPool<SentPacket> mSentPacketPool;
	RentableObjectPool<ReceivedPacket> mReceivedPacketPool;
};
