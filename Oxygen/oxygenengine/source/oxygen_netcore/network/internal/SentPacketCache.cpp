/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/internal/SentPacketCache.h"
#include "oxygen_netcore/network/LowLevelPackets.h"


void SentPacketCache::clear()
{
	mQueue.clear();
	mQueueStartUniquePacketID = 0;
	mNextUniquePacketID = 1;
}

uint32 SentPacketCache::getNextUniquePacketID() const
{
	return mNextUniquePacketID;
}

void SentPacketCache::addPacket(SentPacket& sentPacket, bool isStartConnectionPacket)
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

	mQueue.push_back(&sentPacket);
	++mNextUniquePacketID;
}

void SentPacketCache::onPacketReceiveConfirmed(uint32 uniquePacketID)
{
	// If the ID part is not part of the queue, ignore it
	if (uniquePacketID < mQueueStartUniquePacketID)
		return;
	const size_t index = uniquePacketID - mQueueStartUniquePacketID;
	if (index >= mQueue.size())
		return;

	// Also ignore if the packet already got confirmed
	if (mQueue[index]->mConfirmed)
		return;

	mQueue[index]->mConfirmed = true;

	// Remove as many items from queue as possible
	if (index == 0)
	{
		do
		{
			mQueue.front()->returnToPool();
			mQueue.pop_front();
			++mQueueStartUniquePacketID;
		}
		while (!mQueue.empty() && mQueue.front()->mConfirmed);
	}
}

void SentPacketCache::updateResend(std::vector<SentPacket*>& outPacketsToResend, uint64 currentTimestamp)
{
	if (mQueue.empty())
		return;

	const uint64 minimumInitialTimestamp = currentTimestamp - 250;		// Start resending packets after 250 ms
	for (size_t index = 0; index < mQueue.size(); ++index)
	{
		SentPacket& sentPacket = *mQueue[index];
		if (sentPacket.mInitialTimestamp > minimumInitialTimestamp)
		{
			// No need to look at the following queue items, none of them will have a lower timestamp
			break;
		}

		// Resend every 250 ms
		if (sentPacket.mResendCounter == 0 || currentTimestamp >= sentPacket.mLastResendTimestamp + 250)
		{
			// TODO: Limit number of resends, and cause a connection timeout when hitting that limit

			++sentPacket.mResendCounter;
			sentPacket.mLastResendTimestamp = currentTimestamp;

			// Trigger a resend
			outPacketsToResend.push_back(&sentPacket);
		}
	}
}
