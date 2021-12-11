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


struct ReceivedPacket
{
public:
	struct Dump
	{
		std::vector<const ReceivedPacket*> mPackets;
	};

public:
	std::vector<uint8> mContent;
	uint16 mLowLevelSignature = 0;
	SocketAddress mSenderAddress;
	NetConnection* mConnection = nullptr;
	bool mShouldBeReturned = false;

public:
	inline void setDump(Dump* dump)
	{
		mDump = dump;
	}

	inline void returnToDump()
	{
		RMX_ASSERT(nullptr != mDump, "Dump is not set or received packet already got returned");
		mDump->mPackets.push_back(this);
		mDump = nullptr;	// Mark as being returned
		mShouldBeReturned = false;
	}

private:
	Dump* mDump = nullptr;
};
