/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/BaseType.h"


namespace lemon
{

	size_t BaseTypeHelper::getSizeOfBaseType(BaseType baseType)
	{
		switch (baseType)
		{
			case BaseType::UINT_8:		return 1;
			case BaseType::UINT_16:		return 2;
			case BaseType::UINT_32:		return 4;
			case BaseType::UINT_64:		return 8;
			case BaseType::INT_8:		return 1;
			case BaseType::INT_16:		return 2;
			case BaseType::INT_32:		return 4;
			case BaseType::INT_64:		return 8;
			case BaseType::INT_CONST:	return 8;
			case BaseType::FLOAT:		return 4;
			case BaseType::DOUBLE:		return 8;
			default:					return 0;
		}
	}

	bool BaseTypeHelper::isPureIntegerBaseCast(BaseCastType baseCastType)
	{
		const uint8 value = (uint8)baseCastType;
		return (value > 0 && value < 0x20);
	}

}
