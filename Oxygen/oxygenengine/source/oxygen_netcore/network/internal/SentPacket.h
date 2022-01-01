/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct SentPacket
{
public:
	std::vector<uint8> mContent;
	uint64 mInitialTimestamp = 0;
	uint64 mLastSendTimestamp = 0;
	int mResendCounter = 0;

public:
	inline void initializeWithPool(RentableObjectPool<SentPacket>& pool)
	{
		mOwningPool = &pool;
	}

	inline void returnToPool()
	{
		mOwningPool->returnObject(*this);
	}

private:
	RentableObjectPool<SentPacket>* mOwningPool = nullptr;
};
