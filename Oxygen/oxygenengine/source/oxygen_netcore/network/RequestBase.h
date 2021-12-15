/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/HighLevelPacketBase.h"

class NetConnection;


namespace highlevel
{

	class RequestBase
	{
	friend class ::NetConnection;

	public:
		enum class State
		{
			PENDING,
			SUCCESS,
			FAILED
		};

	public:
		virtual ~RequestBase();

		inline State getState() const	{ return mState; }
		inline bool hasResponse() const	{ return (mState != State::PENDING); }
		inline bool hasError() const	{ return (mState == State::FAILED); }

	protected:
		// Only for access by NetConnection
		virtual PacketBase& getQueryPacket() = 0;
		virtual PacketBase& getResponsePacket() = 0;

	private:
		NetConnection* mRegisteredAtConnection = nullptr;
		uint32 mUniqueRequestID = 0;	// This is just the unique packet ID used for the query when it gets sent
		State mState = State::PENDING;
	};


	// Excuse the quite ugly macro here, but it makes definitions of request classes SO MUCH more compact and less prone to mistakes
	#define HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY(_classname_) \
		public: \
			struct Query : public highlevel::PacketBase, public QueryData \
			{ \
				HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE(_classname_ "::Query") \
				virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override  { serializeData(serializer, protocolVersion); } \
 			}; \
			struct Response : public highlevel::PacketBase, public ResponseData \
			{ \
				HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE(_classname_ "::Response") \
				virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override  { serializeData(serializer, protocolVersion); } \
			}; \
			\
			Query mQuery; \
			Response mResponse; \
			\
		protected: \
			inline virtual highlevel::PacketBase& getQueryPacket() override	{ return mQuery; } \
			inline virtual highlevel::PacketBase& getResponsePacket() override	{ return mResponse; } \

}
