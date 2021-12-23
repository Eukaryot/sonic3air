/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/serverclient/Packets.h"

struct ReceivedPacketEvaluation;


class GhostSync
{
public:
	struct GhostData
	{
		enum Flag : uint8
		{
			FLAG_FLIP_X = 0x01,			// From "sonic.render_flags"
			FLAG_FLIP_Y = 0x02,			// From "sonic.render_flags"
			FLAG_PRIORITY = 0x04,		// From "sonic.sprite_attributes", flag 0x8000
			FLAG_LAYER = 0x08,			// From "u8[A0 + 0x46]", which is either 0x0c or 0x0e
			FLAG_ACT_TRANSITION = 0x10	// Used when act number and apparent act number differ
		};
		uint8  mCharacter = 0;
		uint16 mZoneAndAct = 0;		// This is always the apparent zone and act
		Vec2i  mPosition;
		uint16 mSprite = 0;
		uint8  mRotation = 0;
		uint8  mFlags = 0;
	};

public:
	inline GhostSync(NetConnection& serverConnection) : mServerConnection(serverConnection) {}

	bool isActive() const;

	void performUpdate();
	void evaluateServerFeaturesResponse(const network::GetServerFeaturesRequest& request);
	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);

	void onPostUpdateFrame();
	void updateSending();
	void updateGhostPlayers();

private:
	enum class State
	{
		INACTIVE,
		JOINING_CHANNEL,
		JOINED_CHANNEL,
		FAILED
	};

	struct PlayerData
	{
		uint32 mPlayerID;
		std::deque<GhostData> mGhostDataQueue;
		GhostData mShownGhostData;
	};

private:
	void serializeGhostData(VectorBinarySerializer& serializer, GhostData& ghostData);

private:
	NetConnection& mServerConnection;
	State mState = State::INACTIVE;

	network::JoinChannelRequest mJoinChannelRequest;
	uint32 mJoinedChannelHash = 0;
	uint32 mJoinedSubChannelHash = 0;

	GhostData mOwnGhostData;
	std::deque<GhostData> mOwnUnsentGhostData;
	network::BroadcastChannelMessagePacket mBroadcastChannelMessagePacket;

	std::unordered_map<uint32, PlayerData> mGhostPlayers;
};
