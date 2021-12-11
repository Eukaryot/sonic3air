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

void SentPacketCache::addPacket(uint32 uniquePacketID, const std::vector<uint8>& content, uint64 currentTimestamp)
{
	// Special handling if this is the first packet to be added
	if (mQueueStartUniquePacketID == 0)
	{
		// First expected incoming unique packet ID can be 0 or 1:
		//  - On client side, ID 0 is the first one, as it's used for the StartConnectionPacket
		//  - On server side, no packet with ID 0 will be added, so we'd expect the first packet to use ID 1
		RMX_ASSERT(uniquePacketID <= 1, "Unique packet ID differs from expected ID");
		mQueueStartUniquePacketID = uniquePacketID;
		mNextUniquePacketID = uniquePacketID;
	}
	RMX_ASSERT(uniquePacketID == mNextUniquePacketID, "Unique packet ID differs from expected ID");
	++mNextUniquePacketID;

	mQueue.emplace_back();
	CacheItem& item = mQueue.back();
	item.mContent = content;			// TODO: Avoid copying if possible
	item.mInitialTimestamp = currentTimestamp;
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
	if (mQueue[index].mConfirmed)
		return;

	mQueue[index].mConfirmed = true;
	if (index == 0)
	{
		do
		{
			mQueue.pop_front();
			++mQueueStartUniquePacketID;
		}
		while (!mQueue.empty() && mQueue.front().mConfirmed);
	}
}

void SentPacketCache::updateResend(std::vector<CacheItem*>& outItemsToResend, uint64 currentTimestamp)
{
	if (mQueue.empty())
		return;

	const uint64 minimumInitialTimestamp = currentTimestamp - 250;		// Start resending packets after 250 ms
	for (size_t index = 0; index < mQueue.size(); ++index)
	{
		CacheItem& item = mQueue[index];
		if (item.mInitialTimestamp > minimumInitialTimestamp)
		{
			// No need to look at the following queue items, none of them will have a lower timestamp
			break;
		}

		// Resend every 250 ms
		if (item.mResendCounter == 0 || currentTimestamp >= item.mLastResendTimestamp + 250)
		{
			// TODO: Limit number of resends, and cause a connection timeout when hitting that limit

			++item.mResendCounter;
			item.mLastResendTimestamp = currentTimestamp;

			// Trigger a resend
			outItemsToResend.push_back(&item);
		}
	}
}
