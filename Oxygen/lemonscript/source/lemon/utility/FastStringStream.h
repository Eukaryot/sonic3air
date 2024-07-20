/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon::detail
{
	class FastStringStream
	{
	public:
		void clear()
		{
			mLength = 0;
		}

		void addChar(char ch)
		{
			RMX_CHECK(mLength < 0x100, "Too long string", return);
			mBuffer[mLength] = ch;
			++mLength;
		}

		void addString(const char* str, int length)
		{
			RMX_CHECK(mLength + length <= 0x100, "Too long string", return);
			memcpy(&mBuffer[mLength], str, length);
			mLength += length;
		}

		void addString(std::string_view str)
		{
			addString(str.data(), (int)str.length());
		}

		void addDecimal(int64 value, int minDigits = 0)
		{
			if (value < 0)
			{
				RMX_CHECK(mLength < 0x100, "Too long string", return);
				mBuffer[mLength] = '-';
				++mLength;
				value = -value;
			}
			else if (value == 0)
			{
				const int numDigits = std::max(1, minDigits);
				RMX_CHECK(mLength + numDigits <= 0x100, "Too long string", return);
				for (int k = 0; k < numDigits; ++k)
					mBuffer[mLength+k] = '0';
				mLength += numDigits;
				return;
			}

			int numDigits = 1;
			int64 digitMax = 10;	// One more than the maximum number representable using numDigits
			while (numDigits < 19 && (digitMax <= value || numDigits < minDigits))
			{
				++numDigits;
				digitMax *= 10;
			}
			for (int64 digitBase = digitMax / 10; digitBase > 0; digitBase /= 10)
			{
				RMX_CHECK(mLength < 0x100, "Too long string", return);
				mBuffer[mLength] = '0' + (char)((value / digitBase) % 10);
				++mLength;
			}
		}

		void addBinary(uint64 value, int minDigits)
		{
			int shift = 1;
			while (shift < 64 && ((value >> shift) != 0 || shift < minDigits))
			{
				++shift;
			}
			RMX_CHECK(mLength + shift <= 0x100, "Too long string", return);
			for (--shift; shift >= 0; --shift)
			{
				const int binValue = (value >> shift) & 0x01;
				mBuffer[mLength] = (binValue == 0) ? '0' : '1';
				++mLength;
			}
		}

		void addHex(uint64 value, int minDigits)
		{
			int shift = 4;
			while (shift < 64 && ((value >> shift) != 0 || shift < minDigits * 4))
			{
				shift += 4;
			}
			RMX_CHECK(mLength + shift / 4 <= 0x100, "Too long string", return);
			for (shift -= 4; shift >= 0; shift -= 4)
			{
				const int hexValue = (value >> shift) & 0x0f;
				mBuffer[mLength] = (hexValue <= 9) ? ('0' + (char)hexValue) : ('a' + (char)(hexValue - 10));
				++mLength;
			}
		}

		void addFloat(float value)
		{
			std::stringstream str;
			str << value;
			addString(str.str());
		}

		void addDouble(double value)
		{
			std::stringstream str;
			str << value;
			addString(str.str());
		}

	public:
		char mBuffer[0x100] = { 0 };
		int mLength = 0;
	};
}
