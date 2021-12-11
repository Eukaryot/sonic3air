/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/internal/ReceivedPacket.h"
#include "oxygen_netcore/network/LowLevelPackets.h"


class ReceivedPacketCache
{
public:
	struct CacheItem
	{
		lowlevel::HighLevelPacket mPacketHeader;
		uint32 mUniqueRequestID = 0;	// Only used for "lowlevel::RequestResponsePacket"
		size_t mHeaderSize = 0;
		ReceivedPacket* mReceivedPacket = nullptr;
	};

public:
	~ReceivedPacketCache();

	void clear();

	bool enqueuePacket(ReceivedPacket& receivedPacket, const lowlevel::HighLevelPacket& packet, VectorBinarySerializer& serializer, uint32 uniqueRequestID);
	bool extractPacket(CacheItem& outExtractionResult);

private:
	uint32 mLastExtractedUniquePacketID = 0;
	std::deque<CacheItem> mQueue;
};
