/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/serverclient/Packets.h"


class UpdateCheck
{
public:
	inline UpdateCheck(NetConnection& serverConnection) : mServerConnection(serverConnection) {}

	bool hasUpdate() const;

	void performUpdate();
	void evaluateServerFeaturesResponse(const network::GetServerFeaturesRequest& request);

private:
	enum class State
	{
		INACTIVE,
		READY_TO_START,
		WAITING_FOR_RESPONSE,
		HAS_RESPONSE,
		FAILED
	};

private:
	NetConnection& mServerConnection;
	State mState = State::INACTIVE;
	network::AppUpdateCheckRequest mAppUpdateCheckRequest;
};
