/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/client/UpdateCheck.h"

#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/serverclient/Packets.h"


class GameClient : public ServerClientBase, public SingleInstance<GameClient>
{
public:
	GameClient();
	~GameClient();

	void setupClient();
	void updateClient(float timeElapsed);

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override
	{
		// Do not allow incoming connections
		return nullptr;
	}

	virtual void destroyNetConnection(NetConnection& connection) override
	{
		RMX_ASSERT(false, "This should never get called");
	}

private:
	enum class State
	{
		NONE,
		STARTED,
		READY,
	};

private:
	UDPSocket mSocket;
	ConnectionManager mConnectionManager;
	NetConnection mServerConnection;
	State mState = State::NONE;

	UpdateCheck mUpdateCheck;
	network::GetServerFeaturesRequest mGetServerFeaturesRequest;
};
