/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/RequestBase.h"


// Packets for communication between Oxygen server and client
namespace network
{

	// Request to start a file download
	class FileDownloadRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			std::string mFilePath;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mFilePath, 0xff);
			}
		};

		struct ResponseData
		{
			struct ChunkInfo
			{
				uint32 mChunkSize = 0;
				uint64 mChunkHash = 0;
			};
			bool mFileAvailable = false;
			uint32 mTransferHandle = 0;
			uint32 mFileSize = 0;
			uint64 mFileHash = 0;
			std::vector<ChunkInfo> mChunks;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mFileAvailable);
				if (mFileAvailable)
				{
					serializer.serialize(mTransferHandle);
					serializer.serialize(mFileSize);
					serializer.serialize(mFileHash);
					serializer.serializeArraySize(mChunks, 0x1000);
					for (ChunkInfo& chunk : mChunks)
					{
						serializer.serialize(chunk.mChunkSize);
						serializer.serialize(chunk.mChunkHash);
					}
				}
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("FileDownloadRequest")
	};


	// Request new pieces of a file download
	struct FileTransferRequestPiecesPacket : public highlevel::PacketBase
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("FileTransferRequestPiecesPacket");

		struct PieceInfo
		{
			uint16 mChunkIndex = 0;
			uint32 mStartOffset = 0;	// Relative address inside chunk
			uint32 mSize = 0;
		};

		uint32 mTransferHandle = 0;
		bool mTransferComplete = false;
		std::vector<PieceInfo> mRequestedPieces;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mTransferHandle);
			serializer.serialize(mTransferComplete);
			if (!mTransferComplete)
			{
				serializer.serializeArraySize(mRequestedPieces, 0x40);
				for (PieceInfo& pieceInfo : mRequestedPieces)
				{
					serializer.serialize(pieceInfo.mChunkIndex);
					serializer.serialize(pieceInfo.mStartOffset);
					serializer.serialize(pieceInfo.mSize);
				}
			}
		}
	};


	// Transfer a single piece of a file download
	struct FileTransferPiecePacket : public highlevel::PacketBase
	{
		HIGHLEVEL_PACKET_DEFINE_PACKET_TYPE("FileTransferPiecePacket");

		static inline const constexpr size_t MAX_PIECE_SIZE = 0x7f00;	// Limited by the maximum packet size of 0x8000 that our socket classes can send/receive and the overhead of other packet content

		uint32 mTransferHandle = 0;
		uint16 mChunkIndex = 0;
		uint32 mStartOffset = 0;	// Relative address inside chunk
		uint16 mSize = 0;

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mTransferHandle);
			serializer.serialize(mChunkIndex);
			serializer.serialize(mStartOffset);
			serializer.serialize(mSize);

			// Actual data comes afterwards and must be read/written separately by the serializer
		}
	};

}
