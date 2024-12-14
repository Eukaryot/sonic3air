/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"

class ServerNetConnection;


class Channels
{
public:
	struct PlayerData
	{
		ServerNetConnection* mServerNetConnection = nullptr;
		std::vector<uint8> mReplicatedData;
	};

	struct Channel
	{
		uint32 mID = 0;
		std::string mName;
		std::vector<PlayerData> mPlayers;
	};

public:
	bool onReceivedPacket(ReceivedPacketEvaluation& evaluation);
	bool onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation);

	Channel* findChannel(uint32 channelID);
	Channel& createChannel(uint32 channelID, const std::string& channelName);
	void destroyChannel(Channel& channel);

	void addPlayerToChannel(Channel& channel, ServerNetConnection& playerConnection);
	void removePlayerFromAllChannels(ServerNetConnection& playerConnection);
	bool removePlayerFromSingleChannel(Channel& channel, ServerNetConnection& playerConnection);
	void cleanupEmptyChannels();

private:
	std::unordered_map<uint32, Channel*> mAllChannels;	// Key is the channel ID
	std::vector<Channel*> mPossiblyEmptyChannels;		// These channels will be destroyed on cleanup if still empty by then
	ObjectPool<Channel> mChannelPool;
};
