/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"


class NetplaySetup
{
public:
	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);
	bool onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation);
	void onDestroyConnection(NetConnection& connection);

private:
	struct Session
	{
		uint64 mSessionID = 0;
		uint64 mRegistrationTimestamp = 0;
		WeakPtr<NetConnection> mHostConnection;

		std::string mHostGameSocketIP;
		uint16 mHostGameSocketPort = 0;
	};

	struct ParticipantInfo
	{
		NetConnection* mConnection = nullptr;
		std::string_view mGameSocketIP;
		uint16 mGameSocketPort = 0;
	};

private:
	void connectHostAndClient(Session& session, const ParticipantInfo& host, const ParticipantInfo& client);

private:
	std::unordered_map<uint64, Session> mSessions;
};
