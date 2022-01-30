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
	template<typename T>
	FORCE_INLINE bool serializePrimitiveDataType(T& value, bool reading, std::vector<uint8>& buffer, size_t& readPosition)
	{
		if (reading)
		{
			if (readPosition + sizeof(T) > buffer.size())
				return false;

			value = *(T*)&buffer[readPosition];
			readPosition += sizeof(T);
		}
		else
		{
			const size_t oldSize = buffer.size();
			buffer.resize(oldSize + sizeof(T));
			*(T*)&buffer[oldSize] = value;
		}
		return true;
	}

	template<>
	FORCE_INLINE bool serializePrimitiveDataType<bool>(bool& value, bool reading, std::vector<uint8>& buffer, size_t& readPosition)
	{
		if (reading)
		{
			if (readPosition >= buffer.size())
				return false;

			value = *(bool*)&buffer[readPosition];
			++readPosition;
		}
		else
		{
			const size_t oldSize = buffer.size();
			buffer.resize(oldSize + 1);
			buffer[oldSize] = value ? 1 : 0;
		}
		return true;
	}
}


VectorBinarySerializer::VectorBinarySerializer(bool read, std::vector<uint8>& buffer) :
	mReading(read),
	mBuffer(buffer)
{
}

VectorBinarySerializer::VectorBinarySerializer(bool read, const std::vector<uint8>& buffer) :
	mReading(read),
	mBuffer(const_cast<std::vector<uint8>&>(buffer))
{
}

void VectorBinarySerializer::read(void* pointer, size_t size)
{
	const uint8* source = readAccess(size);
	if (nullptr != source)
	{
		memcpy(pointer, source, size);
	}
}

void VectorBinarySerializer::write(const void* pointer, size_t size)
{
	memcpy(writeAccess(size), pointer, size);
}

void VectorBinarySerializer::serialize(void* pointer, size_t size)
{
	if (mReading)
	{
		read(pointer, size);
	}
	else
	{
		write(pointer, size);
	}
}

void VectorBinarySerializer::serialize(bool& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(uint8& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(int8& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(uint16& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(int16& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(uint32& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(int32& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(uint64& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(int64& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(float& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(double& value)
{
	mHasError = (mHasError || !serializePrimitiveDataType(value, mReading, mBuffer, mReadPosition));
}

void VectorBinarySerializer::serialize(std::string& value, size_t stringLengthLimit)
{
	if (mReading)
	{
		const size_t length = (stringLengthLimit <= 0xff) ? static_cast<size_t>(read<uint8>()) : (stringLengthLimit <= 0xffff) ? static_cast<size_t>(read<uint16>()) : read<uint32>();

		// Limit length of strings
		if (length > stringLengthLimit)
		{
			mHasError = true;
		}

		if (mHasError)
		{
			value.clear();
			return;
		}

		value.resize(length);
	}
	else
	{
		if (stringLengthLimit <= 0xff)
		{
			writeAs<uint8>(value.size());
		}
		else if (stringLengthLimit <= 0xffff)
		{
			writeAs<uint16>(value.size());
		}
		else
		{
			writeAs<uint32>(value.size());
		}
	}

	if (!value.empty())
	{
		serialize(&value[0], value.length());
	}
}

void VectorBinarySerializer::serialize(std::wstring& value)
{
	if (mReading)
	{
		value.resize((size_t)read<uint32>());
		if (!value.empty())
		{
		#ifdef PLATFORM_WINDOWS
			static_assert(sizeof(wchar_t) == 2);
			read(&value[0], value.length() * sizeof(wchar_t));
		#else
			const uint16* pointer = (uint16*)readAccess(value.length() * 2);
			for (size_t i = 0; i < value.length(); ++i)
				value[i] = (wchar_t)pointer[i];
		#endif
		}
	}
	else
	{
		writeAs<uint32>(value.length());
		if (!value.empty())
		{
			// Write with 2 bytes per character -- TODO: expand to 4 bytes, or encode differently, like UTF-8 instead?
		#ifdef PLATFORM_WINDOWS
			static_assert(sizeof(wchar_t) == 2);
			write(&value[0], value.length() * sizeof(wchar_t));
		#else
			const size_t size = value.length() * 2;
			uint16* pointer = (uint16*)writeAccess(size);
			for (size_t i = 0; i < value.length(); ++i)
				pointer[i] = (uint16)value[i];
		#endif
		}
	}
}

void VectorBinarySerializer::serialize(String& value)
{
	if (mReading)
	{
		value.expand((int)read<uint32>());
		read(value.accessData(), value.length());
	}
	else
	{
		writeAs<uint32>(value.length());
		if (!value.empty())
			write(value.accessData(), value.length());
	}
}

void VectorBinarySerializer::serialize(WString& value)
{
	if (mReading)
	{
		value.expand((int)read<uint32>());
		read(value.accessData(), value.length() * sizeof(wchar_t));			// TODO: This is not compatible among different platforms!
	}
	else
	{
		writeAs<uint32>(value.length());
		if (!value.empty())
			write(value.accessData(), value.length() * sizeof(wchar_t));	// TODO: This is not compatible among different platforms!
	}
}

void VectorBinarySerializer::serializeData(std::vector<uint8>& data, size_t bytesLimit)
{
	if (isReading())
	{
		const size_t numBytes = (bytesLimit <= 0xff) ? static_cast<size_t>(read<uint8>()) : (bytesLimit <= 0xffff) ? static_cast<size_t>(read<uint16>()) : read<uint32>();

		// Limit number of bytes
		if (numBytes > bytesLimit)
		{
			mHasError = true;
		}

		if (mHasError)
		{
			data.clear();
			return;
		}

		data.resize(numBytes);
	}
	else
	{
		if (bytesLimit <= 0xff)
		{
			writeAs<uint8>(data.size());
		}
		else if (bytesLimit <= 0xffff)
		{
			writeAs<uint16>(data.size());
		}
		else
		{
			writeAs<uint32>(data.size());
		}
	}

	if (!data.empty())
	{
		serialize(&data[0], data.size());
	}
}

std::string_view VectorBinarySerializer::readStringView(size_t stringLengthLimit)
{
	const size_t length = (stringLengthLimit <= 0xff) ? static_cast<size_t>(read<uint8>()) : (stringLengthLimit <= 0xffff) ? static_cast<size_t>(read<uint16>()) : read<uint32>();

	// Limit length of strings
	if (length > stringLengthLimit)
	{
		mHasError = true;
	}
	if (mHasError || length == 0)
	{
		return std::string_view();
	}

	const char* ptr = (const char*)readAccess(length);
	return std::string_view(ptr, length);
}

void VectorBinarySerializer::write(std::string_view value, size_t stringLengthLimit)
{
	if (stringLengthLimit <= 0xff)
	{
		writeAs<uint8>(value.size());
	}
	else if (stringLengthLimit <= 0xffff)
	{
		writeAs<uint16>(value.size());
	}
	else
	{
		writeAs<uint32>(value.size());
	}

	if (!value.empty())
	{
		write(&value[0], value.length());
	}
}

void VectorBinarySerializer::write(std::wstring_view value)
{
	writeAs<uint32>(value.length());
	if (!value.empty())
	{
		// Write with 2 bytes per character -- TODO: expand to 4 bytes, or encode differently, like UTF-8 instead?
	#ifdef PLATFORM_WINDOWS
		static_assert(sizeof(wchar_t) == 2);
		write(&value[0], value.length() * sizeof(wchar_t));
	#else
		const size_t size = value.length() * 2;
		uint16* pointer = (uint16*)writeAccess(size);
		for (size_t i = 0; i < value.length(); ++i)
			pointer[i] = (uint16)value[i];
	#endif
	}
}

void VectorBinarySerializer::skip(size_t bytes)
{
	if (mReading)
	{
		mReadPosition += bytes;
	}
}

const uint8* VectorBinarySerializer::peek() const
{
	if (mReading)
	{
		return &mBuffer[mReadPosition];
	}
	else
	{
		return nullptr;
	}
}

const uint8* VectorBinarySerializer::readAccess(size_t size)
{
	// Don't read more data than there is
	if (mReadPosition + size > mBuffer.size())
	{
		mHasError = true;
		mReadPosition = mBuffer.size();
		return nullptr;
	}

	const uint8* result = &mBuffer[mReadPosition];
	mReadPosition += size;
	return result;
}

uint8* VectorBinarySerializer::writeAccess(size_t size)
{
	const size_t oldSize = mBuffer.size();
	mBuffer.resize(oldSize + size);
	return &mBuffer[oldSize];
}
