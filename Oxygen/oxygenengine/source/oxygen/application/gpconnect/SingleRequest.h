/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/RequestBase.h"


class SingleRequest
{
public:
	enum class State
	{
		INACTIVE = 0,
		CONNECTION_STARTED,
		REQUEST_SENT,
		FINISHED
	};

public:
	inline State getState() const  { return mState; }

	void startRequest(const std::string& serverHostName, uint16 serverPort, highlevel::RequestBase& request, ConnectionManager& connectionManager);
	void updateRequest();

private:
	uint64 getCurrentTimestamp() const;

private:
	State mState = State::INACTIVE;
	highlevel::RequestBase* mRequest = nullptr;
	NetConnection mConnection;
};
