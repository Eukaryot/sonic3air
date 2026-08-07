/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/RequestBase.h"
#include "oxygen_netcore/network/NetConnection.h"


highlevel::RequestBase::~RequestBase()
{
	// Unregister at connection
	if (nullptr != mRegisteredAtConnection)
	{
		mRegisteredAtConnection->unregisterRequest(*this);
	}
}
