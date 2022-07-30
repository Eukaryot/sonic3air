/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/ConstantArray.h"
#include "lemon/program/DataType.h"


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

	void ConstantArray::serializeData(VectorBinarySerializer& serializer)
	{
		serializer.serializeArraySize(mData);
		if (serializer.isReading())
		{
			switch (mElementDataType->getBytes())
			{
				case 1:
				{
					for (uint64& value : mData)
						value = (uint64)serializer.read<uint8>();
					break;
				}
				case 2:
				{
					for (uint64& value : mData)
						value = (uint64)serializer.read<uint16>();
					break;
				}
				case 4:
				{
					for (uint64& value : mData)
						value = (uint64)serializer.read<uint32>();
					break;
				}
				case 8:
				{
					for (uint64& value : mData)
						value = serializer.read<uint64>();
					break;
				}
			}
		}
		else
		{
			switch (mElementDataType->getBytes())
			{
				case 1:
				{
					for (uint64& value : mData)
						serializer.write<uint8>((uint8)value);
					break;
				}
				case 2:
				{
					for (uint64& value : mData)
						serializer.write<uint16>((uint16)value);
					break;
				}
				case 4:
				{
					for (uint64& value : mData)
						serializer.write<uint32>((uint32)value);
					break;
				}
				case 8:
				{
					for (uint64& value : mData)
						serializer.write(value);
					break;
				}
			}
		}
	}
}
