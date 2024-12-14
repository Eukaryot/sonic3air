/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/RequestBase.h"


// Packets for communication between Oxygen server and client
namespace network
{

	enum class NetplayConnectionType
	{
		DIRECT		 = 0,		// Direct connection without NAT punchthrough or anything; can be used if host and client are in the same network anyways
		PUNCHTHROUGH = 1,		// Connect by performing NAT punchthrough first
		RELAYED		 = 2,		// The whole connection is relayed via the server
	};


	// Sent from the netplay host to the server
	class RegisterNetplayHostPacket : public highlevel::PacketBase
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("RegisterNetplayHostPacket");

		uint64 mSessionID = 0;
		std::string mGameSocketExternalIP;
		uint16 mGameSocketExternalPort = 0;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mSessionID);
			serializer.serialize(mGameSocketExternalIP, 64);
			serializer.serialize(mGameSocketExternalPort);
		}
	};


	// Sent from the netplay client to the server
	class RegisterNetplayClientPacket : public highlevel::PacketBase
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("RegisterNetplayClientPacket");

		uint64 mSessionID = 0;
		std::string mGameSocketExternalIP;
		uint16 mGameSocketExternalPort = 0;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mSessionID);
			serializer.serialize(mGameSocketExternalIP, 64);
			serializer.serialize(mGameSocketExternalPort);
		}
	};


	// Sent from the server to the netplay client
	class ConnectToNetplayPacket : public highlevel::PacketBase
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("ConnectToNetplayPacket");

		uint64 mSessionID = 0;
		NetplayConnectionType mConnectionType = NetplayConnectionType::DIRECT;
		std::string mConnectToIP;
		uint16 mConnectToPort = 0;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mSessionID);
			serializer.serializeAs<uint8>(mConnectionType);
			serializer.serialize(mConnectToIP, 64);
			serializer.serialize(mConnectToPort);
		}
	};

}
