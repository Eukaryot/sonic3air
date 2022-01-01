/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


namespace
{
	const bool DEFAULT_IS_BIG_ENDIAN = false;

	bool isLittleEndian()
	{
		const uint16 a = 0x1234;
		return (*(int8*)&a == 0x34);
	}
}


BinarySerializer::BinarySerializer(std::istream& stream) :
	mInputStream(&stream),
	mOutputStream(nullptr),
	mTokenMode(TOKEN_FLAG_NONE)
{
	// Check endianness
	mIsLittleEndianMachine = isLittleEndian();

	// Read token mode
	uint8 tokenModeAsByte = 0;
	readPortable(&tokenModeAsByte, 1, false);
	mTokenMode = static_cast<TokenMode>(tokenModeAsByte);
}

BinarySerializer::BinarySerializer(std::ostream& stream, TokenMode tokenMode) :
	mInputStream(nullptr),
	mOutputStream(&stream),
	mTokenMode(tokenMode)
{
	// Check endianness
	mIsLittleEndianMachine = isLittleEndian();

	// Write token mode
	uint8 tokenModeAsByte = static_cast<uint8>(mTokenMode);
	writePortable(&tokenModeAsByte, 1, false);
}

BinarySerializer::~BinarySerializer()
{
	// Nothing here
}

std::istream& BinarySerializer::getInputStream() const
{
	RMX_CHECK(nullptr != mInputStream, "Don't call 'BinarySerializer::getInputStream' if you are not in reading mode", RMX_REACT_THROW);
	return *mInputStream;
}

std::ostream& BinarySerializer::getOutputStream() const
{
	RMX_CHECK(nullptr != mOutputStream, "Don't call 'BinarySerializer::getOutputStream' if you are not in writing mode", RMX_REACT_THROW);
	return *mOutputStream;
}

void BinarySerializer::serializeRawBlock(void* address, size_t bytes)
{
	if (nullptr != mInputStream)
	{
		// Read content
		readRawBlock(address, bytes);
	}
	else if (nullptr != mOutputStream)
	{
		// Write content
		writeRawBlock(address, bytes);
	}
	else
	{
		RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
	}
}

void BinarySerializer::serializeRawBlock(void* address, size_t bytes, bool checkEndianness)
{
	if (nullptr != mInputStream)
	{
		// Read content
		readPortable(address, bytes, checkEndianness);
	}
	else if (nullptr != mOutputStream)
	{
		// Write content
		writePortable(address, bytes, checkEndianness);
	}
	else
	{
		RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
	}
}

void BinarySerializer::readRawBlock(void* address, size_t bytes)
{
	RMX_ASSERT(isReading(), "Don't call 'BinarySerializer::readRawBlock' if you are not in reading mode");

	mInputStream->read(reinterpret_cast<char*>(address), bytes);
	RMX_CHECK(mInputStream->good(), "BinarySerializer: I/O error while deserializing", RMX_REACT_THROW);
}

void BinarySerializer::readRawBlock(void* address, size_t bytes, bool checkEndianness)
{
	RMX_ASSERT(isReading(), "Don't call 'BinarySerializer::readRawBlock' if you are not in reading mode");
	readPortable(address, bytes, checkEndianness);
}

void BinarySerializer::writeRawBlock(const void* address, size_t bytes)
{
	RMX_ASSERT(isWriting(), "Don't call 'BinarySerializer::writeRawBlock' if you are not in writing mode");

	mOutputStream->write(reinterpret_cast<const char*>(address), bytes);
	RMX_CHECK(mOutputStream->good(), "BinarySerializer: I/O error while serializing", RMX_REACT_THROW);
}

void BinarySerializer::writeRawBlock(const void* address, size_t bytes, bool checkEndianness)
{
	RMX_ASSERT(isWriting(), "Don't call 'BinarySerializer::writeRawBlock' if you are not in writing mode");
	writePortable(address, bytes, checkEndianness);
}

void BinarySerializer::skip(size_t bytes)
{
	if (nullptr != mInputStream)
	{
		mInputStream->seekg(mInputStream->tellg() + static_cast<std::streamoff>(bytes));
	}
	else
	{
		RMX_ERROR("Calling 'BinarySerializer::skip' is only allowed for reading mode", RMX_REACT_THROW);
	}
}

void BinarySerializer::beginDataBlock(BinarySerializer::DataBlockInfo& dataBlockInfo)
{
	if (nullptr != mInputStream)
	{
		uint32 length = 0;
		readPortable(&length, 4, true);

		dataBlockInfo.mBeginPosition = mInputStream->tellg();
		dataBlockInfo.mEndPosition = dataBlockInfo.mBeginPosition + length;
	}
	else if (nullptr != mOutputStream)
	{
		uint32 length = 0;
		writePortable(&length, 4, true);

		dataBlockInfo.mBeginPosition = mOutputStream->tellp();
		dataBlockInfo.mEndPosition = 0;
	}
	else
	{
		RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
	}
}

void BinarySerializer::endDataBlock(BinarySerializer::DataBlockInfo& dataBlockInfo)
{
	if (dataBlockInfo.mEndPosition < 0xffffffffffffffffULL)
	{
		if (nullptr != mInputStream)
		{
			const uint64 currentPosition = static_cast<uint64>(mInputStream->tellg());
			RMX_CHECK(currentPosition == dataBlockInfo.mEndPosition, "End of data block read position mismatch (current: " << currentPosition << ", expected: " << dataBlockInfo.mEndPosition << ")", RMX_REACT_THROW);
		}
		else if (nullptr != mOutputStream)
		{
			dataBlockInfo.mEndPosition = mOutputStream->tellp();

			mOutputStream->seekp(dataBlockInfo.mBeginPosition - 4);

			RMX_CHECK(dataBlockInfo.mEndPosition - dataBlockInfo.mBeginPosition < UINT32_MAX, "Defining a datablock bigger than 4 GiB is currently not supported", RMX_REACT_THROW);
			uint32 length = static_cast<uint32>(dataBlockInfo.mEndPosition - dataBlockInfo.mBeginPosition);
			writePortable(&length, 4, true);

			mOutputStream->seekp(dataBlockInfo.mEndPosition);
		}
		else
		{
			RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
		}
	}
	else
	{
		RMX_ERROR("Trying to define end of data block, but begin of data block was not defined", RMX_REACT_THROW);
	}
}

void BinarySerializer::jumpToBeginOfDataBlock(BinarySerializer::DataBlockInfo& dataBlockInfo)
{
	if (dataBlockInfo.mBeginPosition < 0xffffffffffffffffULL)
	{
		if (nullptr != mInputStream)
		{
			mInputStream->seekg(dataBlockInfo.mBeginPosition);
		}
		else if (nullptr != mOutputStream)
		{
			mOutputStream->seekp(dataBlockInfo.mBeginPosition);
		}
		else
		{
			RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
		}
	}
	else
	{
		RMX_ERROR("Trying to jump to begin of data block, but begin of data block is not yet defined", RMX_REACT_THROW);
	}
}

void BinarySerializer::jumpToEndOfDataBlock(BinarySerializer::DataBlockInfo& dataBlockInfo)
{
	if (dataBlockInfo.mEndPosition < 0xffffffffffffffffULL)
	{
		if (nullptr != mInputStream)
		{
			mInputStream->seekg(dataBlockInfo.mEndPosition);
		}
		else if (nullptr != mOutputStream)
		{
			mOutputStream->seekp(dataBlockInfo.mEndPosition);
		}
		else
		{
			RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
		}
	}
	else
	{
		RMX_ERROR("Trying to jump to end of data block, but end of data block is not yet defined", RMX_REACT_THROW);
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void BinarySerializer::tokenSerialization(uint32 token, const char* name)
{
	// Fallback for name parameter
	if (nullptr == name)
		name = "?";

	if (nullptr != mInputStream)
	{
		// Read and check type token
		if (mTokenMode & TOKEN_FLAG_TYPE)
		{
			uint32 tokenValue = 0;
			readPortable(&tokenValue, 4, true);
			RMX_CHECK(tokenValue == token, "BinarySerializer: type token mismatch, expected " << token << ", but read " << tokenValue << ".", RMX_REACT_THROW);
		}

		// Read and check name token
		if (mTokenMode & TOKEN_FLAG_NAME)
		{
			std::string tokenName;
			char character = 0;
			do
			{
				readPortable(reinterpret_cast<uint8*>(&character), 1, false);
				if (character != 0)
					tokenName += character;
			}
			while (character != 0);

			RMX_CHECK(tokenName == name, "BinarySerializer: name token mismatch, expected \"" << name << "\", but read \"" << tokenName << "\"", RMX_REACT_THROW);
		}
	}
	else if (nullptr != mOutputStream)
	{
		// Write tokens
		if (mTokenMode & TOKEN_FLAG_TYPE)
		{
			writePortable(&token, 4, true);
		}

		if (mTokenMode & TOKEN_FLAG_NAME)
		{
			writeRawBlock(name, static_cast<uint32>(strlen(name)+1));
		}
	}
	else
	{
		RMX_ERROR("Undefined serialization mode, this should not happen.", RMX_REACT_THROW);
	}
}

void BinarySerializer::readPortable(void* address, size_t bytes, bool checkEndianness)
{
	if (checkEndianness && mIsLittleEndianMachine == DEFAULT_IS_BIG_ENDIAN)
	{
		if (bytes == 8)
		{
			uint64 value = 0;
			mInputStream->read(reinterpret_cast<char*>(&value), bytes);
			RMX_CHECK(mInputStream->good(), "BinarySerializer: I/O error while deserializing", RMX_REACT_THROW);
			*reinterpret_cast<uint64*>(address) = swapBytes64(value);
			return;
		}

		if (bytes == 4)
		{
			uint32 value = 0;
			mInputStream->read(reinterpret_cast<char*>(&value), bytes);
			RMX_CHECK(mInputStream->good(), "BinarySerializer: I/O error while deserializing", RMX_REACT_THROW);
			*reinterpret_cast<uint32*>(address) = swapBytes32(value);
			return;
		}

		if (bytes == 2)
		{
			uint16 value = 0;
			mInputStream->read(reinterpret_cast<char*>(&value), bytes);
			RMX_CHECK(mInputStream->good(), "BinarySerializer: I/O error while deserializing", RMX_REACT_THROW);
			*reinterpret_cast<uint16*>(address) = swapBytes16(value);
			return;
		}
	}

	mInputStream->read(reinterpret_cast<char*>(address), bytes);
	RMX_CHECK(mInputStream->good(), "BinarySerializer: I/O error while deserializing", RMX_REACT_THROW);
}

void BinarySerializer::writePortable(const void* address, size_t bytes, bool checkEndianness)
{
	if (checkEndianness && mIsLittleEndianMachine == DEFAULT_IS_BIG_ENDIAN)
	{
		if (bytes == 8)
		{
			uint64 value = swapBytes64(*reinterpret_cast<const uint64*>(address));
			mOutputStream->write(reinterpret_cast<char*>(&value), bytes);
			RMX_CHECK(mOutputStream->good(), "BinarySerializer: I/O error while serializing", RMX_REACT_THROW);
			return;
		}

		if (bytes == 4)
		{
			uint32 value = swapBytes32(*reinterpret_cast<const uint32*>(address));
			mOutputStream->write(reinterpret_cast<char*>(&value), bytes);
			RMX_CHECK(mOutputStream->good(), "BinarySerializer: I/O error while serializing", RMX_REACT_THROW);
			return;
		}

		if (bytes == 2)
		{
			uint16 value = swapBytes16(*reinterpret_cast<const uint16*>(address));
			mOutputStream->write(reinterpret_cast<char*>(&value), bytes);
			RMX_CHECK(mOutputStream->good(), "BinarySerializer: I/O error while serializing", RMX_REACT_THROW);
			return;
		}
	}

	mOutputStream->write(reinterpret_cast<const char*>(address), bytes);
	RMX_CHECK(mOutputStream->good(), "BinarySerializer: I/O error while serializing", RMX_REACT_THROW);
}
