/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/serverclient/Packets.h"

class GameClient;


class UpdateCheck
{
public:
	enum class State
	{
		INACTIVE,
		CONNECTING,
		READY_TO_START,
		SEND_QUERY,
		WAITING_FOR_RESPONSE,
		HAS_RESPONSE,
		FAILED
	};

public:
	inline UpdateCheck(GameClient& gameClient) : mGameClient(gameClient) {}

	inline State getState() const  { return mState; }
	bool hasUpdate() const;
	const network::AppUpdateCheckRequest::Response* getResponse() const;

	void startUpdateCheck();

	void performUpdate();
	void evaluateServerFeaturesResponse(const network::GetServerFeaturesRequest& request);

private:
	GameClient& mGameClient;
	State mState = State::INACTIVE;
	network::AppUpdateCheckRequest mAppUpdateCheckRequest;
	uint64 mLastUpdateCheckTimestamp = 0;
};
