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
	return false;
}

bool NetplaySetup::onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::RegisterForNetplayRequest::Query::PACKET_TYPE:
		{
			using Request = network::RegisterForNetplayRequest;
			Request request;
			if (!evaluation.readQuery(request))
				return false;

			const uint64 sessionID = request.mQuery.mSessionID;

			request.mResponse.mSessionID = request.mQuery.mSessionID;
			request.mResponse.mSuccess = true;

			if (request.mQuery.mIsHost)
			{
				if (nullptr != mapFind(mSessions, sessionID))
				{
					// TODO: Send back an error (or maybe change the whole packet to be a request instead?)
					return true;
				}

				// Add new session
				Session& session = mSessions[sessionID];
				session.mSessionID = sessionID;
				session.mRegistrationTimestamp = ConnectionManager::getCurrentTimestamp();
				session.mHostConnection = &evaluation.mConnection;

				session.mHostGameSocketIP = request.mQuery.mGameSocketExternalIP;
				session.mHostGameSocketPort = request.mQuery.mGameSocketExternalPort;
			}
			else
			{
				// Check if the session exists and its host is still valid
				Session* session = mapFind(mSessions, sessionID);
				if (nullptr != session && session->mHostConnection.isValid())
				{
					// Inform the host and the client
					ParticipantInfo host;
					host.mGameSocketIP = session->mHostGameSocketIP;
					host.mGameSocketPort = session->mHostGameSocketPort;
					host.mConnection = session->mHostConnection;

					ParticipantInfo client;
					client.mGameSocketIP = request.mQuery.mGameSocketExternalIP;
					client.mGameSocketPort = request.mQuery.mGameSocketExternalPort;
					client.mConnection = &evaluation.mConnection;

					connectHostAndClient(*session, host, client);
				}
				else
				{
					request.mResponse.mSuccess = false;
				}
			}


			return evaluation.respond(request);
		}
	}

	return false;
}

void NetplaySetup::onDestroyConnection(NetConnection& connection)
{
	for (auto it = mSessions.begin(); it != mSessions.end(); ++it)
	{
		if (it->second.mHostConnection.get() == &connection)
		{
			mSessions.erase(it);
			return;
		}
	}
}

void NetplaySetup::connectHostAndClient(Session& session, const ParticipantInfo& host, const ParticipantInfo& client)
{
	// Inform the host and the client
	network::ConnectToNetplayPacket response;
	response.mSessionID = session.mSessionID;
	response.mConnectionType = network::NetplayConnectionType::PUNCHTHROUGH;	// TODO: Decide which connection type is the right one

	response.mConnectToIP = client.mGameSocketIP;
	response.mConnectToPort = client.mGameSocketPort;
	host.mConnection->sendPacket(response);

	response.mConnectToIP = host.mGameSocketIP;
	response.mConnectToPort = host.mGameSocketPort;
	client.mConnection->sendPacket(response);
}
