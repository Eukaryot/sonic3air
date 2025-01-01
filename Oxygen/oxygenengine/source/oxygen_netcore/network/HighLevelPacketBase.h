/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace highlevel
{

	// All high-level packets are built upon "lowlevel::HighLevelPacket" as their shared header
	//  -> Yes, that name sounds a bit dumb, bit it's a low-level packet after all...

	struct PacketBase
	{
	friend struct PacketTypeRegistration;

	public:
		bool serializePacket(VectorBinarySerializer& serializer, uint8 protocolVersion)
		{
			serializeContent(serializer, protocolVersion);
			return !serializer.hasError();
		}

	public:
		virtual uint32 getPacketType() const = 0;
		virtual bool isReliablePacket() const { return true; }

	protected:
		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) = 0;

	};


	struct PacketTypeRegistration
	{
		inline PacketTypeRegistration(uint32 packetType, const std::string& packetName)
		{
			RMX_ASSERT(mPacketTypeRegistry.count(packetType) == 0, "Multiple definitions of packet type '" << packetName << "'");
			mPacketTypeRegistry[packetType] = packetName;
		}

	private:
		static inline std::unordered_map<uint32, std::string> mPacketTypeRegistry;
	};

}


#define HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE(_name_) \
	public: \
		static inline const std::string PACKET_NAME = _name_; \
		static const constexpr uint32 PACKET_TYPE = rmx::compileTimeFNV_32(_name_); \
		virtual uint32 getPacketType() const override  { return PACKET_TYPE; } \
	private: \
		static inline highlevel::PacketTypeRegistration mPacketTypeRegistration { PACKET_TYPE, PACKET_NAME }; \
	public:
