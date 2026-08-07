/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/utility/DataTypeHelper.h"
#include "lemon/utility/AnyBaseValue.h"


namespace lemon
{

	bool DataTypeHelper::negateBaseTypeValue(AnyBaseValue& value, const DataTypeDefinition& dataType)
	{
		switch (dataType.getClass())
		{
			case DataTypeDefinition::Class::INTEGER:
			{
				switch (dataType.getBytes())
				{
					case 1:  value.set<int8>(-value.get<int8>());    return true;
					case 2:  value.set<int16>(-value.get<int16>());  return true;
					case 4:  value.set<int32>(-value.get<int32>());  return true;
					case 8:  value.set<int64>(-value.get<int64>());  return true;
				}
				break;
			}

			case DataTypeDefinition::Class::FLOAT:
			{
				switch (dataType.getBytes())
				{
					case 4:  value.set<float>(-value.get<float>());    return true;
					case 8:  value.set<double>(-value.get<double>());  return true;
				}
				break;
			}

			default:
				return false;
		}
		return false;
	}

	bool DataTypeHelper::isInsideIntegerRange(int64 value, const IntegerDataType& dataType)
	{
		const size_t bits = dataType.getBytes() * 8;
		if (bits >= 64)
			return true;

		if (dataType.mIsSigned)
		{
			if (value >= 0)
				return (value >> bits) == 0;			// Note this does allow for something like "constant s16 a = 0xf000"
			else
				return (value >> (bits - 1)) == ~0ull;
		}
		else
		{
			return (value >> bits) == 0;
		}
	}

}
