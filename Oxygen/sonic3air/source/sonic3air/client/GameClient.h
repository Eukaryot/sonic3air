/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/client/GhostSync.h"
#include "sonic3air/client/UpdateCheck.h"

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/serverclient/Packets.h"

#include "oxygen/network/EngineServerClient.h"


class GameClient : public EngineServerClient::Listener, public SingleInstance<GameClient>
{
public:
	GameClient();

	GhostSync& getGhostSync()		 { return mGhostSync; }
	UpdateCheck& getUpdateCheck()	 { return mUpdateCheck; }

	void setupClient();
	void updateClient(float timeElapsed);

protected:
	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override;

private:
	bool mEvaluatedServerFeatures = false;
	GhostSync mGhostSync;
	UpdateCheck mUpdateCheck;
};
