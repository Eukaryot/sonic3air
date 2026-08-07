/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"


namespace lemon
{
	struct AnyBaseValue;


	class DataTypeHelper
	{
	public:
		static bool negateBaseTypeValue(AnyBaseValue& value, const DataTypeDefinition& dataType);
		static bool isInsideIntegerRange(int64 value, const IntegerDataType& dataType);
	};

}
