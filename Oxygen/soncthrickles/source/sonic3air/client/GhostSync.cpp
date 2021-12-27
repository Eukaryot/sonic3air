/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/GhostSync.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/helper/GameUtils.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/NetConnection.h"

#include "oxygen/simulation/EmulatorInterface.h"


namespace
{
	static const constexpr uint32 GHOSTSYNC_BROADCAST_MESSAGE_TYPE = rmx::compileTimeFNV_32("S3AIR_GhostSync");
	static const constexpr uint8 GHOSTSYNC_BROADCAST_MESSAGE_VERSION = 1;
}


bool GhostSync::isActive() const
{
	return (mState == State::JOINED_CHANNEL);
}

void GhostSync::performUpdate()
{
	if (mState == State::INACTIVE)
		return;

	switch (mState)
	{
		case State::READY_TO_JOIN:
		{
			const char* subChannelName = getDesiredSubChannelName();
			if (nullptr != subChannelName)
			{
				// Join channel
				mJoinChannelRequest.mQuery.mChannelName = "sonic3air-ghostsync-" + ConfigurationImpl::instance().mGameServer.mGhostSync.mChannelName + "-" + subChannelName;
				mJoinChannelRequest.mQuery.mChannelHash = (uint32)rmx::getMurmur2_64(mJoinChannelRequest.mQuery.mChannelName);
				mServerConnection.sendRequest(mJoinChannelRequest);

				mJoiningSubChannelName = subChannelName;
				mState = State::JOINING_CHANNEL;
			}
			break;
		}

		case State::JOINING_CHANNEL:
		{
			if (mJoinChannelRequest.hasResponse())
			{
				if (mJoinChannelRequest.hasSuccess() && mJoinChannelRequest.mResponse.mSuccessful)
				{
					mState = State::JOINED_CHANNEL;
					mJoinedChannelHash = mJoinChannelRequest.mQuery.mChannelHash;
					mGhostPlayers.clear();
				}
				else
				{
					mState = State::FAILED;
				}
			}
			break;
		}

		case State::JOINED_CHANNEL:
		{
			// Check if the channel changed
			const char* subChannelName = getDesiredSubChannelName();
			if (mJoiningSubChannelName != subChannelName)
			{
				mLeaveChannelRequest.mQuery.mChannelHash = mJoinedChannelHash;
				mServerConnection.sendRequest(mLeaveChannelRequest);

				mJoiningSubChannelName = nullptr;
				mState = State::LEAVING_CHANNEL;
			}
			break;
		}

		case State::LEAVING_CHANNEL:
		{
			if (mLeaveChannelRequest.hasResponse())
			{
				mJoinedChannelHash = 0;
				mGhostPlayers.clear();
				mState = State::READY_TO_JOIN;

				// Update once again right away
				performUpdate();
			}
			break;
		}

		default:
			break;
	}
}

void GhostSync::evaluateServerFeaturesResponse(const network::GetServerFeaturesRequest& request)
{
	bool supportsUpdate = false;
	for (const network::GetServerFeaturesRequest::Response::Feature& feature : request.mResponse.mFeatures)
	{
		if (feature.mIdentifier == "channel-broadcasting" && feature.mVersions.contains(1))
		{
			supportsUpdate = true;
		}
	}

	if (supportsUpdate && ConfigurationImpl::instance().mGameServer.mGhostSync.mEnabled)
	{
		if (mState == State::INACTIVE)
		{
			// TODO: Leave channel if already joined one
			mState = State::READY_TO_JOIN;
		}
	}
	else
	{
		mState = State::INACTIVE;
	}
}

bool GhostSync::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::ChannelMessagePacket::PACKET_TYPE:
		{
			network::ChannelMessagePacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			// Ignore messages of the wrong type or with an unsupported version
			if (packet.mMessageType != GHOSTSYNC_BROADCAST_MESSAGE_TYPE || packet.mMessageVersion != GHOSTSYNC_BROADCAST_MESSAGE_VERSION)
				return false;

			PlayerData* playerData = nullptr;
			{
				const auto it = mGhostPlayers.find(packet.mSendingPlayerID);
				if (it == mGhostPlayers.end())
				{
					// Limit to a sane number of players
					if (mGhostPlayers.size() >= 32)
						return true;
					playerData = &mGhostPlayers[packet.mSendingPlayerID];
					playerData->mPlayerID = packet.mSendingPlayerID;
				}
				else
				{
					playerData = &it->second;
				}
			}

			VectorBinarySerializer serializer(true, packet.mMessage);
			const size_t count = (size_t)serializer.read<uint8>();
			if (count > 12)
				return true;

			while (playerData->mGhostDataQueue.size() + count > 12)
			{
				playerData->mGhostDataQueue.pop_front();
			}
			for (size_t k = 0; k < count; ++k)
			{
				playerData->mGhostDataQueue.emplace_back();
				serializeGhostData(serializer, playerData->mGhostDataQueue.back());
				playerData->mGhostDataQueue.back().mValid = true;
			}
			
			return true;
		}
	}
	return false;
}

void GhostSync::onPostUpdateFrame()
{
	if (mState != State::JOINED_CHANNEL)
		return;

	// Nothing to do outside of main game
	const bool isMainGame = (EmulatorInterface::instance().readMemory8(0xfffff600) == 0x0c);
	const bool hasStarted = ((int8)EmulatorInterface::instance().readMemory8(0xfffff711) > 0);

	if (isMainGame && hasStarted)
	{
		updateSending();
		updateGhostPlayers();
	}
}

void GhostSync::updateSending()
{
	// Collect data from simulation
	EmulatorInterface& emulatorInterface = EmulatorInterface::instance();
	mOwnGhostData.mCharacter = emulatorInterface.readMemory8(0xffffb038);
	mOwnGhostData.mZoneAndAct = emulatorInterface.readMemory16(0xffffee4e);
	mOwnGhostData.mFrameCounter = emulatorInterface.readMemory16(0xfffffe04);
	mOwnGhostData.mPosition.x = emulatorInterface.readMemory16(0xffffb010);
	mOwnGhostData.mPosition.y = emulatorInterface.readMemory16(0xffffb014);
	mOwnGhostData.mSprite = emulatorInterface.readMemory16(0x801002);
	mOwnGhostData.mRotation = emulatorInterface.readMemory8(0xffffb026);

	const float vx = (float)(int16)emulatorInterface.readMemory16(0xffffb018);
	const float vy = (float)(int16)emulatorInterface.readMemory16(0xffffb01a);
	mOwnGhostData.mMoveDirection = (uint8)roundToInt(std::atan2(vy, vx) * 128.0f / PI_FLOAT);

	// Set flags accordingly
	mOwnGhostData.mFlags = (emulatorInterface.readMemory8(0xffffb004) & 0x03);
	if (emulatorInterface.readMemory16(0xffffb00a) & 0x8000)					 { mOwnGhostData.mFlags |= GhostData::FLAG_PRIORITY; }
	if (emulatorInterface.readMemory8(0xffffb046) == 0x0e)						 { mOwnGhostData.mFlags |= GhostData::FLAG_LAYER; }
	if (emulatorInterface.readMemory16(0xfffffe10) != mOwnGhostData.mZoneAndAct) { mOwnGhostData.mFlags |= GhostData::FLAG_ACT_TRANSITION; }

	mOwnUnsentGhostData.emplace_back(mOwnGhostData);
	if (mOwnUnsentGhostData.size() >= 6)
	{
		// Send ghost data for the last frames
		while (mOwnUnsentGhostData.size() > 6)
			mOwnUnsentGhostData.pop_front();

		network::BroadcastChannelMessagePacket& packet = mBroadcastChannelMessagePacket;
		packet.mMessage.clear();
		VectorBinarySerializer serializer(false, packet.mMessage);

		serializer.writeAs<uint8>(mOwnUnsentGhostData.size());
		for (GhostData& ghostData : mOwnUnsentGhostData)
		{
			serializeGhostData(serializer, ghostData);
		}

		packet.mIsReplicatedData = false;
		packet.mChannelHash = mJoinedChannelHash;
		packet.mMessageType = GHOSTSYNC_BROADCAST_MESSAGE_TYPE;
		packet.mMessageVersion = GHOSTSYNC_BROADCAST_MESSAGE_VERSION;
		mServerConnection.sendPacket(packet, NetConnection::SendFlags::UNRELIABLE);

		mOwnUnsentGhostData.clear();
	}
}

void GhostSync::updateGhostPlayers()
{
	if (mGhostPlayers.empty())
		return;

	// TODO: Check own player's state and whether showing others even makes sense

	EmulatorInterface& emulatorInterface = EmulatorInterface::instance();

	std::vector<uint32> playersToRemove;
	for (auto& pair : mGhostPlayers)
	{
		PlayerData& playerData = pair.second;
		if (playerData.mGhostDataQueue.empty())
		{
			// Draw last frame's ghost data once again, if that one is valid
			if (!playerData.mShownGhostData.mValid)
				continue;

			// Remove ghost without any updates after some seconds
			++playerData.mTimeout;
			if (playerData.mTimeout > 10 * 60)
			{
				playersToRemove.push_back(pair.first);
				continue;
			}
		}
		else
		{
			playerData.mShownGhostData = playerData.mGhostDataQueue.front();
			playerData.mGhostDataQueue.pop_front();
		}

		const GhostData& ghostData = playerData.mShownGhostData;
		if (ghostData.mZoneAndAct != mOwnGhostData.mZoneAndAct)
		{
			// Don't show a ghost, as player is in a different zone or (apparent) act
			continue;
		}

		int px = ghostData.mPosition.x - emulatorInterface.readMemory16(0xffffee80);
		int py = ghostData.mPosition.y - emulatorInterface.readMemory16(0xffffee84);

		// Level section fixes for AIZ 1 and ICZ 1
		if ((ghostData.mZoneAndAct & 0xff00) == 0x0000)
		{
			if (ghostData.mFlags & 0x10)
			{
				px += 0x2f00;
				py += 0x80;
			}
			if (mOwnGhostData.mFlags & 0x10)
			{
				px -= 0x2f00;
				py -= 0x80;
			}
		}
		else if ((ghostData.mZoneAndAct & 0xff00) == 0x0500)
		{
			if (ghostData.mFlags & 0x10)
			{
				px += 0x6880;
				py -= 0x100;
			}
			if (mOwnGhostData.mFlags & 0x10)
			{
				px -= 0x6880;
				py += 0x100;
			}
		}

		// Consider vertical level wrap
		if (emulatorInterface.readMemory16(0xffffee18) != 0)
		{
			const int levelHeightBitmask = emulatorInterface.readMemory16(0xffffeeaa);
			py &= levelHeightBitmask;
			if (py >= levelHeightBitmask / 2)
				py -= (levelHeightBitmask + 1);
		}

		const float moveDirAngle = (float)ghostData.mMoveDirection / 128.0f * PI_FLOAT;
		const bool enableOffscreen = ConfigurationImpl::instance().mGameServer.mGhostSync.mShowOffscreenGhosts;
		s3air::drawPlayerSprite(emulatorInterface, ghostData.mCharacter, Vec2i(px, py), moveDirAngle, ghostData.mSprite, ghostData.mFlags & 0x0f, ghostData.mRotation, Color(1.5f, 1.5f, 1.5f, 0.65f), &ghostData.mFrameCounter, enableOffscreen);
	}

	for (uint32 id : playersToRemove)
	{
		mGhostPlayers.erase(id);
	}
}

const char* GhostSync::getDesiredSubChannelName() const
{
	EmulatorInterface& emulatorInterface = EmulatorInterface::instance();
	const uint16 zoneAndAct = emulatorInterface.readMemory16(0xffffee4e);

	const SharedDatabase::Zone* zone = SharedDatabase::getZoneByInternalIndex(zoneAndAct >> 8);
	if (nullptr != zone)
	{
		return zone->mInitials.c_str();		// This stays valid, so it's safe to return a const char*
	}
	return nullptr;
}

void GhostSync::serializeGhostData(VectorBinarySerializer& serializer, GhostData& ghostData)
{
	serializer.serialize(ghostData.mCharacter);
	serializer.serialize(ghostData.mZoneAndAct);

	if (ghostData.mZoneAndAct != 0xffff)
	{
		serializer.serializeAs<int16>(ghostData.mFrameCounter);
		serializer.serializeAs<int16>(ghostData.mPosition.x);
		serializer.serializeAs<int16>(ghostData.mPosition.y);

		if (ghostData.mCharacter == 1)	// Only for Tails
		{
			serializer.serialize(ghostData.mMoveDirection);
		}
		else if (serializer.isReading())
		{
			ghostData.mMoveDirection = 0;
		}

		serializer.serialize(ghostData.mSprite);
		serializer.serialize(ghostData.mRotation);
		serializer.serialize(ghostData.mFlags);
	}
}
