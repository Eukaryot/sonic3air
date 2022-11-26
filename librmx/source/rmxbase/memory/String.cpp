/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace rmx
{

	template<> char StringTraits<char>::fromUnicode(uint32 code)
	{
		return (code <= 0xff) ? (char)code : (char)0x7f;
	}

	template<> wchar_t StringTraits<wchar_t>::fromUnicode(uint32 code)
	{
		return (wchar_t)code;
	}

	template<> uint32 StringTraits<char>::toUnicode(char ch)
	{
		return (uint32)(uint8)ch;
	}

	template<> uint32 StringTraits<wchar_t>::toUnicode(wchar_t ch)
	{
		return (uint32)ch;
	}

	template<> int StringTraits<char>::buildFormatted(char* dst, size_t dstSize, const char* format, va_list argv)
	{
		return vsnprintf_s(dst, dstSize, dstSize, format, argv);
	}

	template<> int StringTraits<wchar_t>::buildFormatted(wchar_t* dst, size_t dstSize, const wchar_t* format, va_list argv)
	{
		return _vsnwprintf_s(dst, dstSize, dstSize, format, argv);
	}

}



/* ----- Specialised functions for String ----------------------------------------------------------------------- */

String::String(int ignoreMe, const char* format, ...)
{
	init();
	if (nullptr == format || format[0] == 0)
		return;

	va_list argv;
	va_start(argv, format);
	static char buffer[1024];
	rmx::StringTraits<char>::buildFormatted(buffer, 1024, format, argv);
	va_end(argv);

	int len = 0;
	while (buffer[len])
		++len;
	expand(len);
	memcpy(mData, buffer, len * sizeof(char));
	mLength = len;
	mData[mLength] = 0;
}

WString String::toWString() const
{
	WString output;
	output.setLength(mLength);
	wchar_t* dst = output.accessData();
	for (int i = 0; i <= (int)mLength; ++i)
	{
		dst[i] = (wchar_t)mData[i];
	}
	output.mLength = mLength;
	return output;
}

std::string String::toStdString() const
{
	return std::string(operator*());
}

std::wstring String::toStdWString() const
{
	return std::wstring(*toWString());
}



/* ----- Specialised functions for WString ---------------------------------------------------------------------- */

WString::WString(int ignoreMe, const wchar_t* format, ...)
{
	init();
	if (nullptr == format || format[0] == 0)
		return;

	va_list argv;
	va_start(argv, format);
	static wchar_t buffer[1024];
	rmx::StringTraits<wchar_t>::buildFormatted(buffer, 1024, format, argv);
	va_end(argv);

	int len = 0;
	while (buffer[len])
		++len;
	expand(len);
	memcpy(mData, buffer, len * sizeof(wchar_t));
	mLength = len;
	mData[mLength] = 0;
}

String WString::toString() const
{
	String output;
	output.setLength(mLength);
	char* dst = output.accessData();
	for (size_t i = 0; i <= mLength; ++i)
	{
		dst[i] = (mData[i] < 0x100) ? (char)mData[i] : '#';
	}
	return output;
}

String WString::toUTF8() const
{
	String output;
	output.reserve(mLength * 3/2);	// Just a first rough guess

	size_t size = 0;
	for (size_t i = 0; i < (size_t)mLength; ++i)
	{
		const uint32 code = (uint32)mData[i];
		if (code <= 0x7f)
			++size;
		else if (code <= 0x7ff)
			size += 2;
		else if (code <= 0xffff)
			size += 3;
		else
			size += 4;
	}

	output.expand(size);
	uint8* ptr = (uint8*)output.accessData();

	for (size_t i = 0; i < (size_t)mLength; ++i)
	{
		const uint32 code = (uint32)mData[i];
		if (code <= 0x7f)
		{
			ptr[0] = (uint8)code;
			++ptr;
		}
		else if (code <= 0x7ff)
		{
			ptr[0] = 0xc0 + (code >> 6);
			ptr[1] = 0x80 + (code & 0x3f);
			ptr += 2;
		}
		else if (code <= 0xffff)
		{
			ptr[0] = 0xe0 + (code >> 12);
			ptr[1] = 0x80 + ((code >> 6) & 0x3f);
			ptr[2] = 0x80 + (code & 0x3f);
			ptr += 3;
		}
		else
		{
			ptr[0] = 0xf0 + (code >> 18);
			ptr[1] = 0x80 + ((code >> 12) & 0x3f);
			ptr[2] = 0x80 + ((code >> 6) & 0x3f);
			ptr[3] = 0x80 + (code & 0x3f);
			ptr += 4;
		}
	}
	assert(ptr == (uint8*)output.accessData() + size);
	output.mLength = (int)size;
	output[size] = 0;
	return output;
}

std::string WString::toStdString() const
{
	return std::string(*toString());
}

std::wstring WString::toStdWString() const
{
	return std::wstring(operator*());
}

void WString::fromUTF8(const char* str, size_t length)
{
	clear();
	reserve(length);		// Possibly too large, but that's better than reallocations

	while (length > 0)
	{
		const uint32 code = readUTF8(str, length);
		if (code == 0xffffffff)
			break;

		mData[mLength] = code;
		++mLength;
	}
	mData[mLength] = 0;
}

void WString::fromUTF8(const String& str)
{
	fromUTF8(*str, str.length());
}

void WString::fromUTF8(const std::string& str)
{
	fromUTF8(str.c_str(), str.length());
}

uint32 WString::readUTF8(const char*& str, size_t& length)
{
	const char* initialStr = str;
	const uint8 firstByte = (uint8)initialStr[0];
	++str;
	--length;

	if (firstByte <= 0x7f)
	{
		return firstByte;
	}
	else if (firstByte <= 0xbf)
	{
		return 0xffffffff;	// Invalid first byte
	}
	else if (firstByte <= 0xc1)
	{
		return 0xffffffff;	// Invalid alternative encoding for codes 0x00 .. 0x7f
	}
	else if (firstByte <= 0xf4)
	{
		const size_t seqLength = (firstByte <= 0xdf) ? 2 : (firstByte <= 0xef) ? 3 : 4;
		if (length < (seqLength - 1))
			return 0xffffffff;

		str += (seqLength - 1);
		length -= (seqLength - 1);

		uint8 values[4];
		for (size_t k = 1; k < seqLength; ++k)
		{
			values[k] = initialStr[k];
			if (values[k] < 0x80 || values[k] >= 0xc0)
				return 0xffffffff;		// Additional bytes of a sequence have to start with bits 10xxxxxx
			values[k] &= 0x3f;
		}

		if (seqLength == 2)
		{
			return ((uint32)(firstByte & 0x1f) << 6) + ((uint32)values[1]);
		}
		else if (seqLength == 3)
		{
			return ((uint32)(firstByte & 0x0f) << 12) + ((uint32)values[1] << 6) + ((uint32)values[2]);
		}
		else
		{
			return ((uint32)(firstByte & 0x07) << 18) + ((uint32)values[1] << 12) + ((uint32)values[2] << 6) + ((uint32)values[3]);
		}
	}
	else
	{
		return 0xffffffff;		// Too large code (over 0x140000)
	}
}
