/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

namespace lowlevel
{
	struct HighLevelPacket;
}


class SentPacketCache
{
public:
	struct CacheItem
	{
		// TODO: Some kind of pooling / instance management would be nice for these, or at least their content

		std::vector<uint8> mContent;

		uint64 mInitialTimestamp = 0;
		uint64 mLastResendTimestamp = 0;
		int mResendCounter = 0;
		bool mConfirmed = false;

		// TODO: Add data relevant for re-sending, like last send timestamp
	};

public:
	void clear();
	uint32 getNextUniquePacketID() const;

	void addPacket(uint32 uniquePacketID, const std::vector<uint8>& content, uint64 currentTimestamp);
	void onPacketReceiveConfirmed(uint32 uniquePacketID);

	void updateResend(std::vector<CacheItem*>& outItemsToResend, uint64 currentTimestamp);

private:
	uint32 mQueueStartUniquePacketID = 1;
	uint32 mNextUniquePacketID = 1;			// This should always be "mQueueStartUniquePacketID + mQueue.size()"
	std::deque<CacheItem> mQueue;
};
