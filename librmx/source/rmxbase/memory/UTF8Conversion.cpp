/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"
#include "rmxbase/memory/UTF8Conversion.h"


namespace rmx
{
	uint32 UTF8Conversion::readCharacterFromUTF8(std::string_view& input)
	{
		const uint32 firstByte = (uint32)input[0];
		if (firstByte <= 0x7f)
		{
			// Single byte
			input = input.substr(1);
			return firstByte;
		}
		else if (firstByte >= 0xc2 && firstByte <= 0xf4)
		{
			// Multiple bytes
			#define READ_SEQUENCE_BYTE(_variable_, _index_) \
				uint32 _variable_ = input[_index_]; \
				if (_variable_ < 0x80 || _variable_ >= 0xc0) \
					return 0xffffffff; \
				_variable_ &= 0x3f; \

			if (firstByte <= 0xdf)
			{
				if (input.length() < 2)
					return 0xffffffff;

				READ_SEQUENCE_BYTE(secondByte, 1);
				input = input.substr(2);

				return ((firstByte & 0x1f) << 6) + secondByte;
			}
			else if (firstByte <= 0xef)
			{
				if (input.length() < 3)
					return 0xffffffff;

				READ_SEQUENCE_BYTE(secondByte, 1);
				READ_SEQUENCE_BYTE(thirdByte,  2);
				input = input.substr(3);

				return ((firstByte & 0x0f) << 12) + (secondByte << 6) + thirdByte;
			}
			else
			{
				if (input.length() < 4)
					return 0xffffffff;

				READ_SEQUENCE_BYTE(secondByte, 1);
				READ_SEQUENCE_BYTE(thirdByte,  2);
				READ_SEQUENCE_BYTE(fourthByte, 3);
				input = input.substr(4);

				return ((firstByte & 0x07) << 18) + (secondByte << 12) + (thirdByte << 6) + fourthByte;
			}

			#undef READ_SEQUENCE_BYTE
		}
		else
		{
			// Invalid code
			return 0xffffffff;
		}
	}

	bool UTF8Conversion::convertFromUTF8(std::string_view input, std::wstring& output)
	{
		output.reserve(output.length() + input.length());	// Possibly too large, but that's better than reallocations
		while (input.length() > 0)
		{
			const uint32 code = readCharacterFromUTF8(input);
			if (code == 0xffffffff)
				return false;

			output.push_back((wchar_t)code);
		}
		return true;
	}

	size_t UTF8Conversion::getCharacterLengthAsUTF8(uint32 input)
	{
		if (input <= 0x7f)
			return 1;
		else if (input <= 0x7ff)
			return 2;
		else if (input <= 0xffff)
			return 3;
		else
			return 4;
	}

	size_t UTF8Conversion::getLengthAsUTF8(std::wstring_view input)
	{
		size_t length = 0;
		for (wchar_t ch : input)
		{
			length += getCharacterLengthAsUTF8((uint32)ch);
		}
		return length;
	}

	size_t UTF8Conversion::writeCharacterAsUTF8(uint32 input, char* output)
	{
		if (input <= 0x7f)
		{
			output[0] = (char)input;
			return 1;
		}
		else if (input <= 0x7ff)
		{
			output[0] = (char)(0xc0 + (input >> 6));
			output[1] = (char)(0x80 + (input & 0x3f));
			return 2;
		}
		else if (input <= 0xffff)
		{
			output[0] = (char)(input >> 12);
			output[1] = (char)((input >> 6) & 0x3f);
			output[2] = (char)(input & 0x3f);
			return 3;
		}
		else
		{
			output[0] = (char)(input >> 18);
			output[1] = (char)((input >> 12) & 0x3f);
			output[2] = (char)((input >> 6) & 0x3f);
			output[3] = (char)(input & 0x3f);
			return 4;
		}
	}

	void UTF8Conversion::convertToUTF8(std::wstring_view input, std::string& output)
	{
		const size_t existingLength = output.length();
		const size_t additionalLength = getLengthAsUTF8(input);
		output.resize(existingLength + additionalLength);

		char* writePosition = &output[existingLength];
		for (wchar_t ch : input)
		{
			const size_t encodedLength = writeCharacterAsUTF8((uint32)ch, writePosition);
			writePosition += encodedLength;
		}
	}
}
