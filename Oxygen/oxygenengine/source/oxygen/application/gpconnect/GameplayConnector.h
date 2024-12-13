/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/network/ConnectionManager.h"

class ControlsIn;
class GameplayClient;
class GameplayHost;


class GameplayConnector : public ServerClientBase, public SingleInstance<GameplayConnector>
{
public:
	static const uint16 DEFAULT_PORT = 28840;
	static const bool USE_IPV6 = false;

public:
	GameplayConnector();
	~GameplayConnector();

	inline ConnectionManager& getConnectionManager()		{ return mConnectionManager; }

	inline const GameplayHost* getGameplayHost() const		{ return mGameplayHost; }
	inline const GameplayClient* getGameplayClient() const	{ return mGameplayClient; }

	bool setupAsHost(uint16 port = DEFAULT_PORT);
	void startConnectToHost(std::string_view hostIP, uint16 hostPort);
	void closeConnections();

	void updateConnections(float deltaSeconds);

	bool canBeginNextFrame(uint32 frameNumber);
	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber);

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override;
	virtual void destroyNetConnection(NetConnection& connection) override;
	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override;

private:
	UDPSocket mUDPSocket;
	ConnectionManager mConnectionManager;

	GameplayHost* mGameplayHost = nullptr;
	GameplayClient* mGameplayClient = nullptr;
};
