/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	static const constexpr uint64 FNV1a_32_START_VALUE = 0xcbf29ce4u;
	static const constexpr uint64 FNV1a_32_MAGIC_PRIME = 0x1000193u;
	static const constexpr uint64 FNV1a_64_START_VALUE = 0xcbf29ce484222325;
	static const constexpr uint64 FNV1a_64_MAGIC_PRIME = 0x00000100000001b3;

	// Calculate FNV-1a 32-bit hash for data
	uint32 getFNV1a_32(const uint8* data, size_t bytes);
	uint32 startFNV1a_32();
	uint32 addToFNV1a_32(uint32 hash, const uint8* data, size_t bytes);

	// Calculate FNV-1a 64-bit hash for data
	uint64 getFNV1a_64(const uint8* data, size_t bytes);
	uint64 startFNV1a_64();
	uint64 addToFNV1a_64(uint64 hash, const uint8* data, size_t bytes);

	// Compile-time FNV-1a 32-bit and 64-bit of a string
	static constexpr inline uint32 compileTimeFNV_32(const char* string, const uint32 value = FNV1a_32_START_VALUE) noexcept
	{
		return (string[0] == 0) ? value : compileTimeFNV_32(&string[1], (value ^ static_cast<uint32>(string[0])) * FNV1a_32_MAGIC_PRIME);
	}
	static constexpr inline uint64 compileTimeFNV_64(const char* string, const uint64 value = FNV1a_64_START_VALUE) noexcept
	{
		return (string[0] == 0) ? value : compileTimeFNV_64(&string[1], (value ^ static_cast<uint32>(string[0])) * 0x00000100000001b3);
	}

	// Calculate Murmur2 64-bit hash for data
	uint64 getMurmur2_64(const uint8* data, size_t bytes);
	uint64 getMurmur2_64(const String& str);
	uint64 getMurmur2_64(const WString& str);
	uint64 getMurmur2_64(const std::string& str);
	uint64 getMurmur2_64(const std::wstring& str);
	uint64 getMurmur2_64(std::string_view str);
	uint64 getMurmur2_64(std::wstring_view str);

	// Calculate CRC32 checksum for data
	uint32 getCRC32(const uint8* data, size_t bytes);

	// Calculate Adler32 checksum for data
	uint32 getAdler32(const uint8* data, size_t bytes);


	// Parse integer, with support for hexidecimal string (starting with "0x") and 64-bit values
	uint64 parseInteger(const String& input, size_t& pos);
	uint64 parseInteger(const String& input);

	// Create hexadecimal String
	std::string hexString(uint64 value, const char* prefix = "0x");
	std::string hexString(uint64 value, uint32 minDigits, const char* prefix = "0x");

	// Check if an std::string starts/ends with a given other string
	bool startsWith(const std::string& fullString, const std::string& prefix);
	bool startsWith(const std::wstring& fullString, const std::wstring& prefix);
	bool startsWith(std::string_view fullString, std::string_view prefix);
	bool startsWith(std::wstring_view fullString, std::wstring_view prefix);
	bool endsWith(const std::string& fullString, const std::string& suffix);
	bool endsWith(const std::wstring& fullString, const std::wstring& suffix);
	bool endsWith(std::string_view fullString, std::string_view prefix);
	bool endsWith(std::wstring_view fullString, std::wstring_view prefix);

}
