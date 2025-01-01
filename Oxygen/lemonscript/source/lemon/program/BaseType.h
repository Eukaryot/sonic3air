/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{

	enum class BaseType : uint8
	{
		VOID	  = 0x00,
		UINT_8	  = 0x10 + 0x00,
		UINT_16	  = 0x10 + 0x01,
		UINT_32	  = 0x10 + 0x02,
		UINT_64	  = 0x10 + 0x03,
		INT_8	  = 0x18 + 0x00,
		INT_16	  = 0x18 + 0x01,
		INT_32	  = 0x18 + 0x02,
		INT_64	  = 0x18 + 0x03,
		BOOL	  = UINT_8,
		INT_CONST = 0x1f,			// Constants have an undefined int type
		FLOAT	  = 0x20,
		DOUBLE	  = 0x21
	};

	struct BaseTypeHelper
	{
		inline static bool isIntegerType(BaseType baseType)			{ return ((uint8)baseType & 0xf0) == 0x10; }
		inline static bool isFloatingPointType(BaseType baseType)	{ return ((uint8)baseType & 0xf0) == 0x20; }

		inline static uint8 getIntegerSizeFlags(BaseType baseType)	{ return (uint8)baseType & 0x03; }
		inline static bool isIntegerSigned(BaseType baseType)		{ return (uint8)baseType & 0x08; }

		inline static BaseType makeIntegerSigned(BaseType baseType)		{ return isIntegerType(baseType) ? (BaseType)((uint8)baseType | 0x08)  : baseType; }
		inline static BaseType makeIntegerUnsigned(BaseType baseType)	{ return isIntegerType(baseType) ? (BaseType)((uint8)baseType & ~0x0c) : baseType; }	// Remove flag 0x08 (signed flag) - and also 0x04 to convert CONST_INT to UINT64
	};



	enum class BaseCastType : uint8
	{
		INVALID = 0xff,
		NONE    = 0x00,

		#define BASE_CAST_TYPE(base, original, target) (base + original * 4 + target)

		// Integer cast up (value is unsigned -> adding zeroes)
		UINT_8_TO_16  = BASE_CAST_TYPE(0x00, 0, 1),
		UINT_8_TO_32  = BASE_CAST_TYPE(0x00, 0, 2),
		UINT_8_TO_64  = BASE_CAST_TYPE(0x00, 0, 3),
		UINT_16_TO_32 = BASE_CAST_TYPE(0x00, 1, 2),
		UINT_16_TO_64 = BASE_CAST_TYPE(0x00, 1, 3),
		UINT_32_TO_64 = BASE_CAST_TYPE(0x00, 2, 3),

		// Integer cast down (signed or unsigned makes no difference here)
		INT_16_TO_8   = BASE_CAST_TYPE(0x00, 1, 0),
		INT_32_TO_8   = BASE_CAST_TYPE(0x00, 2, 0),
		INT_64_TO_8   = BASE_CAST_TYPE(0x00, 3, 0),
		INT_32_TO_16  = BASE_CAST_TYPE(0x00, 2, 1),
		INT_64_TO_16  = BASE_CAST_TYPE(0x00, 3, 1),
		INT_64_TO_32  = BASE_CAST_TYPE(0x00, 3, 2),

		// Integer cast up (value is signed -> adding highest bit)
		SINT_8_TO_16  = BASE_CAST_TYPE(0x10, 0, 1),
		SINT_8_TO_32  = BASE_CAST_TYPE(0x10, 0, 2),
		SINT_8_TO_64  = BASE_CAST_TYPE(0x10, 0, 3),
		SINT_16_TO_32 = BASE_CAST_TYPE(0x10, 1, 2),
		SINT_16_TO_64 = BASE_CAST_TYPE(0x10, 1, 3),
		SINT_32_TO_64 = BASE_CAST_TYPE(0x10, 2, 3),

		// Integer cast to float
		UINT_8_TO_FLOAT   = BASE_CAST_TYPE(0x20, 0, 0),
		UINT_16_TO_FLOAT  = BASE_CAST_TYPE(0x20, 0, 1),
		UINT_32_TO_FLOAT  = BASE_CAST_TYPE(0x20, 0, 2),
		UINT_64_TO_FLOAT  = BASE_CAST_TYPE(0x20, 0, 3),
		SINT_8_TO_FLOAT   = BASE_CAST_TYPE(0x24, 0, 0),
		SINT_16_TO_FLOAT  = BASE_CAST_TYPE(0x24, 0, 1),
		SINT_32_TO_FLOAT  = BASE_CAST_TYPE(0x24, 0, 2),
		SINT_64_TO_FLOAT  = BASE_CAST_TYPE(0x24, 0, 3),

		UINT_8_TO_DOUBLE  = BASE_CAST_TYPE(0x28, 0, 0),
		UINT_16_TO_DOUBLE = BASE_CAST_TYPE(0x28, 0, 1),
		UINT_32_TO_DOUBLE = BASE_CAST_TYPE(0x28, 0, 2),
		UINT_64_TO_DOUBLE = BASE_CAST_TYPE(0x28, 0, 3),
		SINT_8_TO_DOUBLE  = BASE_CAST_TYPE(0x2c, 0, 0),
		SINT_16_TO_DOUBLE = BASE_CAST_TYPE(0x2c, 0, 1),
		SINT_32_TO_DOUBLE = BASE_CAST_TYPE(0x2c, 0, 2),
		SINT_64_TO_DOUBLE = BASE_CAST_TYPE(0x2c, 0, 3),

		// Float cast to integer
		FLOAT_TO_UINT_8   = BASE_CAST_TYPE(0x30, 0, 0),
		FLOAT_TO_UINT_16  = BASE_CAST_TYPE(0x30, 0, 1),
		FLOAT_TO_UINT_32  = BASE_CAST_TYPE(0x30, 0, 2),
		FLOAT_TO_UINT_64  = BASE_CAST_TYPE(0x30, 0, 3),
		FLOAT_TO_SINT_8   = BASE_CAST_TYPE(0x34, 0, 0),
		FLOAT_TO_SINT_16  = BASE_CAST_TYPE(0x34, 0, 1),
		FLOAT_TO_SINT_32  = BASE_CAST_TYPE(0x34, 0, 2),
		FLOAT_TO_SINT_64  = BASE_CAST_TYPE(0x34, 0, 3),

		DOUBLE_TO_UINT_8  = BASE_CAST_TYPE(0x48, 0, 0),
		DOUBLE_TO_UINT_16 = BASE_CAST_TYPE(0x48, 0, 1),
		DOUBLE_TO_UINT_32 = BASE_CAST_TYPE(0x48, 0, 2),
		DOUBLE_TO_UINT_64 = BASE_CAST_TYPE(0x48, 0, 3),
		DOUBLE_TO_SINT_8  = BASE_CAST_TYPE(0x4c, 0, 0),
		DOUBLE_TO_SINT_16 = BASE_CAST_TYPE(0x4c, 0, 1),
		DOUBLE_TO_SINT_32 = BASE_CAST_TYPE(0x4c, 0, 2),
		DOUBLE_TO_SINT_64 = BASE_CAST_TYPE(0x4c, 0, 3),

		// Float cast
		FLOAT_TO_DOUBLE   = 0x40,
		DOUBLE_TO_FLOAT   = 0x41,

		#undef BASE_CAST_TYPE
	};

}
