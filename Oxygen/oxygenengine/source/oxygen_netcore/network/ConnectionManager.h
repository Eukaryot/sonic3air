/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"

class NetConnection;


class ConnectionManager
{
friend class NetConnection;

public:
	struct ReceivedPacket
	{
		std::vector<uint8> mContent;
		uint16 mLowLevelSignature = 0;
		SocketAddress mSenderAddress;
		NetConnection* mConnection = nullptr;
	};

public:
	explicit ConnectionManager(UDPSocket& socket);

	inline UDPSocket& getSocket() const  { return mSocket; }

	bool updateReceivePackets();	// TODO: This is meant to be executed by a thread later on

	void syncPacketQueues();

	inline bool hasAnyPacket() const  { return !mReceivedPackets.mSyncedQueue.empty(); }
	const ReceivedPacket* getNextPacket();

	bool hasConnectionTo(uint64 senderKey) const;

protected:
	// Only meant to be called from the NetConnection
	void addConnection(NetConnection& connection);
	void removeConnection(NetConnection& connection);

private:
	struct SyncedPacketQueue
	{
		std::deque<ReceivedPacket*> mWorkerQueue;	// Used by the worker thread that adds packets
		std::deque<ReceivedPacket*> mSyncedQueue;	// Used by the main thread that reads packets
		std::vector<ReceivedPacket*> mToBeReturned;
	};

private:
	UDPSocket& mSocket;

	std::unordered_map<uint16, NetConnection*> mActiveConnections;		// Using local connection ID as key
	std::unordered_map<uint64, NetConnection*> mConnectionsBySender;	// Using a sender key (= hash for the sender address + remote connection ID) as key
	SyncedPacketQueue mReceivedPackets;

	RentableObjectPool<ReceivedPacket> mReceivedPacketPool;
};
