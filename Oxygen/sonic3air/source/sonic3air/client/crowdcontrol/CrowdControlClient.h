/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"

#include <lemon/program/StringRef.h>


// Crowd Control documentation: https://developer.crowdcontrol.live/sdk/simpletcp/structure.html

class CrowdControlClient
{
public:
	bool startConnection();
	void stopConnection();
	void updateConnection(float timeElapsed);

	void sendResponse(uint32 id, uint8 status, lemon::StringRef message);

private:
	enum class StatusCode : uint8
	{
		SUCCESS			= 0x00,		// The effect executed successfully
		FAILURE			= 0x01,		// The effect failed to trigger, but is still available for use; viewer(s) will be refunded
		UNAVAILABLE		= 0x02,		// Same as FAILURE but the effect is no longer available for use for the remainder of the game
		RETRY			= 0x03,		// The effect cannot be triggered right now, try again in a few seconds - note: this is the "normal" failure response
		PAUSED			= 0x06,		// The timed effect has been paused and is now waiting
		RESUME			= 0x07,		// The timed effect has been resumed and is counting down again
		FINISHED		= 0x08,		// The timed effect has finished

		VISIBLE			= 0x80,		// The effect should be shown in the menu
		NOT_VISIBLE		= 0x81,		// The effect should be hidden in the menu
		SELECTABLE		= 0x82,		// The effect should be selectable in the menu
		NOT_SELECTABLE	= 0x83		// The effect should be unselectable in the menu
	};

	enum class RequestType
	{
		TEST		= 0x00,		// Respond as with a normal request but don’t actually activate the effect
		START		= 0x01,		// A standard effect request. A response (see above) is always required
		STOP		= 0x02,		// A request to stop all instances of the requested effect code
		PLAYER_INFO	= 0xe0,		// RESERVED This field will appear in a future release. Request information about the player
		LOGIN		= 0xf0,		// Indicates that this is a login request or response
		KEEP_ALIVE	= 0xff
	};

	struct Request
	{
		int mId = 0;
		RequestType mType = RequestType::KEEP_ALIVE;
		std::string mCode;
		std::string mMessage;
		int mCost = 0;
		int mDuration = 0;
		int mQuantity = 0;
		std::string mViewer;
	};

private:
	void evaluateRequestJson(const Json::Value& requestJson);
	void triggerEffect(const Request& request);

private:
	bool mSetupDone = false;
	TCPSocket mSocket;
};
