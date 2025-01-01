/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <string>


namespace rmx
{
	struct UTF8Conversion
	{
		static uint32 readCharacterFromUTF8(std::string_view& input);		// Returns 0xffffffff on failure
		static bool convertFromUTF8(std::string_view input, std::wstring& output);

		static size_t getCharacterLengthAsUTF8(uint32 input);
		static size_t getLengthAsUTF8(std::wstring_view input);
		static size_t writeCharacterAsUTF8(uint32 input, char* output);
		static void convertToUTF8(std::wstring_view input, std::string& output);
	};
}
