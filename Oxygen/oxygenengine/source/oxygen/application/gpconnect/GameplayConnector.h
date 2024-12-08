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


struct GameStateIncrementPacket : public highlevel::PacketBase
{
	HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("GameStateIncrementPacket");

	uint32 mFrameNumber = 0;
	uint16 mInput = 0;

	virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
	{
		serializer.serialize(mFrameNumber);
		serializer.serialize(mInput);
	}
};


class GameplayConnector : public ServerClientBase, public SingleInstance<GameplayConnector>
{
public:
	static const uint16 DEFAULT_PORT = 28840;
	static const bool USE_IPV6 = false;

	enum class Role
	{
		NONE,
		HOST,
		CLIENT
	};

	struct Connection : public NetConnection {};

public:
	GameplayConnector();
	~GameplayConnector();

	inline Role getRole() const  { return mRole; }
	inline const Connection& getConnectionToHost() const  { return mConnectionToHost; }
	inline const std::vector<Connection*> getConnectionsToClient() const  { return mConnectsToClients; }

	bool setupAsHost();
	void startConnectToHost(std::string_view hostIP, uint16 hostPort);
	void closeConnections();

	void updateConnections(float deltaSeconds);

	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber, bool& inputWasInjected);

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override;
	virtual void destroyNetConnection(NetConnection& connection) override;
	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override;

private:
	void performCleanup();

private:
	UDPSocket mUDPSocket;
	ConnectionManager mConnectionManager;

	Role mRole = Role::NONE;

	Connection mConnectionToHost;
	std::vector<Connection*> mConnectsToClients;
	uint64 mLastCleanupTimestamp = 0;

	GameStateIncrementPacket mLastGameStateIncrementPacket;
};
