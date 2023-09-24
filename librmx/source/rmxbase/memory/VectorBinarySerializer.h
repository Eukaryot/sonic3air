/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "rmxbase/base/Types.h"

#include <string>
#include <vector>


class VectorBinarySerializer
{
public:
	VectorBinarySerializer(bool read, std::vector<uint8>& buffer);
	VectorBinarySerializer(bool read, const std::vector<uint8>& buffer);

	inline bool isReading() const		   { return mReading; }
	inline size_t getSize() const		   { return mBuffer.size(); }
	inline size_t getReadPosition() const  { return mReadPosition; }
	inline size_t getRemaining() const	   { return mBuffer.size() - mReadPosition; }

	inline const std::vector<uint8>& getBuffer() const	 { return mBuffer; }
	inline uint8* getBufferPointer(size_t offset) const	 { return &mBuffer[offset]; }

	inline bool hasError() const  { return mHasError; }
	inline void setError()		  { mHasError = true; }

	void read(void* pointer, size_t size);
	void write(const void* pointer, size_t size);

	void serialize(void* pointer, size_t size);

	void serialize(bool& value);
	void serialize(uint8& value);
	void serialize(int8& value);
	void serialize(uint16& value);
	void serialize(int16& value);
	void serialize(uint32& value);
	void serialize(int32& value);
	void serialize(uint64& value);
	void serialize(int64& value);
	void serialize(float& value);
	void serialize(double& value);
	void serialize(std::string& value, size_t stringLengthLimit = 0xffffffff);
	void serialize(std::wstring& value, size_t stringLengthLimit = 0xffffffff);
	void serialize(String& value);
	void serialize(WString& value);

	// Warning: the "bytesLimit" value must stay consistent between serialization and deserialization
	void serializeData(std::vector<uint8>& data, size_t bytesLimit = 0xffffffff);

	template <typename T>
	void serializeArraySize(std::vector<T>& value, uint32 arraySizeLimit = 0xffff)
	{
		if (mReading)
		{
			const uint32 numEntries = (arraySizeLimit <= 0xff) ? static_cast<uint16>(read<uint8>()) : (arraySizeLimit <= 0xffff) ? static_cast<uint16>(read<uint16>()) : read<uint32>();

			// Limit number of bytes
			if (numEntries > arraySizeLimit)
			{
				mHasError = true;
			}

			if (mHasError)
			{
				value.clear();
				return;
			}

			value.resize(numEntries);
		}
		else
		{
			RMX_ASSERT((uint32)value.size() <= arraySizeLimit, "Array size " << value.size() << " exceeds limit " << arraySizeLimit);
			if (arraySizeLimit <= 0xff)
			{
				writeAs<uint8>(value.size());
			}
			else if (arraySizeLimit <= 0xffff)
			{
				writeAs<uint16>(value.size());
			}
			else
			{
				writeAs<uint32>(value.size());
			}
		}
	}

	template <typename T, typename S>
	void serializeAs(S& value)
	{
		if (mReading)
		{
			T targetTypeValue;
			serialize(targetTypeValue);
			value = static_cast<S>(targetTypeValue);
		}
		else
		{
			T targetTypeValue = static_cast<T>(value);
			serialize(targetTypeValue);
		}
	}

	template <typename T>
	void operator&(T& value)
	{
		serialize(value);
	}

	size_t readSize(size_t limit);
	void writeSize(size_t value, size_t limit);

	template <typename T>
	T read()
	{
		T value;
		serialize(value);
		return value;
	}

	std::string_view readStringView(size_t stringLengthLimit = 0xffffffff);

	template <typename T>
	void write(const T& value)
	{
		serialize(const_cast<T&>(value));
	}

	void write(std::string_view value, size_t stringLengthLimit = 0xffffffff);
	void write(std::wstring_view value, size_t stringLengthLimit = 0xffffffff);

	template <typename T, typename S>
	void writeAs(const S& value)
	{
		T targetTypeValue = static_cast<T>(value);
		serialize(targetTypeValue);
	}

	void skip(size_t bytes);

	const uint8* peek() const;

private:
	const uint8* readAccess(size_t size);
	uint8* writeAccess(size_t size);

private:
	bool mReading;
	std::vector<uint8>& mBuffer;
	size_t mReadPosition = 0;
	bool mHasError = false;
};
