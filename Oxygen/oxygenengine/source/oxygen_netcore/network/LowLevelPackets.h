/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/VersionRange.h"


namespace lowlevel
{
	// Note that for all low-level packets, an additional six bytes is sent preceding the actual packet-specific content:
	//  - the packet signature
	//  - the connection IDs (local and remote, as seen from the sender's point of view)

	struct PacketBase
	{
	public:
		static const constexpr VersionRange<uint8> LOWLEVEL_PROTOCOL_VERSIONS { 1, 1 };

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
		VersionRange<uint8> mLowLevelProtocolVersionRange;
		VersionRange<uint8> mHighLevelProtocolVersionRange;

		static const constexpr uint16 SIGNATURE = 0x87a1;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			mLowLevelProtocolVersionRange.serialize(serializer);
			mHighLevelProtocolVersionRange.serialize(serializer);
		}
	};


	struct AcceptConnectionPacket : public PacketBase
	{
		uint8 mLowLevelProtocolVersion = 0;
		uint8 mHighLevelProtocolVersion = 0;

		static const constexpr uint16 SIGNATURE = 0x1b22;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mLowLevelProtocolVersion);
			serializer.serialize(mHighLevelProtocolVersion);
		}
	};


	struct ErrorPacket : public PacketBase
	{
		enum class ErrorCode : uint8
		{
			// The errors marking with (*) are sent without an actual establishes connection - that means they do not include a proper local connection ID and just re-use the received remote connection ID
			UNKNOWN					= 0,
			CONNECTION_INVALID		= 1,	// (*) Received a packet with an unknown connection ID
			UNSUPPORTED_VERSION		= 2,	// (*) Received a start connection packet that uses protocol versions that can't be supported
			TOO_MANY_CONNECTIONS	= 3,	// (*) Remote server / client has too many active connections already
		};
		ErrorCode mErrorCode = ErrorCode::UNKNOWN;
		uint32 mParameter = 0;

		static const constexpr uint16 SIGNATURE = 0xf584;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		inline ErrorPacket() {}
		inline ErrorPacket(ErrorCode errorCode, uint32 parameter = 0) : mErrorCode(errorCode), mParameter(parameter) {}

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serializeAs<uint8>(mErrorCode);
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
