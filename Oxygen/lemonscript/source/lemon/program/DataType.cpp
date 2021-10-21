/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/DataType.h"


namespace lemon
{

	const std::string& DataTypeDefinition::toString() const
	{
		RMX_ASSERT(mClass == Class::VOID, "Base class call to 'DataTypeDefinition::toString' is only allowed for the void type");
		static const std::string TYPE_STRING_VOID = "void";
		return TYPE_STRING_VOID;
	}

	const std::string& IntegerDataType::toString() const
	{
		switch (mSemantics)
		{
			case Semantics::BOOLEAN:
			{
				static const std::string TYPE_STRING_BOOL = "bool";
				return TYPE_STRING_BOOL;
			}

			case Semantics::CONSTANT:
			{
				static const std::string TYPE_STRING_CONST_INT = "const_int";
				return TYPE_STRING_CONST_INT;
			}

			default:
			{
				static const std::string TYPE_STRINGS[] = { "s8", "s16", "s32", "s64", "u8", "u16", "u32", "u64" };
				const std::string* typeStrings = mIsSigned ? &TYPE_STRINGS[0] : &TYPE_STRINGS[4];
				if (mBytes <= 1)
					return typeStrings[0];
				else if (mBytes <= 2)
					return typeStrings[1];
				else if (mBytes <= 4)
					return typeStrings[2];
				else
					return typeStrings[3];
			}
		}
	}

}
