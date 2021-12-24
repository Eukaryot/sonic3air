/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/RequestBase.h"


// Concrete packets for communication between Oxygen server and client
namespace network
{

	class GetServerFeaturesRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			// No parameters at all

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
			}
		};

		struct ResponseData
		{
			struct Feature
			{
				std::string mIdentifier;
				uint8 mVersion = 1;

				inline Feature() {}
				inline Feature(const char* identifier, uint8 version) : mIdentifier(identifier), mVersion(version) {}
			};
			std::vector<Feature> mFeatures;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serializeArraySize(mFeatures, 0xff);
				for (Feature& feature : mFeatures)
				{
					serializer.serialize(feature.mIdentifier, 0xff);
					serializer.serialize(feature.mVersion);
				}
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("GetServerFeaturesRequest")
	};


	class AppUpdateCheckRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			std::string mAppName;
			std::string mPlatform;
			std::string mReleaseChannel;
			uint32 mInstalledAppVersion = 0;
			uint32 mInstalledContentVersion = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mAppName, 0xff);
				serializer.serialize(mPlatform, 0xff);
				serializer.serialize(mReleaseChannel, 0xff);
				serializer.serialize(mInstalledAppVersion);
				serializer.serialize(mInstalledContentVersion);
			}
		};

		struct ResponseData
		{
			bool mHasUpdate = false;
			uint32 mAvailableAppVersion = 0;
			uint32 mAvailableContentVersion = 0;
			std::string mUpdateInfoURL;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mHasUpdate);
				if (mHasUpdate)
				{
					serializer.serialize(mAvailableAppVersion);
					serializer.serialize(mAvailableContentVersion);
					serializer.serialize(mUpdateInfoURL, 0xff);
				}
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("AppUpdateCheck")
	};


	class JoinChannelRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			uint32 mChannelHash = 0;
			std::string mChannelName;
			uint32 mSubChannelHash = 0;
			std::string mSubChannelName;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mChannelHash);
				serializer.serialize(mChannelName, 0xff);
				serializer.serialize(mSubChannelHash);
				serializer.serialize(mSubChannelName, 0xff);
			}
		};

		struct ResponseData
		{
			bool mSuccessful = true;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mSuccessful);
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("JoinChannelRequest")
	};


	class LeaveChannelRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			uint32 mChannelHash = 0;
			uint32 mSubChannelHash = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mChannelHash);
				serializer.serialize(mSubChannelHash);
			}
		};

		struct ResponseData
		{
			// No response data

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("LeaveChannelRequest")
	};


	class GetChannelContent : public highlevel::RequestBase
	{
		struct QueryData
		{
			uint32 mChannelHash = 0;
			uint32 mSubChannelHash = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mChannelHash);
				serializer.serialize(mSubChannelHash);
			}
		};

		struct ResponseData
		{
			bool mSuccess = true;
			struct PlayerInfo
			{
				uint32 mPlayerID = 0;
				std::vector<uint8> mChannelReplicatedData;
				std::vector<uint8> mSubChannelReplicatedData;
			};
			std::vector<PlayerInfo> mPlayers;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mSuccess);
				if (mSuccess)
				{
					serializer.serializeArraySize(mPlayers, 16);
					for (PlayerInfo& player : mPlayers)
					{
						serializer.serialize(player.mPlayerID);
						serializer.serializeData(player.mChannelReplicatedData, 0x400);
						serializer.serializeData(player.mSubChannelReplicatedData, 0x400);
					}
				}
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("GetChannelContent")
	};


	// Sent from client to server
	struct BroadcastChannelMessagePacket : public highlevel::PacketBase
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("BroadcastChannelMessagePacket");

		bool mIsReplicatedData = false;		// If true, this is replicated data that gets cached on the server; otherwise it's just a message to broadcast
		uint32 mChannelHash = 0;
		uint32 mSubChannelHash = 0;			// Can stay 0 if posting to the main channel
		uint32 mMessageType = 0;
		uint8 mMessageVersion = 0;
		std::vector<uint8> mMessage;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mIsReplicatedData);
			serializer.serialize(mChannelHash);
			serializer.serialize(mSubChannelHash);
			serializer.serialize(mMessageType);
			serializer.serialize(mMessageVersion);
			serializer.serializeData(mMessage, 0x400);
		}
	};


	// Sent from server to multiple clients
	struct ChannelMessagePacket : public BroadcastChannelMessagePacket
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("ChannelMessagePacket");

		// All properties from "BroadcastChannelMessagePacket" are shared here
		uint32 mSendingPlayerID = 0;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			BroadcastChannelMessagePacket::serializeContent(serializer, protocolVersion);
			serializer.serialize(mSendingPlayerID);
		}
	};

}
