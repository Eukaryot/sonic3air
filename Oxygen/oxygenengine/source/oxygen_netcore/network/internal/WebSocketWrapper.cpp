/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen_netcore/pch.h"
#include "oxygen_netcore/network/internal/WebSocketWrapper.h"
#include "oxygen_netcore/network/internal/CryptoFunctions.h"
#include "oxygen_netcore/network/NetConnection.h"


bool WebSocketWrapper::handleWebSocketHttpHeader(const std::vector<uint8>& receivedData, String& outWebSocketKey)
{
	if (receivedData.size() < 0x80 || *(uint32*)&receivedData[0] != *(uint32*)"GET ")
		return false;

	// Parse WebSocket handshake header
	String input;
	input.add((char*)&receivedData[0], receivedData.size());

	uint8 flags = 0;
	String line;
	int pos = 0;
	while (pos < input.length())
	{
		pos = input.getLine(line, pos);
		if (line == "Upgrade: websocket")
		{
			flags |= 0x01;
		}
		else if (line.startsWith("Connection: Upgrade") || line.startsWith("Connection: keep-alive, Upgrade"))
		{
			flags |= 0x02;
		}
		else if (line.startsWith("Sec-WebSocket-Key: "))
		{
			outWebSocketKey = line.getSubString(19);
			flags |= 0x04;
		}
	}
	return ((flags & 0x07) == 0x07);
}

void WebSocketWrapper::getWebSocketHttpResponse(const String& webSocketKey, String& outResponse)
{
	// Send back the required response
	static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	uint32 sha1[5];
	Crypto::buildSHA1(webSocketKey.toStdString() + GUID, sha1);
	for (int k = 0; k < 5; ++k)
		sha1[k] = swapBytes32(sha1[k]);
	const std::string acceptString = Crypto::encodeBase64((const uint8*)sha1, sizeof(uint32) * 5);

	outResponse.clear();
	outResponse << "HTTP/1.1 101 Switching Protocols\r\n";
	outResponse << "Upgrade: websocket\r\n";
	outResponse << "Connection: Upgrade\r\n";
	outResponse << "Sec-WebSocket-Accept: " << acceptString << "\r\n";
	outResponse << "\r\n";
}

bool WebSocketWrapper::processReceivedClientPacket(std::vector<uint8>& data)
{
	// Process this "fragment" to get the actual message
	if (data.size() < 7)	// 2 bytes for the minimal header, 4 bytes for the masking key (always required on server side), at least 1 byte of payload
		return false;

	// First byte
	uint8 byte = data[0];
	if ((byte & 0x80) == 0)
	{
		// TODO: Support messages that span more than one fragment
		return false;
	}
	if ((byte & 0x70) != 0)
	{
		// Error: Unsupported format
		return false;
	}
	const uint8 opcode = (byte & 0x0f);
	// TODO: Handle the different kinds of opcodes (at least 0, 1, 2, 8)

	// Second byte
	byte = data[1];
	if ((byte & 0x80) == 0)
	{
		// Error: Data is not masked
		return false;
	}
	size_t offset = 2;
	uint64 payloadLength = (byte & 0x7f);
	if (payloadLength >= 126)
	{
		if (payloadLength == 126)
		{
			payloadLength = swapBytes16(*(uint16*)&data[offset]);
			offset += 2;
		}
		else
		{
			payloadLength = swapBytes64(*(uint64*)&data[offset]);
			offset += 8;
			// TODO: Limit length to some reasonable value
		}
	}

	if (offset + 5 >= data.size())
	{
		// Error: Data just stops unexpectedly
		return false;
	}

	uint8 maskingKey[4];
	memcpy(maskingKey, &data[offset], 4);
	offset += 4;

	data.erase(data.begin(), data.begin() + offset);
	for (size_t k = 0; k < data.size(); ++k)
		data[k] ^= maskingKey[k % 4];

	return true;
}

void WebSocketWrapper::wrapDataToSendToClient(const std::vector<uint8>& data, std::vector<uint8>& outWrappedData)
{
	uint8 headerContent[10];
	size_t headerSize = 2;
	headerContent[0] = 0x82;	// FIN bit set + opcode for binary data
	headerContent[1] = 0x00;	// Masking bit not set
	if (data.size() <= 125)
	{
		headerContent[1] += (uint8)data.size();
	}
	else if (data.size() <= 0xffff)
	{
		headerContent[1] += 126;
		*(uint16*)&headerContent[2] = (uint16)data.size();
		headerSize += 2;
	}
	else
	{
		headerContent[1] += 127;
		*(uint64*)&headerContent[2] = (uint64)data.size();
		headerSize += 8;
	}

	outWrappedData.resize(headerSize + data.size());
	memcpy(&outWrappedData[0], headerContent, headerSize);
	if (!data.empty())
		memcpy(&outWrappedData[headerSize], &data[0], data.size());
}
