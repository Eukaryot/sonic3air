/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/subsystems/NetplaySetup.h"

#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/serverclient/NetplaySetupPackets.h"


bool NetplaySetup::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::RegisterNetplayHostPacket::PACKET_TYPE:
		{
			network::RegisterNetplayHostPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			if (nullptr != mapFind(mSessions, packet.mSessionID))
			{
				// TODO: Send back an error (or maybe change the whole packet to be a request instead?)
				return true;
			}

			// Add new session
			Session& session = mSessions[packet.mSessionID];
			session.mSessionID = packet.mSessionID;
			session.mRegistrationTimestamp = ConnectionManager::getCurrentTimestamp();
			session.mHostConnection = &evaluation.mConnection;

			session.mHostGameSocketIP = packet.mGameSocketExternalIP;
			session.mHostGameSocketPort = packet.mGameSocketExternalPort;

			return true;
		}
	}

	switch (evaluation.mPacketType)
	{
		case network::RegisterNetplayClientPacket::PACKET_TYPE:
		{
			network::RegisterNetplayClientPacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			Session* session = mapFind(mSessions, packet.mSessionID);
			if (nullptr == session)
			{
				// TODO: Send back an error (or maybe make the whole packet into a request instead?)
				return true;
			}

			// Check if the host is still valid
			if (!session->mHostConnection.isValid())
			{
				// TODO: Send back an error
				return false;
			}

			// Inform the host and the client
			network::ConnectToNetplayPacket response;
			response.mSessionID = session->mSessionID;
			response.mConnectionType = network::NetplayConnectionType::PUNCHTHROUGH;	// TODO: Decide which connection type is the right one

			response.mConnectToIP = packet.mGameSocketExternalIP;
			response.mConnectToPort = packet.mGameSocketExternalPort;
			session->mHostConnection->sendPacket(response);

			response.mConnectToIP = session->mHostGameSocketIP;
			response.mConnectToPort = session->mHostGameSocketPort;
			evaluation.mConnection.sendPacket(response);

			return true;
		}
	}

	return false;
}
