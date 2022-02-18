/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class NetConnection;


// Basic wrapper to support WebSocket connections as well
//  -> It's only server-side and quite minimalistic, only for the use-case of communication with the web version of the game client
//  -> For details on the WebSocket protocol, see https://datatracker.ietf.org/doc/html/rfc6455#section-5.2
class WebSocketWrapper
{
public:
	static bool handleWebSocketHttpHeader(const std::vector<uint8>& receivedData, String& outWebSocketKey);
	static void getWebSocketHttpResponse(const String& webSocketKey, String& outResponse);
	static bool processReceivedClientPacket(std::vector<uint8>& data);
	static void wrapDataToSendToClient(const std::vector<uint8>& data, std::vector<uint8>& outWrappedData);
};
