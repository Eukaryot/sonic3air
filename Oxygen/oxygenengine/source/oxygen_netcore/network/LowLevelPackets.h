/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lowlevel
{
	// Note that for all low-level packets, an additional six bytes is sent preceding the actual packet-specific content:
	//  - the packet signature
	//  - the connection IDs (local and remote, as seen from the sender's point of view)

	struct PacketBase
	{
	public:
		static const constexpr uint8 LOWLEVEL_MINIMUM_PROTOCOL_VERSION = 1;
		static const constexpr uint8 LOWLEVEL_MAXIMUM_PROTOCOL_VERSION = 1;

	public:
		bool serializePacket(VectorBinarySerializer& serializer, uint8 protocolVersion)
		{
			serializeContent(serializer, protocolVersion);
			return !serializer.hasError();
		}

	public:
		virtual uint16 getSignature() const = 0;

	protected:
		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) = 0;
	};


	struct StartConnectionPacket : public PacketBase
	{
		int8 mLowLevelMinimumProtocolVersion = 0;
		int8 mLowLevelMaximumProtocolVersion = 0;
		int8 mHighLevelMinimumProtocolVersion = 0;
		int8 mHighLevelMaximumProtocolVersion = 0;

		static const constexpr uint16 SIGNATURE = 0x87a1;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mLowLevelMinimumProtocolVersion);
			serializer.serialize(mLowLevelMaximumProtocolVersion);
			serializer.serialize(mHighLevelMinimumProtocolVersion);
			serializer.serialize(mHighLevelMaximumProtocolVersion);
		}
	};


	struct AcceptConnectionPacket : public PacketBase
	{
		int8 mLowLevelProtocolVersion = 0;
		int8 mHighLevelProtocolVersion = 0;

		static const constexpr uint16 SIGNATURE = 0x1b22;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mLowLevelProtocolVersion);
			serializer.serialize(mHighLevelProtocolVersion);
		}
	};


	struct HighLevelPacket : public PacketBase
	{
		struct Flags
		{
			// None defined yet
		};

		// This is only the header, afterwards comes the actual packet-specific content
		uint32 mPacketType = 0;
		uint8  mPacketFlags = 0;
		uint32 mUniquePacketID = 0;

		static const constexpr uint16 SIGNATURE = 0xe994;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mPacketType);
			serializer.serialize(mPacketFlags);
			serializer.serialize(mUniquePacketID);
		}
	};


	struct RequestQueryPacket : public HighLevelPacket
	{
		// No need for any custom members here

		static const constexpr uint16 SIGNATURE = 0x0c7a;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			HighLevelPacket::serializeContent(serializer, protocolVersion);
		}
	};


	struct RequestResponsePacket : public HighLevelPacket
	{
		// Extension to the high level packet
		uint32 mUniqueRequestID = 0;

		static const constexpr uint16 SIGNATURE = 0xd028;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			HighLevelPacket::serializeContent(serializer, protocolVersion);
			serializer.serialize(mUniqueRequestID);
		}
	};


	struct ReceiveConfirmationPacket : public PacketBase
	{
		uint32 mUniquePacketID = 0;

		static const constexpr uint16 SIGNATURE = 0x276f;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mUniquePacketID);
		}
	};

}
