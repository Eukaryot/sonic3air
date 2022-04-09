/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/NetConnection.h"


class ServerNetConnection : public NetConnection
{
public:
	inline explicit ServerNetConnection(uint32 playerID) : mPlayerID(playerID) {}

	inline uint32 getPlayerID() const  { return mPlayerID; }

	void unregisterPlayer();

private:
	uint32 mPlayerID = 0;
};
