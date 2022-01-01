/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


#include "Basics.h"

#include <iosfwd> // For forward declarations of std::istream and std::ostream
#include <string>

class BinarySerializer;


// Default template definitions for serialization of variables with a binary serializer
namespace serialization
{

	// We use a function object template instead of a function template.
	// A function template specialisation with an own template parameters doesn't comply to the c++ standard! (Bad VC++ very bad from you to support such things)
	// Only with a class/struct template a specialisation with own template parameters works.
	template<typename T>
	struct serializer
	{
		inline static void serialize(BinarySerializer& serializer, T& value)
		{
			// When you get a compiler error here, it's because the type you try to serialize is not supported
			//  -> Most probably, you're just missing the right include for that type, see predefined serialization in "qsf/serialization/binary/"
#ifdef _MSC_VER
			T::UNDEFINED_SERIALIZATION_TYPE;
#endif
		}
	};

	template<typename T>
	struct serializer<const T>
	{
		inline static void serialize(BinarySerializer& serializer, const T& value)
		{
			// When you get a compiler error here, it's because the type you try to serialize is const
#ifdef _MSC_VER
			T::CONST_SERIALIZATION_TYPE;
#endif
		}
	};

	template<typename T>
	uint32 getToken()
	{
		// Return default token
		return *(uint32*)("type");
	}
}


class API_EXPORT BinarySerializer
{
public:
	enum TokenMode
	{
		TOKEN_FLAG_NONE = 0x00,
		TOKEN_FLAG_TYPE = 0x01,
		TOKEN_FLAG_NAME = 0x02,
		TOKEN_ALL_FLAGS = 0x03
	};

	struct DataBlockInfo
	{
		uint64 mBeginPosition = 0xffffffffffffffffULL;
		uint64 mEndPosition = 0xffffffffffffffffULL;
	};

public:
	BinarySerializer(std::istream& stream);
	BinarySerializer(std::ostream& stream, TokenMode tokenMode);
	~BinarySerializer();

	inline bool isReading() const
	{
		return (nullptr != mInputStream);
	}

	inline bool isWriting() const
	{
		return (nullptr != mOutputStream);
	}

	std::istream& getInputStream() const;
	std::ostream& getOutputStream() const;

	template<typename T>
	void serialize(T& value)
	{
		// Serialize tokens
		if (mTokenMode != TOKEN_FLAG_NONE)
		{
			// Note that the token must be platform independent
			tokenSerialization(serialization::getToken<T>());
		}

		// Serialize the variable itself
		serialization::serializer<T>::serialize(*this, value);
	}

	template<typename T>
	BinarySerializer& operator&(T& value)
	{
		serialize(value);
		return *this;
	}

	template<typename TARGETTYPE, typename ORIGINALTYPE>
	void serializeAs(ORIGINALTYPE& value)
	{
		if (nullptr != mInputStream)
		{
			// Read value
			TARGETTYPE targetTypeValue;
			serialize(targetTypeValue);
			value = static_cast<ORIGINALTYPE>(targetTypeValue);
		}
		else if (nullptr != mOutputStream)
		{
			// Write value
			TARGETTYPE targetTypeValue = static_cast<TARGETTYPE>(value);
			serialize(targetTypeValue);
		}
		else
		{
			RMX_ASSERT(false, "Undefined serialization mode, this should not happen.");
		}
	}

	template<typename T>
	T read(const char* name = nullptr)
	{
		RMX_ASSERT(isReading(), "Don't call 'BinarySerializer::read' if you are not in reading mode");
		T value;
		serialize(value);
		return value;
	}

	template<typename T>
	void read(T& value)
	{
		RMX_ASSERT(isReading(), "Don't call 'BinarySerializer::read' if you are not in reading mode");
		serialize(value);
	}

	template<typename T> void write(const T& value);

	template<typename TARGETTYPE, typename ORIGINALTYPE>
	void writeAs(const ORIGINALTYPE& value)
	{
		RMX_ASSERT(isWriting(), "Don't call 'BinarySerializer::writeAs' if you are not in writing mode");
		TARGETTYPE targetTypeValue = static_cast<TARGETTYPE>(value);
		serialize(targetTypeValue);
	}

	void serializeRawBlock(void* address, size_t bytes);
	void serializeRawBlock(void* address, size_t bytes, bool checkEndianness);

	void readRawBlock(void* address, size_t bytes);
	void readRawBlock(void* address, size_t bytes, bool checkEndianness);

	void writeRawBlock(const void* address, size_t bytes);
	void writeRawBlock(const void* address, size_t bytes, bool checkEndianness);

	void skip(size_t bytes);

	void beginDataBlock(DataBlockInfo& dataBlockInfo);
	void endDataBlock(DataBlockInfo& dataBlockInfo);

	void jumpToBeginOfDataBlock(DataBlockInfo& dataBlockInfo);
	void jumpToEndOfDataBlock(DataBlockInfo& dataBlockInfo);


private:
	void tokenSerialization(uint32 token, const char* name);

	void readPortable(void* address, size_t bytes, bool checkEndianness);
	void writePortable(const void* address, size_t bytes, bool checkEndianness);


private:
	std::istream*	mInputStream;
	std::ostream*	mOutputStream;
	TokenMode		mTokenMode;
	bool			mIsLittleEndianMachine;


};
