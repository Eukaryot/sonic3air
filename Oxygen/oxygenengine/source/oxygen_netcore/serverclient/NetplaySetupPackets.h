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


	class RegisterForNetplayRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			bool mIsHost = false;
			uint64 mSessionID = 0;
			std::string mGameSocketExternalIP;
			uint16 mGameSocketExternalPort = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mIsHost);
				serializer.serialize(mSessionID);
				serializer.serialize(mGameSocketExternalIP, 64);
				serializer.serialize(mGameSocketExternalPort);
			}
		};

		struct ResponseData
		{
			bool mSuccess = true;
			uint64 mSessionID = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mSuccess);
				serializer.serialize(mSessionID);
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("RegisterForNetplayRequest")
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


	// Sent between host and client
	struct PunchthroughConnectionlessPacket : public lowlevel::PacketBase
	{
		uint32 mQueryID = 0;
		bool mSenderReceivedPackets = false;

		static const constexpr uint16 SIGNATURE = 0x295e;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mQueryID);
			serializer.serialize(mSenderReceivedPackets);
		}
	};

}
