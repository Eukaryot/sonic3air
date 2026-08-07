/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/internal/SentPacketCache.h"
#include "oxygen_netcore/network/LowLevelPackets.h"


void SentPacketCache::clear()
{
	for (SentPacket* sentPacket : mQueue)
	{
		if (nullptr != sentPacket)
			sentPacket->returnToPool();
	}
	mQueue.clear();
	mQueueStartUniquePacketID = 0;
	mNextUniquePacketID = 1;
}

uint32 SentPacketCache::getNextUniquePacketID() const
{
	return mNextUniquePacketID;
}

void SentPacketCache::addPacket(SentPacket& sentPacket, uint64 currentTimestamp, bool isStartConnectionPacket)
{
	// Special handling if this is the first packet added
	if (mQueueStartUniquePacketID == 0)
	{
		// First expected incoming unique packet ID can be 0 or 1:
		//  - On client side, ID 0 is the first one, as it's used for the StartConnectionPacket (parameter "isStartConnectionPacket" is true in that exact case)
		//  - On server side, no packet with ID 0 will be added, so we'd expect the first packet to use ID 1
		RMX_ASSERT(mNextUniquePacketID <= 1, "Unique packet ID differs from expected ID");

		const uint32 uniquePacketID = isStartConnectionPacket ? 0 : 1;
		mQueueStartUniquePacketID = uniquePacketID;
		mNextUniquePacketID = uniquePacketID;
	}
	else
	{
		RMX_ASSERT(!isStartConnectionPacket, "When adding a isStartConnectionPacket, it must be the first one in the cache");
	}

	sentPacket.mInitialTimestamp = currentTimestamp;
	sentPacket.mLastSendTimestamp = currentTimestamp;

	mQueue.push_back(&sentPacket);
	++mNextUniquePacketID;
}

bool SentPacketCache::wasPacketReceiveConfirmed(uint32 uniquePacketID) const
{
	// If the ID is not part of the queue any more, it counts as received
	if (uniquePacketID < mQueueStartUniquePacketID)
		return true;

	// If the ID is invalid, it's not received, obviously
	const size_t index = uniquePacketID - mQueueStartUniquePacketID;
	if (index >= mQueue.size())
		return false;

	return (nullptr == mQueue[index]);
}

void SentPacketCache::onPacketReceiveConfirmed(uint32 uniquePacketID)
{
	// If the ID is not part of the queue, ignore it
	if (uniquePacketID < mQueueStartUniquePacketID)
		return;
	const size_t index = uniquePacketID - mQueueStartUniquePacketID;
	if (index >= mQueue.size())
		return;

	// Also ignore if the packet already got confirmed
	if (nullptr == mQueue[index])
		return;

	mQueue[index]->returnToPool();
	mQueue[index] = nullptr;

	// Remove as many items from the queue as possible
	if (index == 0)
	{
		do
		{
			mQueue.pop_front();
			++mQueueStartUniquePacketID;
		}
		while (!mQueue.empty() && nullptr == mQueue.front());
	}
}

void SentPacketCache::updateResend(std::vector<SentPacket*>& outPacketsToResend, uint64 currentTimestamp)
{
	if (mQueue.empty())
		return;

	const uint64 minimumInitialTimestamp = currentTimestamp - 200;	// Start resending packets after 200 ms
	int timeBetweenResends = 200;
	size_t remainingPacketsToConsider = 5;

	// Check the first packet in the queue, i.e. the one that's waiting for the longest time
	//  -> That one determines how many packets in the queue are even considered for resending, and how long to wait between resends
	{
		SentPacket& sentPacket = *mQueue.front();
		if (sentPacket.mInitialTimestamp > minimumInitialTimestamp)
			return;

		if (sentPacket.mResendCounter < 5)
		{
			// Until 5th resend (1 second gone): Send with a high frequency
			timeBetweenResends = 200;
			remainingPacketsToConsider = 5;
		}
		else if (sentPacket.mResendCounter < 10)
		{
			// Until 10th resend (6 seconds gone): The connection seems to have some issues, reduce resending
			timeBetweenResends = 1000;
			remainingPacketsToConsider = 3;
		}
		else
		{
			// After that: There's serious connection problems, reduce resending to a minimum
			timeBetweenResends = 2500;
			remainingPacketsToConsider = 1;
		}
	}

	size_t index = 0;
	while (true)
	{
		// Skip the already confirmed packets
		if (nullptr != mQueue[index])
		{
			SentPacket& sentPacket = *mQueue[index];
			if (currentTimestamp >= sentPacket.mLastSendTimestamp + timeBetweenResends)
			{
				++sentPacket.mResendCounter;
				sentPacket.mLastSendTimestamp = currentTimestamp;

				// Trigger a resend
				outPacketsToResend.push_back(&sentPacket);
			}

			--remainingPacketsToConsider;
			if (remainingPacketsToConsider == 0)
				return;
		}

		// Go to the next packet in the queue
		++index;
		if (index >= mQueue.size())
			return;
		if (nullptr != mQueue[index] && mQueue[index]->mInitialTimestamp > minimumInitialTimestamp)
			return;
	}
}
