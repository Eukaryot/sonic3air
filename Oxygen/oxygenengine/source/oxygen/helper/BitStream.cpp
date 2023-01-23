/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/BitStream.h"


Bitstream::Bitstream(std::vector<uint8>& data, uint32 position) :
	mData(data),
	mBytePosition(position),
	mNextBitValue(1)
{
}

uint32 Bitstream::getSize() const
{
	return (mNextBitValue == 1) ? mBytePosition : (mBytePosition + 1);
}

bool Bitstream::read()
{
	const bool result = (mData[mBytePosition] & mNextBitValue) != 0;
	advance();
	return result;
}

void Bitstream::write(bool bit)
{
	if (mNextBitValue == 1 && mBytePosition >= mData.size())
		mData.push_back(0);

	if (bit)
	{
		mData[mBytePosition] |= mNextBitValue;
	}
	else
	{
		mData[mBytePosition] &= ~mNextBitValue;
	}
	advance();
}

void Bitstream::advance()
{
	if (mNextBitValue >= 0x80)
	{
		++mBytePosition;
		mNextBitValue = 1;
	}
	else
	{
		mNextBitValue <<= 1;
	}
}
