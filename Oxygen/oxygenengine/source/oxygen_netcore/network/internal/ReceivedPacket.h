/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"

class NetConnection;


struct ReceivedPacket
{
friend class ReceivedPacketCache;

public:
	struct Dump
	{
		std::vector<const ReceivedPacket*> mPackets;
	};

public:
	std::vector<uint8> mContent;
	SocketAddress mSenderAddress;
	uint16 mLowLevelSignature = 0;
	NetConnection* mConnection = nullptr;

public:
	inline void initializeWithDump(Dump* dump)
	{
		RMX_ASSERT(nullptr == mDump, "Expected a packet that is removed from the dump to not have a dump pointer already set");
		RMX_ASSERT(mReferenceCounter == 0, "Expected a packet that is removed from the dump to not have a reference count of zero");
		mDump = dump;
		mReferenceCounter = 1;
	}

	inline void incReferenceCounter() const
	{
		RMX_ASSERT(nullptr != mDump, "Reference counting is used for an instance already returned to the dump");
		++mReferenceCounter;
	}

	inline void decReferenceCounter() const
	{
		RMX_ASSERT(nullptr != mDump, "Reference counting is used for an instance already returned to the dump");
		RMX_ASSERT(mReferenceCounter > 0, "Trying to remove a reference when counter already is at zero");
		--mReferenceCounter;
		if (mReferenceCounter == 0)
		{
			mDump->mPackets.push_back(this);
			mDump = nullptr;	// Mark as being returned
		}
	}

private:
	mutable Dump* mDump = nullptr;
	mutable int mReferenceCounter = 0;
};
