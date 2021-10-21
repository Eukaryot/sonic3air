/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class Bitstream
{
public:
	Bitstream(std::vector<uint8>& data, uint32 position = 0);

	uint32 getSize() const;

	bool read();
	void write(bool bit);

private:
	void advance();

private:
	std::vector<uint8>& mData;
	uint32 mBytePosition;		// Position inside mData
	uint32 mNextBitValue;		// Always a power of two between 1 and 128
};
