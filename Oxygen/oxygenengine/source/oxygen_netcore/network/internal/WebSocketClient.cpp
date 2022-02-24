/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/internal/WebSocketClient.h"
#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/NetConnection.h"


WebSocketClient::WebSocketClient(NetConnection& connection) :
	mConnection(connection)
{
}

bool WebSocketClient::isAvailable() const
{
	// Web socket usage via emscripten is only available for the web version
#ifdef PLATFORM_WEB
	return true;
#else
	return false;
#endif
}

void WebSocketClient::clear()
{
#ifdef PLATFORM_WEB
	if (mWebSocket != 0)
	{
		emscripten_websocket_close(mWebSocket, 1000, "Connection cleared");
		emscripten_websocket_delete(mWebSocket);
		mWebSocket = 0;
	}
#endif
}

bool WebSocketClient::connectTo(const SocketAddress& remoteAddress)
{
#ifdef PLATFORM_WEB
	// Use web socket for communication (client-side only)
	if (!emscripten_websocket_is_supported())
		return false;

	const std::string url = "ws://" + remoteAddress.getIP() + ":" + std::to_string(remoteAddress.getPort());
	EmscriptenWebSocketCreateAttributes attributes =
	{
		url.c_str(),
		nullptr,
		EM_TRUE
	};
	mWebSocket = emscripten_websocket_new(&attributes);
	if (mWebSocket == 0)
		return false;

	RMX_LOG_INFO("Successfully created web socket");
	emscripten_websocket_set_onopen_callback   (mWebSocket, this, WebSocketClient::webSocketOpenCallback);
	emscripten_websocket_set_onerror_callback  (mWebSocket, this, WebSocketClient::webSocketErrorCallback);
	emscripten_websocket_set_onclose_callback  (mWebSocket, this, WebSocketClient::webSocketCloseCallback);
	emscripten_websocket_set_onmessage_callback(mWebSocket, this, WebSocketClient::webSocketMessageCallback);
	return true;
#else
	return false;
#endif
}

bool WebSocketClient::sendPacket(const std::vector<uint8>& content)
{
#ifdef PLATFORM_WEB
	if (mWebSocket == 0)
		return false;

	const EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(mWebSocket, (void*)&content[0], (uint32_t)content.size());
	RMX_ASSERT(result == 0, "emscripten_websocket_send_binary failed with result " << result);
	return (result == 0);
#else
	return false;
#endif
}


#ifdef PLATFORM_WEB

EM_BOOL WebSocketClient::webSocketOpenCallback(int eventType, const EmscriptenWebSocketOpenEvent* webSocketEvent, void* userData)
{
	return reinterpret_cast<WebSocketClient*>(userData)->onWebSocketOpen(webSocketEvent);
}

EM_BOOL WebSocketClient::webSocketErrorCallback(int eventType, const EmscriptenWebSocketErrorEvent* webSocketEvent, void* userData)
{
	// TODO: Implement this
	return EM_TRUE;
}

EM_BOOL WebSocketClient::webSocketCloseCallback(int eventType, const EmscriptenWebSocketCloseEvent* webSocketEvent, void* userData)
{
	// TODO: Implement this
	return EM_TRUE;
}

EM_BOOL WebSocketClient::webSocketMessageCallback(int eventType, const EmscriptenWebSocketMessageEvent* webSocketEvent, void* userData)
{
	return reinterpret_cast<WebSocketClient*>(userData)->onWebSocketMessage(webSocketEvent);
}

bool WebSocketClient::onWebSocketOpen(const EmscriptenWebSocketOpenEvent* webSocketEvent)
{
	RMX_LOG_INFO("onWebSocketOpen");
	return mConnection.finishStartConnect();
}

bool WebSocketClient::onWebSocketMessage(const EmscriptenWebSocketMessageEvent* webSocketEvent)
{
	RMX_LOG_INFO("onWebSocketMessage");
	if (webSocketEvent->numBytes == 0)	// Ignore empty messages
		return true;
	if (webSocketEvent->isText)			// Only care for binary data
		return true;

	static std::vector<uint8> buffer;
	buffer.resize((size_t)webSocketEvent->numBytes);
	memcpy(&buffer[0], webSocketEvent->data, webSocketEvent->numBytes);
	return mConnection.receivedWebSocketPacket(buffer);
}

#endif
