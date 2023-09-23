/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/crowdcontrol/CrowdControlClient.h"


bool CrowdControlClient::startConnection()
{
	if (mSetupDone)
	{
		// Already connected?
		if (mSocket.isValid())
			return true;
		mSetupDone = false;
	}

	Sockets::startupSockets();

	// Assume a locally running instance of the Crowd Control app
	if (!mSocket.connectTo("127.0.0.1", 43384))
		return false;

	// Done
	mSetupDone = true;
	return true;
}

void CrowdControlClient::stopConnection()
{
	mSocket.close();
	mSetupDone = false;
}

void CrowdControlClient::updateConnection(float timeElapsed)
{
	if (!mSetupDone)
		return;

	TCPSocket::ReceiveResult result;
	if (mSocket.receiveNonBlocking(result))
	{
		std::string errors;
		Json::Value jsonRoot = rmx::JsonHelper::loadFromMemory(result.mBuffer, &errors);
		if (jsonRoot.isObject())
		{
			evaluateMessage(jsonRoot);
		}
	}
}

void CrowdControlClient::evaluateMessage(const Json::Value& message)
{
	// Read properties from the JSON message
	const std::string code = message["code"].asString();
	const std::string viewer = message["viewer"].asString();
	// TODO: https://github.com/BttrDrgn/ccpp/blob/master/ccpp.cpp removes double quotes " here for code and viewer, is this needed for us as well?
	const int id = message["id"].asInt();

	// Trigger the effect
	const StatusCode statusCode = triggerEffect(code);

	// Send back a response
	const std::string response = "{\"id\":" + std::to_string(id) + ",\"status\":" + std::to_string((int)statusCode) + "}";
	mSocket.sendData((const uint8*)response.c_str(), response.length() + 1);
}

CrowdControlClient::StatusCode CrowdControlClient::triggerEffect(const std::string& effectCode)
{
	// TODO: Call a script function, something like "Game.triggerCrownControlEffect(effectCode)"
	//  -> However, this requires waiting for a frame to get a response...
	//  -> Or we even run into a timeout because no script is currently running
	// Alternative: Trigger script function run right away!

	//return StatusCode::SUCCESS;
	return StatusCode::UNAVAILABLE;
}
