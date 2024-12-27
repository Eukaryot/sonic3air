/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/network/netplay/ExternalAddressQuery.h"

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/ConnectionManager.h"

class ControlsIn;
class NetplayClient;
class NetplayHost;


class NetplayManager : public ConnectionListenerInterface, public SingleInstance<NetplayManager>
{
public:
	static const uint16 DEFAULT_PORT = 28840;

public:
	NetplayManager();
	~NetplayManager();

	inline ConnectionManager& getConnectionManager()		{ return mConnectionManager; }
	inline bool isUsingIPv6() const							{ return mUseIPv6; }

	inline NetplayHost* getNetplayHost() 					{ return mNetplayHost; }
	inline NetplayClient* getNetplayClient()				{ return mNetplayClient; }
	inline const ExternalAddressQuery& getExternalAddressQuery() const  { return mExternalAddressQuery; }

	bool setupAsHost(bool registerSessionAtServer, uint16 port = DEFAULT_PORT);
	void startJoinViaServer();
	void startJoinDirect(std::string_view ip, uint16 port);
	void closeConnections();

	void updateConnections(float deltaSeconds);
	bool onReceivedGameServerPacket(ReceivedPacketEvaluation& evaluation);

	bool canBeginNextFrame(uint32 frameNumber);
	void onFrameUpdate(ControlsIn& controlsIn, uint32 frameNumber);

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override;
	virtual void destroyNetConnection(NetConnection& connection) override;

	virtual bool onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation)	override;
	virtual bool onReceivedPacket(ReceivedPacketEvaluation& evaluation) override;

private:
	bool restartConnection(bool asHost, uint16 hostPort = 0);

private:
	UDPSocket mUDPSocket;
	ConnectionManager mConnectionManager;
	bool mUseIPv6 = false;

	NetplayHost* mNetplayHost = nullptr;
	NetplayClient* mNetplayClient = nullptr;
	ExternalAddressQuery mExternalAddressQuery;
};
