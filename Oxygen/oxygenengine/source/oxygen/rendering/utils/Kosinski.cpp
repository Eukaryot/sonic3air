/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/utils/Kosinski.h"
#include "oxygen/simulation/EmulatorInterface.h"


namespace
{

	struct ByteReader
	{
		EmulatorInterface& emulatorInterface;
		uint32& address;

		inline ByteReader(EmulatorInterface& ei, uint32& source) : emulatorInterface(ei), address(source) {}

		uint8 nextByte()
		{
			const uint8 result = emulatorInterface.readMemory8(address);
			++address;
			return result;
		}

		uint16 nextWord()
		{
			const uint16 result = swapBytes16(emulatorInterface.readMemory16(address));
			address += 2;
			return result;
		}
	};

	struct BitReader
	{
		ByteReader& byteReader;
		uint16 buffer = 0;
		int8 remainingBits = 0;

		inline BitReader(ByteReader& reader) : byteReader(reader) {}

		void init()
		{
			buffer = byteReader.nextWord();
			remainingBits = 15;
		}

		uint8 nextBit()
		{
			uint8 bit = (buffer & 1);
			buffer >>= 1;

			--remainingBits;
			if (remainingBits < 0)
			{
				buffer = byteReader.nextWord();
				remainingBits = 15;
			}
			return bit;
		}
	};

}



void Kosinski::decompress(uint8*& output, uint32& inputAddress)
{
	const uint8* initialOutput = output;
	ByteReader byteReader(EmulatorInterface::instance(), inputAddress);
	BitReader bitReader(byteReader);
	bitReader.init();

	while (true)
	{
		while (bitReader.nextBit() == 1)
		{
			*output = byteReader.nextByte();
			++output;
		}

		uint16 length = 0;
		int16 offset = -1;
		if (bitReader.nextBit() == 0)
		{
			// Copy something between 2 and 5 bytes with an 8-bit large offset
			const uint8 result1 = bitReader.nextBit();
			const uint8 result2 = bitReader.nextBit();
			length = (result1 * 2) + result2 + 2;
			offset = byteReader.nextByte() - 0x100;
		}
		else
		{
			const uint8 bits1 = byteReader.nextByte();
			const uint8 bits2 = byteReader.nextByte();
			const uint8 bits2a = bits2 & 0xf8;
			const uint8 bits2b = bits2 & 0x07;
			offset = (uint16(bits2a) << 5) + bits1 - 0x2000;

			if (bits2b == 0)
			{
				const uint8 value = byteReader.nextByte();
				if (value == 0)
					return;
				if (value == 1)
					continue;

				length = uint16(value) + 1;
			}
			else
			{
				length = bits2b + 2;
			}
		}

		RMX_CHECK(offset < 0, "Invalid offset " << offset << " in Kosinski compression of data at roughly " << rmx::hexString(inputAddress, 6) << ", offset must be negative", return);
		// In non-debug builds, silently ignore this issue and just check for broken access inside -- this behavior is needed for the "Side-view Emeralds" mod for S3AIR
		RMX_ASSERT(output + offset >= initialOutput, "Kosinski decompression is accessing data outside of what was written before (position = " << (int)(output - initialOutput) << ", offset = " << offset << ") at roughly " << rmx::hexString(inputAddress));
		while (length > 0)
		{
			output[0] = (output + offset < initialOutput) ? 0 : output[offset];
			++output;
			--length;
		}
	}
}
