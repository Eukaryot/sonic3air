/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/subsystems/Channels.h"
#include "oxygenserver/server/ServerNetConnection.h"

#include "oxygen_netcore/serverclient/Packets.h"


bool Channels::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::BroadcastChannelMessagePacket::PACKET_TYPE:
		{
			network::BroadcastChannelMessagePacket packet;
			if (!evaluation.readPacket(packet))
				return false;

			ServerNetConnection& connection = static_cast<ServerNetConnection&>(evaluation.mConnection);

			// TODO: Handle "packet.mIsReplicatedData" by saving the data inside the player data

			Channel* channel = findChannel(packet.mChannelHash);
			if (nullptr == channel)
			{
				// Send back an error
				network::ChannelErrorPacket errorPacket;
				errorPacket.mErrorCode = network::ChannelErrorPacket::ErrorCode::UNKNOWN_CHANNEL;
				errorPacket.mParameter = packet.mChannelHash;
				connection.sendPacket(errorPacket, NetConnection::SendFlags::UNRELIABLE);
			}
			else
			{
				bool playerIsInChannel = false;
				for (const Channels::PlayerData& player : channel->mPlayers)
				{
					if (player.mServerNetConnection == &connection)
					{
						playerIsInChannel = true;
						break;
					}
				}

				if (!playerIsInChannel)
				{
					// Send back an error
					network::ChannelErrorPacket errorPacket;
					errorPacket.mErrorCode = network::ChannelErrorPacket::ErrorCode::CHANNEL_NOT_JOINED;
					errorPacket.mParameter = packet.mChannelHash;
					connection.sendPacket(errorPacket, NetConnection::SendFlags::UNRELIABLE);
				}
				else if (channel->mPlayers.size() >= 2)
				{
					// Prepare the packet to send
					network::ChannelMessagePacket broadcastedPacket;
					static_cast<network::BroadcastChannelMessagePacket&>(broadcastedPacket) = packet;	// Copy all the shared members
					broadcastedPacket.mSendingPlayerID = connection.getPlayerID();

					// Broadcast unreliably if that's how the message got sent to the server
					const NetConnection::SendFlags::Flags sendFlags = (evaluation.mUniquePacketID == 0) ? NetConnection::SendFlags::UNRELIABLE : NetConnection::SendFlags::NONE;

					for (const PlayerData& playerData : channel->mPlayers)
					{
						// Ignore the sending player
						if (playerData.mServerNetConnection != &connection)
						{
							playerData.mServerNetConnection->sendPacket(broadcastedPacket, sendFlags);
						}
					}
				}
			}
			return true;
		}
	}
	return false;
}

bool Channels::onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation)
{
	switch (evaluation.mPacketType)
	{
		case network::JoinChannelRequest::Query::PACKET_TYPE:
		{
			using Request = network::JoinChannelRequest;
			Request request;
			if (!evaluation.readQuery(request))
				return false;

			ServerNetConnection& connection = static_cast<ServerNetConnection&>(evaluation.mConnection);
			RMX_LOG_INFO("JoinChannelRequest: " << request.mQuery.mChannelHash << " = '" << request.mQuery.mChannelName << "' (from " << static_cast<ServerNetConnection&>(evaluation.mConnection).getHexPlayerID() << ")");

			// Find or create the channel and add the player there
			Channel* baseChannel = findChannel(request.mQuery.mChannelHash);
			if (nullptr == baseChannel)
			{
				baseChannel = &createChannel(request.mQuery.mChannelHash, request.mQuery.mChannelName);
			}
			addPlayerToChannel(*baseChannel, connection);

			// Done
			request.mResponse.mSuccessful = true;
			return evaluation.respond(request);
		}

		case network::LeaveChannelRequest::Query::PACKET_TYPE:
		{
			using Request = network::LeaveChannelRequest;
			Request request;
			if (!evaluation.readQuery(request))
				return false;

			ServerNetConnection& connection = static_cast<ServerNetConnection&>(evaluation.mConnection);
			RMX_LOG_INFO("LeaveChannelRequest: " << request.mQuery.mChannelHash << " (from " << static_cast<ServerNetConnection&>(evaluation.mConnection).getHexPlayerID() << ")");

			// Remove player from the channel
			Channel* channel = findChannel(request.mQuery.mChannelHash);
			if (nullptr != channel)
			{
				removePlayerFromSingleChannel(*channel, connection);
			}

			// Done
			return evaluation.respond(request);
		}

		// TODO: Implement "GetChannelContent" request

	}
	return false;
}

Channels::Channel* Channels::findChannel(uint32 channelID)
{
	const auto it = mAllChannels.find(channelID);
	return (it == mAllChannels.end()) ? nullptr : it->second;
}

Channels::Channel& Channels::createChannel(uint32 channelID, const std::string& channelName)
{
	Channel& channel = mChannelPool.createObject();
	channel.mID = channelID;
	channel.mName = channelName;
	channel.mPlayers.reserve(8);
	mAllChannels[channelID] = &channel;
	return channel;
}

void Channels::destroyChannel(Channel& channel)
{
	// Unregister and destroy the channel instance
	mAllChannels.erase(channel.mID);
	mChannelPool.destroyObject(channel);
}

void Channels::addPlayerToChannel(Channel& channel, ServerNetConnection& playerConnection)
{
	// TODO: The check for uniqueness could be removed if we tracked which channels a player is added to (in a map) inside the ServerNetConnection instance
	for (const PlayerData& playerData : channel.mPlayers)
	{
		if (playerData.mServerNetConnection == &playerConnection)
			return;
	}

	PlayerData& playerData = vectorAdd(channel.mPlayers);
	playerData.mServerNetConnection = &playerConnection;
}

void Channels::removePlayerFromSingleChannel(Channel& channel, ServerNetConnection& playerConnection)
{
	for (auto it = channel.mPlayers.begin(); it != channel.mPlayers.end(); ++it)
	{
		if (it->mServerNetConnection == &playerConnection)
		{
			channel.mPlayers.erase(it);

			// Is channel empty now?
			if (channel.mPlayers.empty())
			{
				destroyChannel(channel);
			}
			break;
		}
	}
}
