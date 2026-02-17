/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
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


// Functionality kindly provided by CodenameGamma, thanks!
class ProfanityChecker
{
public:
	static std::string sanitizeViewerName(const std::string& input)
	{
		std::string result;
		size_t reserveLen = input.length() < MaxViewerNameLength ? input.length() : MaxViewerNameLength;
		result.reserve(reserveLen);

		const unsigned char* str = reinterpret_cast<const unsigned char*>(input.c_str());
		while (*str && result.length() < MaxViewerNameLength)
		{
			char32_t ch = 0;
			if ((*str & 0x80) == 0x00)
			{
				ch = *str++;
			}
			else if ((*str & 0xE0) == 0xC0)
			{
				ch = ((*str & 0x1F) << 6) | (str[1] & 0x3F);
				str += 2;
			}
			else if ((*str & 0xF0) == 0xE0)
			{
				ch = ((*str & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
				str += 3;
			}
			else if ((*str & 0xF8) == 0xF0)
			{
				ch = ((*str & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
				str += 4;
			}
			else
			{
				++str;
				continue;
			}

			if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
				(ch >= '0' && ch <= '9') || ch == '.' || ch == '!' || ch == '_')
			{
				result += static_cast<char>(ch);
			}
			else
			{
				char replacement = replaceDiacriticChar(ch);
				if (replacement != 0)
					result += replacement;
			}
		}

		// If result is empty or profane, fallback to "Viewer"
		if (result.empty() || containsLightProfanity(result))
		{
			return "Viewer";
		}
		return result;
	}

private:
	static constexpr size_t MaxViewerNameLength = 20;

	static bool containsLightProfanity(const std::string& name)
	{
		static const std::string badWords[] =  // The Fact these have to exist in my code at all makes me sad AF
		{
			// Mild to strong profanity
			"ass", "arse", "asshole", "bastard", "bitch", "bloody", "bollock", "bullshit",
			"crap", "cuck", "cunt", "damn", "dick", "dildo", "dyke", "fuck", "fucker", "fucking",
			"fuk", "goddamn", "hell", "jackass", "jerkoff", "motherfucker", "nigga", "nigger",
			"penis", "piss", "prick", "pussy", "retard", "shit", "shitty", "slut", "tard",
			"twat", "vagina", "wank", "whore",

			// Homophobic/transphobic/racist slurs
			"tranny", "fag", "faggot", "kike", "chink", "gook", "spic", "beaner", "wetback",
			"coon", "porchmonkey", "gypsy", "jigaboo", "raghead", "shemale", "nazi", "heilhitler"
		};
		std::string lowerName = name;
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

		for (const auto& word : badWords)
		{
			if (lowerName.find(word) != std::string::npos)
				return true;
		}
		return false;
	}

	static char replaceDiacriticChar(char32_t ch)
	{
		static const std::unordered_map<char32_t, char> replacements =
		{
			// Lowercase
			{ U'á', 'a' }, { U'à', 'a' }, { U'ä', 'a' }, { U'â', 'a' }, { U'ã', 'a' }, { U'å', 'a' },
			{ U'é', 'e' }, { U'è', 'e' }, { U'ë', 'e' }, { U'ê', 'e' },
			{ U'í', 'i' }, { U'ì', 'i' }, { U'ï', 'i' }, { U'î', 'i' },
			{ U'ó', 'o' }, { U'ò', 'o' }, { U'ö', 'o' }, { U'ô', 'o' }, { U'õ', 'o' },
			{ U'ú', 'u' }, { U'ù', 'u' }, { U'ü', 'u' }, { U'û', 'u' },
			{ U'ñ', 'n' },
			// Uppercase
			{ U'Á', 'A' }, { U'À', 'A' }, { U'Ä', 'A' }, { U'Â', 'A' }, { U'Ã', 'A' }, { U'Å', 'A' },
			{ U'É', 'E' }, { U'È', 'E' }, { U'Ë', 'E' }, { U'Ê', 'E' },
			{ U'Í', 'I' }, { U'Ì', 'I' }, { U'Ï', 'I' }, { U'Î', 'I' },
			{ U'Ó', 'O' }, { U'Ò', 'O' }, { U'Ö', 'O' }, { U'Ô', 'O' }, { U'Õ', 'O' },
			{ U'Ú', 'U' }, { U'Ù', 'U' }, { U'Ü', 'U' }, { U'Û', 'U' },
			{ U'Ñ', 'N' }
		};

		auto it = replacements.find(ch);
		return it != replacements.end() ? it->second : 0;
	}
};



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
	request.mViewer = "Viewer";		// Fallback viewer name

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
	const bool hasViewer   = jsonHelper.tryReadString("viewer", request.mViewer);

	if (hasViewer)
	{
		ProfanityChecker::sanitizeViewerName(request.mViewer);
	}

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

	// Call signature: "void CrowdControl.triggerEffect(u32 id, string code, s32 quantity, s32 duration, string viewer)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_32, request.mId);
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(request.mCode));
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::INT_32, request.mQuantity);
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::INT_32, request.mDuration);
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(request.mViewer));
	codeExec.executeScriptFunction("CrowdControl.triggerEffect", false, &execData);
}
