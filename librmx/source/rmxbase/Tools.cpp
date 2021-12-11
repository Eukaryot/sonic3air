/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"
#include <iomanip>


namespace rmx
{

	uint32 getFNV1a_32(const uint8* data, size_t bytes)
	{
		uint32 hash = FNV1a_32_START_VALUE;
		for (size_t i = 0; i < bytes; ++i)
		{
			hash = (hash ^ data[i]) * FNV1a_32_MAGIC_PRIME;
		}
		return hash;
	}

	uint32 startFNV1a_32()
	{
		return FNV1a_32_START_VALUE;
	}

	uint32 addToFNV1a_32(uint32 hash, const uint8* data, size_t bytes)
	{
		for (size_t i = 0; i < bytes; ++i)
		{
			hash = (hash ^ data[i]) * FNV1a_32_MAGIC_PRIME;
		}
		return hash;
	}

	uint64 getFNV1a_64(const uint8* data, size_t bytes)
	{
		uint64 hash = FNV1a_64_START_VALUE;
		for (size_t i = 0; i < bytes; ++i)
		{
			hash = (hash ^ data[i]) * FNV1a_64_MAGIC_PRIME;
		}
		return hash;
	}

	uint64 startFNV1a_64()
	{
		return FNV1a_64_START_VALUE;
	}

	uint64 addToFNV1a_64(uint64 hash, const uint8* data, size_t bytes)
	{
		for (size_t i = 0; i < bytes; ++i)
		{
			hash = (hash ^ data[i]) * FNV1a_64_MAGIC_PRIME;
		}
		return hash;
	}

	uint64 getMurmur2_64(const uint8* data, size_t bytes)
	{
		// Code is based on https://github.com/abrandoned/murmur2/blob/master/MurmurHash2.c
		//  -> Namely "MurmurHash64A", i.e. the version optimized for 64-bit architectures
		//  -> We're using a fixed seed of 0

		const uint64 m = 0xc6a4a7935bd1e995ull;
		const int r = 47;
		uint64 h = (uint64)bytes * m;

		const uint64* data64 = (const uint64_t*)data;
		const uint64* end = data64 + (bytes / 8);

	#if defined(__arm__)
		const bool isAligned64 = ((size_t)data & 7) == 0;
		if (!isAligned64)
		{
			uint64 k;
			while (data64 != end)
			{
				// Do not access memory directly, but byte-wise to avoid "SIGBUS illegal alignment" issues (this happened on Android Release builds, but not in Debug for some reason)
				//  -> This somewhat defeats the purpose of the whole optimization by using Murmur2...
				memcpy(&k, data64, sizeof(uint64_t));
				++data64;
				k *= m;
				k ^= k >> r;
				k *= m;
				h ^= k;
				h *= m;
			}
		}
		else
	#endif
		{
			while (data64 != end)
			{
				uint64 k = *data64++;
				k *= m;
				k ^= k >> r;
				k *= m;
				h ^= k;
				h *= m;
			}
		}

		const uint8* data8 = (const uint8*)data64;
		switch (bytes & 0x07)
		{
			case 7:  h ^= ((uint64)data8[6]) << 48;
			case 6:  h ^= ((uint64)data8[5]) << 40;
			case 5:  h ^= ((uint64)data8[4]) << 32;
			case 4:  h ^= ((uint64)data8[3]) << 24;
			case 3:  h ^= ((uint64)data8[2]) << 16;
			case 2:  h ^= ((uint64)data8[1]) << 8;
			case 1:  h ^= ((uint64)data8[0]);
				h *= m;
		};

		h ^= h >> r;
		h *= m;
		h ^= h >> r;
		return h;
	}

	uint64 getMurmur2_64(const String& str)
	{
		return getMurmur2_64((const uint8*)*str, str.length() * sizeof(char));
	}

	uint64 getMurmur2_64(const WString& str)
	{
		// Note that this is *not* platform-independent
		return getMurmur2_64((const uint8*)*str, str.length() * sizeof(wchar_t));
	}

	uint64 getMurmur2_64(const std::string& str)
	{
		return getMurmur2_64((const uint8*)&str[0], str.length() * sizeof(char));
	}

	uint64 getMurmur2_64(const std::wstring& str)
	{
		// Note that this is *not* platform-independent
		return getMurmur2_64((uint8*)&str[0], str.length() * sizeof(wchar_t));
	}

	uint32 getCRC32(const uint8* data, size_t bytes)
	{
		static uint32 crc_table[256];
		static bool crc_table_initialized = false;
		if (!crc_table_initialized)
		{
			for (int n = 0; n < 256; ++n)
			{
				uint32 c = (uint32)n;
				for (int k = 0; k < 8; ++k)
				{
					if (c & 1)
						c = 0xedb88320 ^ (c >> 1);
					else
						c = (c >> 1);
				}
				crc_table[n] = c;
			}
			crc_table_initialized = true;
		}
		uint32 crc = 0xffffffff;
		for (size_t n = 0; n < bytes; ++n)
		   crc = crc_table[(crc ^ data[n]) & 0xff] ^ (crc >> 8);
		return crc ^ 0xffffffffL;
	}

	uint32 getAdler32(const uint8* data, size_t bytes)
	{
		uint32 s1 = 1;
		uint32 s2 = 0;
		for (size_t i = 0; i < bytes; ++i)
		{
			s1 = (s1 + data[i]) % 65521;
			s2 = (s2 + s1) % 65521;
		}
		return (s2 << 16) + s1;
	}


	uint64 parseInteger(const String& input, size_t& pos)
	{
		uint64 result = 0;
		uint64 base = 10;
		if ((int)pos+1 < input.length() && input[pos] == '0' && input[pos+1] == 'x')
		{
			// Hexadecimal
			base = 16;
			pos += 2;
		}

		for (; pos < (size_t)input.length(); ++pos)
		{
			uint64 nextDigit;
			const char ch = input[pos];
			if (ch >= '0' && ch <= '9')
				nextDigit = (ch - '0');
			else if (base == 16 && ch >= 'A' && ch <= 'F')
				nextDigit = (ch - 'A') + 10;
			else if (base == 16 && ch >= 'a' && ch <= 'f')
				nextDigit = (ch - 'a') + 10;
			else
				break;

			result = result * base + nextDigit;
		}
		return result;
	}

	uint64 parseInteger(const String& input)
	{
		size_t pos = 0;
		return parseInteger(input, pos);
	}

	std::string hexString(uint64 value, const char* prefix)
	{
		std::stringstream str;
		str << prefix << std::hex << value;
		return str.str();
	}

	std::string hexString(uint64 value, uint32 minDigits, const char* prefix)
	{
		std::stringstream str;
		str << prefix << std::hex << std::setfill('0') << std::setw(minDigits) << value;
		return str.str();
	}

	bool startsWith(const std::string& fullString, const std::string& prefix)
	{
		if (fullString.length() < prefix.length())
			return false;
		if (memcmp((void*)&fullString[0], (void*)&prefix[0], prefix.length() * sizeof(char)) != 0)
			return false;
		return true;
	}

	bool startsWith(const std::wstring& fullString, const std::wstring& prefix)
	{
		if (fullString.length() < prefix.length())
			return false;
		if (memcmp((void*)&fullString[0], (void*)&prefix[0], prefix.length() * sizeof(wchar_t)) != 0)
			return false;
		return true;
	}

	bool endsWith(const std::string& fullString, const std::string& suffix)
	{
		if (fullString.length() < suffix.length())
			return false;
		const size_t offset = fullString.length() - suffix.length();
		if (memcmp((void*)&fullString[offset], (void*)&suffix[0], suffix.length() * sizeof(char)) != 0)
			return false;
		return true;
	}

	bool endsWith(const std::wstring& fullString, const std::wstring& suffix)
	{
		if (fullString.length() < suffix.length())
			return false;
		const size_t offset = fullString.length() - suffix.length();
		if (memcmp((void*)&fullString[offset], (void*)&suffix[0], suffix.length() * sizeof(wchar_t)) != 0)
			return false;
		return true;
	}

}
