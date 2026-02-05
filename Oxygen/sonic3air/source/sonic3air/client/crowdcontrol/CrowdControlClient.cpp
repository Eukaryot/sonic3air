/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/crowdcontrol/CrowdControlClient.h"

#include "oxygen/application/Application.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"


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
	if (!mSocket.connectTo("127.0.0.1", 58430))
		return false;

	LogDisplay::instance().setLogDisplay("Now connected to Crowd Control!", 10.0f);

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
#if 0
	// Test for script function call if not connected to CC
	{
		static float timeout = 3.0f;
		timeout -= std::min(timeElapsed, 0.05f);
		if (timeout < 0.0f)
		{
			triggerEffect("AddRing");
			timeout = 5.0f;
		}
	}
#endif

	if (!mSetupDone)
		return;

	TCPSocket::ReceiveResult result;
	if (mSocket.receiveNonBlocking(result) && !result.mBuffer.empty())
	{
		std::string errors;
		Json::Value jsonRoot = rmx::JsonHelper::loadFromMemory(result.mBuffer, &errors);
		if (jsonRoot.isObject())
		{
			evaluateRequestJson(jsonRoot);
		}
	}
}

void CrowdControlClient::sendResponse(uint32 id, uint8 status, lemon::StringRef message)
{
	// Send back a response JSON to Crowd Control
	std::string response = "{";
	response += "\"id\":" + std::to_string(id);
	response += ",\"status\":" + std::to_string(status);
	if (!message.isEmpty())
		response += ",\"message\":\"" + std::string(message.getString()) + "\"";
	response += "}";

	mSocket.sendData((const uint8*)response.c_str(), response.length() + 1);
}

void CrowdControlClient::evaluateRequestJson(const Json::Value& requestJson)
{
	// Read request properties from the JSON
	Request request;

	JsonHelper jsonHelper(requestJson);
	if (!jsonHelper.tryReadInt("id", request.mId))
		return;
	if (!jsonHelper.tryReadAsInt("type", request.mType))
		return;

	const bool hasCode     = jsonHelper.tryReadString("code", request.mCode);
	const bool hasMessage  = jsonHelper.tryReadString("message", request.mMessage);
	const bool hasCost     = jsonHelper.tryReadInt("cost", request.mCost);
	const bool hasDuration = jsonHelper.tryReadInt("duration", request.mDuration);
	const bool hasQuantity = jsonHelper.tryReadInt("quantity", request.mQuantity);

	switch (request.mType)
	{
		case RequestType::START:
		{
			// Trigger the effect
			triggerEffect(request);

			// Note that we're not sending a response right away, but scripts will have to handle that by calling "CrowdControl.sendResponse"
			break;
		}

		default:
			break;
	}
}

void CrowdControlClient::triggerEffect(const Request& request)
{
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

	// Call signature: "void CrowdControl.triggerEffect(u32 id, string code, s32 quantity, s32 duration)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_32, request.mId);
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(request.mCode));
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::INT_32, request.mQuantity);
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::INT_32, request.mDuration);
	codeExec.executeScriptFunction("CrowdControl.triggerEffect", false, &execData);
}
