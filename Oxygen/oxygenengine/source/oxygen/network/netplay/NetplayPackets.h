/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/HighLevelPacketBase.h"


// Sent from host to clients
struct StartGamePacket : public highlevel::PacketBase
{
	HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("StartGamePacket");

	static const uint8 PACKET_VERSION = 1;

	uint8 mPacketVersion = PACKET_VERSION;
	uint32 mGameBuildVersion = 0;
	uint8 mTransferMode = 0;
	uint8 mGameMode = 0;
	uint32 mFirstFrameNumber = 0;
	// TODO: Add more, like game settings, persistent data, list of active mods incl. versions

	virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
	{
		serializer.serialize(mPacketVersion);
		if (serializer.isReading() && mPacketVersion != PACKET_VERSION)
		{
			serializer.setError();
			return;
		}

		serializer.serialize(mGameBuildVersion);
		serializer.serialize(mTransferMode);
		serializer.serialize(mGameMode);
		serializer.serialize(mFirstFrameNumber);
	}
};


// Sent from host to clients
struct GameStateIncrementPacket : public highlevel::PacketBase
{
	HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("GameStateIncrementPacket");

	uint32 mFrameNumber = 0;		// Most recent frame number
	uint8 mNumFrames = 0;
	uint8 mNumPlayers = 0;
	std::vector<uint16> mInputs;	// One input per player and frame (player inputs are packed together, forming one frame)

	virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
	{
		serializer.serialize(mFrameNumber);
		serializer.serialize(mNumFrames);
		serializer.serialize(mNumPlayers);

		if (serializer.isReading())
			mInputs.resize(mNumFrames * mNumPlayers);

		for (uint16& inputs : mInputs)
			serializer.serialize(inputs);
	}
};


// Sent from clients to host
struct PlayerInputIncrementPacket : public highlevel::PacketBase
{
	HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("PlayerInputIncrementPacket");

	uint32 mFrameNumber = 0;		// Most recent frame number
	uint8 mNumFrames = 0;
	std::vector<uint16> mInputs;	// One input per frame -- TODO: Only the last fraem is actually used at the moment

	virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
	{
		serializer.serialize(mFrameNumber);
		serializer.serialize(mNumFrames);

		if (serializer.isReading())
			mInputs.resize(mNumFrames);
		
		for (uint16& inputs : mInputs)
			serializer.serialize(inputs);
	}
};
