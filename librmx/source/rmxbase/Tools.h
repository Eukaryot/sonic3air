/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	// Calculate FNV-1a 64-bit hash for data
	uint64 getFNV1a_64(const uint8* data, size_t bytes);
	uint64 startFNV1a_64();
	uint64 addToFNV1a_64(uint64 hash, const uint8* data, size_t bytes);

	// Calculate Murmur2 64-bit hash for data
	uint64 getMurmur2_64(const uint8* data, size_t bytes);
	uint64 getMurmur2_64(const String& str);
	uint64 getMurmur2_64(const WString& str);
	uint64 getMurmur2_64(const std::string& str);
	uint64 getMurmur2_64(const std::wstring& str);

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

	// Check if an std::string starts with a given other string
	bool startsWith(const std::string& fullString, const std::string& prefix);
	bool startsWith(const std::wstring& fullString, const std::wstring& prefix);
	bool endsWith(const std::string& fullString, const std::string& suffix);
	bool endsWith(const std::wstring& fullString, const std::wstring& suffix);

}
