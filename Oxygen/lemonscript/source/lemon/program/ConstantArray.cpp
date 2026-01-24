/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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

	void ConstantArray::setContent(const AnyBaseValue* values, size_t size)
	{
		setSize(size);
		for (size_t i = 0; i < size; ++i)
			mData[i] = values[i];
	}

	const AnyBaseValue& ConstantArray::getElement(size_t index) const
	{
		static AnyBaseValue EMPTY;
		return (index < mData.size()) ? mData[index] : EMPTY;
	}

	void ConstantArray::setElement(size_t index, const AnyBaseValue& value)
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
			if (mElementDataType->isA<FloatDataType>())
			{
				switch (mElementDataType->getBytes())
				{
					case 4:
					{
						for (AnyBaseValue& value : mData)
							value.set(serializer.read<float>());
						break;
					}
					case 8:
					{
						for (AnyBaseValue& value : mData)
							value.set(serializer.read<double>());
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
						for (AnyBaseValue& value : mData)
							value.set(serializer.read<uint8>());
						break;
					}
					case 2:
					{
						for (AnyBaseValue& value : mData)
							value.set(serializer.read<uint16>());
						break;
					}
					case 4:
					{
						for (AnyBaseValue& value : mData)
							value.set(serializer.read<uint32>());
						break;
					}
					case 8:
					{
						for (AnyBaseValue& value : mData)
							value.set(serializer.read<uint64>());
						break;
					}
				}
			}
		}
		else
		{
			if (mElementDataType->isA<FloatDataType>())
			{
				switch (mElementDataType->getBytes())
				{
					case 4:
					{
						for (const AnyBaseValue& value : mData)
							serializer.write(value.get<float>());
						break;
					}
					case 8:
					{
						for (const AnyBaseValue& value : mData)
							serializer.write(value.get<double>());
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
						for (const AnyBaseValue& value : mData)
							serializer.write(value.get<uint8>());
						break;
					}
					case 2:
					{
						for (const AnyBaseValue& value : mData)
							serializer.write(value.get<uint16>());
						break;
					}
					case 4:
					{
						for (const AnyBaseValue& value : mData)
							serializer.write(value.get<uint32>());
						break;
					}
					case 8:
					{
						for (const AnyBaseValue& value : mData)
							serializer.write(value.get<uint64>());
						break;
					}
				}
			}
		}
	}
}
