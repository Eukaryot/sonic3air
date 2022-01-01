/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/internal/ReceivedPacketCache.h"
#include "oxygen_netcore/network/LowLevelPackets.h"


ReceivedPacketCache::~ReceivedPacketCache()
{
	clear();
}

void ReceivedPacketCache::clear()
{
	// Remove references to all received packets still in the queue
	for (CacheItem& item : mQueue)
	{
		item.mReceivedPacket->decReferenceCounter();
	}
	mQueue.clear();
	mLastExtractedUniquePacketID = 0;
}

bool ReceivedPacketCache::enqueuePacket(ReceivedPacket& receivedPacket, const lowlevel::HighLevelPacket& packet, VectorBinarySerializer& serializer, uint32 uniqueRequestID)
{
	ReceivedPacketCache::CacheItem* itemToFill = nullptr;

	// Check the unique packet ID
	const uint32 uniquePacketID = packet.mUniquePacketID;
	const uint32 lastEnqueuedPacketID = mLastExtractedUniquePacketID + (uint32)mQueue.size();
	if (uniquePacketID <= lastEnqueuedPacketID)
	{
		if (uniquePacketID <= mLastExtractedUniquePacketID)
		{
			// It was extracted previously already
			return false;
		}

		// It's part of the queue, find it
		const uint32 firstQueueIndex = mLastExtractedUniquePacketID + 1;
		const size_t queueIndex = (size_t)(uniquePacketID - firstQueueIndex);
		if (nullptr != mQueue[queueIndex].mReceivedPacket)
		{
			// It already exists in the cache
			return false;
		}

		// Replace the empty spot with the received packet
		itemToFill = &mQueue[queueIndex];
	}
	else
	{
		// Is it the next packet that we're just waiting for?
		uint32 indexAfterQueue = uniquePacketID - (lastEnqueuedPacketID + 1);
		if (indexAfterQueue != 0)
		{
			// Create a gap...
			// TODO: Check if that would create a very large gap
			for (size_t k = 0; k < indexAfterQueue; ++k)
			{
				mQueue.emplace_back();
			}
		}

		// Enqueue the packet content
		mQueue.emplace_back();
		itemToFill = &mQueue.back();
	}

	// Fill the cache item accordingly
	itemToFill->mPacketHeader = packet;
	itemToFill->mUniqueRequestID = uniqueRequestID;
	itemToFill->mHeaderSize = serializer.getReadPosition();
	itemToFill->mReceivedPacket = &receivedPacket;

	// Retain packet while it's referenced in the queue
	receivedPacket.incReferenceCounter();

	// Now the packet was enqueued, and we can't do more here right now
	return true;
}

bool ReceivedPacketCache::extractPacket(CacheItem& outExtractionResult)
{
	if (mQueue.empty())
		return false;

	CacheItem& item = mQueue.front();
	if (nullptr == item.mReceivedPacket)
		return false;

	RMX_ASSERT(item.mPacketHeader.mUniquePacketID == mLastExtractedUniquePacketID + 1, "Detected a mismatch in unique packet IDs");
	// Note that the reference counter does not get decreased here, the caller of this method takes over the reference
	outExtractionResult = item;

	mQueue.pop_front();
	++mLastExtractedUniquePacketID;
	return true;
}
