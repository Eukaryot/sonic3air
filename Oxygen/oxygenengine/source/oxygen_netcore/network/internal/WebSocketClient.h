/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"

#ifdef PLATFORM_WEB
	#include <emscripten/websocket.h>
#endif

class NetConnection;


class WebSocketClient
{
public:
	WebSocketClient(NetConnection& connection);

	bool isAvailable() const;
	void clear();
	bool connectTo(const SocketAddress& remoteAddress);
	bool sendPacket(const std::vector<uint8>& content);

#ifdef PLATFORM_WEB
private:
	static EM_BOOL webSocketOpenCallback(int eventType, const EmscriptenWebSocketOpenEvent* webSocketEvent, void* userData);
	static EM_BOOL webSocketErrorCallback(int eventType, const EmscriptenWebSocketErrorEvent* webSocketEvent, void* userData);
	static EM_BOOL webSocketCloseCallback(int eventType, const EmscriptenWebSocketCloseEvent* webSocketEvent, void* userData);
	static EM_BOOL webSocketMessageCallback(int eventType, const EmscriptenWebSocketMessageEvent* webSocketEvent, void* userData);

private:
	bool onWebSocketOpen(const EmscriptenWebSocketOpenEvent* webSocketEvent);
	bool onWebSocketError(const EmscriptenWebSocketErrorEvent* webSocketEvent);
	bool onWebSocketMessage(const EmscriptenWebSocketMessageEvent* webSocketEvent);

private:
	EMSCRIPTEN_WEBSOCKET_T mWebSocket = 0;
#endif

private:
	NetConnection& mConnection;
};
