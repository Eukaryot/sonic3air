/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/Sockets.h"


namespace lowlevel
{
	// Note that for all low-level packets, an additional six bytes is sent preceding the actual packet-specific content:
	//  - the packet signature
	//  - the connection IDs (local and remote, as seen from the sender's point of view)

	struct PacketBase
	{
		virtual uint16 getSignature() const = 0;
		virtual bool serializeContent(VectorBinarySerializer& serializer) = 0;
	};


	struct StartConnectionPacket : public PacketBase
	{
		// In the complete UDP packet, the first two bytes are the packet signature; this is missing here
		int16 mLowLevelProtocolVersion = 0;
		int16 mHighLevelProtocolVersion = 0;

		static const constexpr uint16 SIGNATURE = 0x87a1;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual bool serializeContent(VectorBinarySerializer& serializer) override
		{
			serializer.serialize(mLowLevelProtocolVersion);
			serializer.serialize(mHighLevelProtocolVersion);
			return !serializer.hasError();
		}
	};


	struct AcceptConnectionPacket : public PacketBase
	{
		int16 mLowLevelProtocolVersion = 0;
		int16 mHighLevelProtocolVersion = 0;

		static const constexpr uint16 SIGNATURE = 0x1b22;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual bool serializeContent(VectorBinarySerializer& serializer) override
		{
			serializer.serialize(mLowLevelProtocolVersion);
			serializer.serialize(mHighLevelProtocolVersion);
			return !serializer.hasError();
		}
	};

}
