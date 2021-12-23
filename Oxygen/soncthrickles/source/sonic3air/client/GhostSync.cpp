/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/GhostSync.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/helper/GameUtils.h"

#include "oxygen_netcore/network/ConnectionListener.h"
#include "oxygen_netcore/network/NetConnection.h"

#include "oxygen/simulation/EmulatorInterface.h"


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
		case State::JOINING_CHANNEL:
		{
			if (mJoinChannelRequest.mResponse.mSuccessful)
			{
				mState = State::JOINED_CHANNEL;
				mJoinedChannelHash = mJoinChannelRequest.mQuery.mChannelHash;
				mJoinedSubChannelHash = mJoinChannelRequest.mQuery.mSubChannelHash;
				mGhostPlayers.clear();
			}
			else
			{
				mState = State::FAILED;
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
		if (feature.mIdentifier == "channel-broadcasting" && feature.mVersion >= 1)
		{
			supportsUpdate = true;
		}
	}

	if (supportsUpdate && ConfigurationImpl::instance().mGameServer.mEnableGhostSync)
	{
		if (mState == State::INACTIVE)
		{
			// Join channel
			mJoinChannelRequest.mQuery.mChannelName = "sonic3air-ghostsync-world";
			mJoinChannelRequest.mQuery.mChannelHash = (uint32)rmx::getMurmur2_64(mJoinChannelRequest.mQuery.mChannelName);
			mJoinChannelRequest.mQuery.mSubChannelName.clear();
			mJoinChannelRequest.mQuery.mSubChannelHash = 0;
			mServerConnection.sendRequest(mJoinChannelRequest);
			mState = State::JOINING_CHANNEL;
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
	const bool isMainGame = true;//(EmulatorInterface::instance().readMemory8(0xfffff600) == 0x0c);		// TODO: Out-commented just for testing with the demos
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
	mOwnGhostData.mCharacter = EmulatorInterface::instance().readMemory8(0xffffb038);
	mOwnGhostData.mZoneAndAct = EmulatorInterface::instance().readMemory16(0xffffee4e);
	mOwnGhostData.mPosition.x = EmulatorInterface::instance().readMemory16(0xffffb010);
	mOwnGhostData.mPosition.y = EmulatorInterface::instance().readMemory16(0xffffb014);
	mOwnGhostData.mSprite = EmulatorInterface::instance().readMemory16(0x801002);
	mOwnGhostData.mRotation = EmulatorInterface::instance().readMemory8(0xffffb026);

	// Set flags accordingly
	mOwnGhostData.mFlags = (EmulatorInterface::instance().readMemory8(0xffffb004) & 0x03);
	if (EmulatorInterface::instance().readMemory16(0xffffb00a) & 0x8000)					 { mOwnGhostData.mFlags |= GhostData::FLAG_PRIORITY; }
	if (EmulatorInterface::instance().readMemory8(0xffffb046) == 0x0e)						 { mOwnGhostData.mFlags |= GhostData::FLAG_LAYER; }
	if (EmulatorInterface::instance().readMemory16(0xfffffe10) != mOwnGhostData.mZoneAndAct) { mOwnGhostData.mFlags |= GhostData::FLAG_ACT_TRANSITION; }

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
		packet.mSubChannelHash = mJoinedSubChannelHash;
		mServerConnection.sendPacket(packet, NetConnection::SendFlags::UNRELIABLE);

		mOwnUnsentGhostData.clear();
	}
}

void GhostSync::updateGhostPlayers()
{
	// TODO: Check own player's state and whether showing others even makes sense

	for (auto& pair : mGhostPlayers)
	{
		PlayerData& playerData = pair.second;

		// TODO: Filter out players in other zones

		if (playerData.mGhostDataQueue.empty())
			continue;

		playerData.mShownGhostData = playerData.mGhostDataQueue.front();
		playerData.mGhostDataQueue.pop_front();

		const GhostData& ghostData = playerData.mShownGhostData;
		if (ghostData.mZoneAndAct != mOwnGhostData.mZoneAndAct)
		{
			// Don't show a ghost, as player is in a different zone or (apparent) act
			continue;
		}

		int px = ghostData.mPosition.x - EmulatorInterface::instance().readMemory16(0xffffee80);
		int py = ghostData.mPosition.y - EmulatorInterface::instance().readMemory16(0xffffee84);

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
		if (EmulatorInterface::instance().readMemory16(0xffffee18) != 0)
		{
			const int levelHeightBitmask = EmulatorInterface::instance().readMemory16(0xffffeeaa);
			py &= levelHeightBitmask;
			if (py >= levelHeightBitmask / 2)
				py -= (levelHeightBitmask + 1);
		}

		uint16 frameNumber = 0;	// TODO
		const Vec2i velocity = /*(frameNumber > 0) ? (ghostData.mPosition - recording.mFrames[frameNumber-1].mPosition) :*/ Vec2i(0, 1);	// TODO
		s3air::drawPlayerSprite(EmulatorInterface::instance(), ghostData.mCharacter, Vec2i(px, py), velocity, ghostData.mSprite, ghostData.mFlags & 0x0f, ghostData.mRotation, Color(1.5f, 1.5f, 1.5f, 0.65f), &frameNumber);
	}
}

void GhostSync::serializeGhostData(VectorBinarySerializer& serializer, GhostData& ghostData)
{
	serializer.serialize(ghostData.mCharacter);
	serializer.serialize(ghostData.mZoneAndAct);
	if (ghostData.mZoneAndAct != 0xffff)
	{
		serializer.serializeAs<int16>(ghostData.mPosition.x);
		serializer.serializeAs<int16>(ghostData.mPosition.y);
		serializer.serialize(ghostData.mSprite);
		serializer.serialize(ghostData.mRotation);
		serializer.serialize(ghostData.mFlags);
	}
}
