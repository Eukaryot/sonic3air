/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/serverclient/Packets.h"

class GameClient;
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
		bool   mValid = false;
		uint8  mCharacter = 0;
		uint16 mZoneAndAct = 0;		// This is always the apparent zone and act
		uint16 mFrameCounter = 0;
		Vec2i  mPosition;
		uint16 mSprite = 0;
		uint8  mRotation = 0;
		uint8  mFlags = 0;
		uint8  mMoveDirection = 0;	// Only needed for Tails, it's not set for other characters
	};

public:
	inline GhostSync(GameClient& gameClient) : mGameClient(gameClient) {}

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
		CONNECTING,
		READY_TO_JOIN,
		JOINING_CHANNEL,
		JOINED_CHANNEL,
		LEAVING_CHANNEL,
		FAILED
	};

	struct PlayerData
	{
		uint32 mPlayerID = 0;
		std::deque<GhostData> mGhostDataQueue;
		GhostData mShownGhostData;
		int mTimeout = 0;
	};

private:
	const char* getDesiredSubChannelName() const;
	void serializeGhostData(VectorBinarySerializer& serializer, GhostData& ghostData);

private:
	GameClient& mGameClient;
	State mState = State::INACTIVE;

	network::JoinChannelRequest mJoinChannelRequest;
	network::LeaveChannelRequest mLeaveChannelRequest;
	uint32 mJoinedChannelHash = 0;
	const char* mJoiningSubChannelName = nullptr;

	GhostData mOwnGhostData;
	std::deque<GhostData> mOwnUnsentGhostData;
	network::BroadcastChannelMessagePacket mBroadcastChannelMessagePacket;

	std::unordered_map<uint32, PlayerData> mGhostPlayers;
};
