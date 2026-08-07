/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class Crypto
{
public:
	static bool decodeBase64(std::string_view encodedString, std::vector<uint8>& outData);
	static std::string encodeBase64(const uint8* data, size_t length);

	static void buildSHA1(const std::string& string, uint32* output);	// Output is supposed to be 5 uint32 values
};
