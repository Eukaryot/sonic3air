/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/ConstantArray.h"


namespace lemon
{
	void ConstantArray::setSize(size_t size)
	{
		mData.resize(size);
	}

	void ConstantArray::setContent(const uint64* values, size_t size)
	{
		setSize(size);
		for (size_t i = 0; i < size; ++i)
			mData[i] = values[i];
	}

	uint64 ConstantArray::getElement(size_t index) const
	{
		return (index < mData.size()) ? mData[index] : 0;
	}

	void ConstantArray::setElement(size_t index, uint64 value)
	{
		if (index < mData.size())
		{
			mData[index] = value;
		}
	}
}
