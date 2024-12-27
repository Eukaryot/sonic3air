/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class ConnectionManager;
struct ConnectionlessPacketEvaluation;


class ExternalAddressQuery
{
public:
	std::string mOwnExternalIP;
	uint16 mOwnExternalPort = 0;

	bool mRunning = false;
	uint64 mQueryID = 0;
	int mSendCounter = 0;
	float mResendTimer = 0.0f;

public:
	ExternalAddressQuery(ConnectionManager& connectionManager, bool useIPv6) : mConnectionManager(connectionManager), mUseIPv6(useIPv6) {}

	void reset();
	void startQuery();
	void updateQuery(float deltaSeconds);
	bool onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation);

private:
	void resendQuery();

private:
	ConnectionManager& mConnectionManager;
	const bool mUseIPv6 = false;
};
