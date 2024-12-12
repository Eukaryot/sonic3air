/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/gpconnect/SingleRequest.h"

#include <chrono>


void SingleRequest::startRequest(const std::string& serverHostName, uint16 serverPort, highlevel::RequestBase& request, ConnectionManager& connectionManager)
{
	// TODO: Clear old request

	std::string ip;
	Sockets::resolveToIP(serverHostName, ip);

	mRequest = &request;
	mState = State::CONNECTION_STARTED;
	mConnection.startConnectTo(connectionManager, SocketAddress(ip, serverPort), getCurrentTimestamp());
}

void SingleRequest::updateRequest()
{
	mConnection.updateConnection(getCurrentTimestamp());

	switch (mState)
	{
		case State::CONNECTION_STARTED:
		{
			// TODO: Add error handling

			if (mConnection.getState() == NetConnection::State::CONNECTED)
			{
				// Send actual request
				mConnection.sendRequest(*mRequest);
				mState = State::REQUEST_SENT;
			}
			break;
		}

		case State::REQUEST_SENT:
		{
			// TODO: Add error handling
			if (mRequest->hasResponse())
			{
				// Close connection
				mConnection.clear();
				mState = State::FINISHED;
			}
			break;
		}
	}
}

uint64 SingleRequest::getCurrentTimestamp() const
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
