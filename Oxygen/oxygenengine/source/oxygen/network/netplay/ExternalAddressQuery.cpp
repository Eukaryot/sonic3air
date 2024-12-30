/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/network/netplay/ExternalAddressQuery.h"
#include "oxygen/network/EngineServerClient.h"

#include "oxygen_netcore/serverclient/Packets.h"


void ExternalAddressQuery::reset()
{
	mOwnExternalIP.clear();
	mOwnExternalPort = 0;
	mRunning = false;
	mQueryID = 0;
	mSendCounter = 0;
	mResendTimer = 0.0f;
}

void ExternalAddressQuery::startQuery()
{
	reset();
	mQueryID = (uint64)(1 + rand()) + ((uint64)rand() << 16) + ((uint64)rand() << 32) + ((uint64)rand() << 48);
	mRunning = true;

	resendQuery();
}

void ExternalAddressQuery::updateQuery(float deltaSeconds)
{
	if (mRunning)
	{
		mResendTimer += deltaSeconds;
		if (mResendTimer >= 0.1f)
		{
			resendQuery();
		}
	}

}

bool ExternalAddressQuery::onReceivedConnectionlessPacket(ConnectionlessPacketEvaluation& evaluation)
{
	switch (evaluation.mLowLevelSignature)
	{
		case network::ReplyExternalAddressConnectionless::SIGNATURE:
		{
			network::ReplyExternalAddressConnectionless packet;
			if (!packet.serializePacket(evaluation.mSerializer, 1))
				return false;

			if (mQueryID == packet.mQueryID)
			{
				mOwnExternalIP = packet.mIP;
				mOwnExternalPort = packet.mPort;
				mRunning = false;
			}
			return true;
		}
	}

	return false;
}

void ExternalAddressQuery::resendQuery()
{
	std::string serverIP;
	if (!EngineServerClient::resolveGameServerHostName(Configuration::instance().mGameServerBase.mServerHostName, serverIP, mUseIPv6))
	{
		// TODO: Error handling
		RMX_ASSERT(false, "Failed to resolve game server host name: " << Configuration::instance().mGameServerBase.mServerHostName);
		return;
	}

	// Retrieve external address for the socket
	// TODO: This needs to be repeated if necessary
	network::GetExternalAddressConnectionless packet;
	packet.mQueryID = mQueryID;
	mConnectionManager.sendConnectionlessLowLevelPacket(packet, SocketAddress(serverIP, Configuration::instance().mGameServerBase.mServerPortUDP), 0, 0);

	++mSendCounter;
	mResendTimer = 0.0f;
}
