/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"


namespace lemon
{

	// This helper is only used by nativizer, after usage was replaced elsewhere
	class OpcodeHelper
	{
	public:
		static BaseType getCastSourceType(BaseCastType castType)
		{
			switch (castType)
			{
				// Cast down (signed or unsigned makes no difference here)
				case BaseCastType::INT_16_TO_8:  return BaseType::INT_16;
				case BaseCastType::INT_32_TO_8:  return BaseType::INT_32;
				case BaseCastType::INT_64_TO_8:  return BaseType::INT_64;
				case BaseCastType::INT_32_TO_16: return BaseType::INT_32;
				case BaseCastType::INT_64_TO_16: return BaseType::INT_64;
				case BaseCastType::INT_64_TO_32: return BaseType::INT_64;

				// Cast up (value is unsigned -> adding zeroes)
				case BaseCastType::UINT_8_TO_16:  return BaseType::UINT_8;
				case BaseCastType::UINT_8_TO_32:  return BaseType::UINT_8;
				case BaseCastType::UINT_8_TO_64:  return BaseType::UINT_8;
				case BaseCastType::UINT_16_TO_32: return BaseType::UINT_16;
				case BaseCastType::UINT_16_TO_64: return BaseType::UINT_16;
				case BaseCastType::UINT_32_TO_64: return BaseType::UINT_32;

				// Cast up (value is signed -> adding highest bit)
				case BaseCastType::SINT_8_TO_16:  return BaseType::INT_8;
				case BaseCastType::SINT_8_TO_32:  return BaseType::INT_8;
				case BaseCastType::SINT_8_TO_64:  return BaseType::INT_8;
				case BaseCastType::SINT_16_TO_32: return BaseType::INT_16;
				case BaseCastType::SINT_16_TO_64: return BaseType::INT_16;
				case BaseCastType::SINT_32_TO_64: return BaseType::INT_32;

				// Integer cast to float
				case BaseCastType::UINT_8_TO_FLOAT:  return BaseType::UINT_8;
				case BaseCastType::UINT_16_TO_FLOAT: return BaseType::UINT_16;
				case BaseCastType::UINT_32_TO_FLOAT: return BaseType::UINT_32;
				case BaseCastType::UINT_64_TO_FLOAT: return BaseType::UINT_64;
				case BaseCastType::SINT_8_TO_FLOAT:  return BaseType::INT_8;
				case BaseCastType::SINT_16_TO_FLOAT: return BaseType::INT_16;
				case BaseCastType::SINT_32_TO_FLOAT: return BaseType::INT_32;
				case BaseCastType::SINT_64_TO_FLOAT: return BaseType::INT_64;

				case BaseCastType::UINT_8_TO_DOUBLE:  return BaseType::UINT_8;
				case BaseCastType::UINT_16_TO_DOUBLE: return BaseType::UINT_16;
				case BaseCastType::UINT_32_TO_DOUBLE: return BaseType::UINT_32;
				case BaseCastType::UINT_64_TO_DOUBLE: return BaseType::UINT_64;
				case BaseCastType::SINT_8_TO_DOUBLE:  return BaseType::INT_8;
				case BaseCastType::SINT_16_TO_DOUBLE: return BaseType::INT_16;
				case BaseCastType::SINT_32_TO_DOUBLE: return BaseType::INT_32;
				case BaseCastType::SINT_64_TO_DOUBLE: return BaseType::INT_64;

				// Float cast to integer
				case BaseCastType::FLOAT_TO_UINT_8:  return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_UINT_16: return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_UINT_32: return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_UINT_64: return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_SINT_8:  return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_SINT_16: return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_SINT_32: return BaseType::FLOAT;
				case BaseCastType::FLOAT_TO_SINT_64: return BaseType::FLOAT;

				case BaseCastType::DOUBLE_TO_UINT_8:  return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_UINT_16: return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_UINT_32: return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_UINT_64: return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_SINT_8:  return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_SINT_16: return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_SINT_32: return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_SINT_64: return BaseType::DOUBLE;

				// Float cast
				case BaseCastType::FLOAT_TO_DOUBLE: return BaseType::FLOAT;
				case BaseCastType::DOUBLE_TO_FLOAT: return BaseType::DOUBLE;

				default:
					throw std::runtime_error("Unrecognized cast type");
			}
			return BaseType::UINT_64;
		}

		static BaseType getCastSourceType(const Opcode& opcode)
		{
			return OpcodeHelper::getCastSourceType(static_cast<BaseCastType>(opcode.mParameter));
		}

		static BaseType getCastTargetType(BaseCastType castType)
		{
			switch (castType)
			{
				// Cast down (signed or unsigned makes no difference here)
				case BaseCastType::INT_16_TO_8:  return BaseType::INT_8;
				case BaseCastType::INT_32_TO_8:  return BaseType::INT_8;
				case BaseCastType::INT_64_TO_8:  return BaseType::INT_8;
				case BaseCastType::INT_32_TO_16: return BaseType::INT_16;
				case BaseCastType::INT_64_TO_16: return BaseType::INT_16;
				case BaseCastType::INT_64_TO_32: return BaseType::INT_32;

				// Cast up (value is unsigned -> adding zeroes)
				case BaseCastType::UINT_8_TO_16:  return BaseType::UINT_16;
				case BaseCastType::UINT_8_TO_32:  return BaseType::UINT_32;
				case BaseCastType::UINT_8_TO_64:  return BaseType::UINT_64;
				case BaseCastType::UINT_16_TO_32: return BaseType::UINT_32;
				case BaseCastType::UINT_16_TO_64: return BaseType::UINT_64;
				case BaseCastType::UINT_32_TO_64: return BaseType::UINT_64;

				// Cast up (value is signed -> adding highest bit)
				case BaseCastType::SINT_8_TO_16:  return BaseType::INT_16;
				case BaseCastType::SINT_8_TO_32:  return BaseType::INT_32;
				case BaseCastType::SINT_8_TO_64:  return BaseType::INT_64;
				case BaseCastType::SINT_16_TO_32: return BaseType::INT_32;
				case BaseCastType::SINT_16_TO_64: return BaseType::INT_64;
				case BaseCastType::SINT_32_TO_64: return BaseType::INT_64;

				// Integer cast to float
				case BaseCastType::UINT_8_TO_FLOAT:  return BaseType::FLOAT;
				case BaseCastType::UINT_16_TO_FLOAT: return BaseType::FLOAT;
				case BaseCastType::UINT_32_TO_FLOAT: return BaseType::FLOAT;
				case BaseCastType::UINT_64_TO_FLOAT: return BaseType::FLOAT;
				case BaseCastType::SINT_8_TO_FLOAT:  return BaseType::FLOAT;
				case BaseCastType::SINT_16_TO_FLOAT: return BaseType::FLOAT;
				case BaseCastType::SINT_32_TO_FLOAT: return BaseType::FLOAT;
				case BaseCastType::SINT_64_TO_FLOAT: return BaseType::FLOAT;

				case BaseCastType::UINT_8_TO_DOUBLE:  return BaseType::DOUBLE;
				case BaseCastType::UINT_16_TO_DOUBLE: return BaseType::DOUBLE;
				case BaseCastType::UINT_32_TO_DOUBLE: return BaseType::DOUBLE;
				case BaseCastType::UINT_64_TO_DOUBLE: return BaseType::DOUBLE;
				case BaseCastType::SINT_8_TO_DOUBLE:  return BaseType::DOUBLE;
				case BaseCastType::SINT_16_TO_DOUBLE: return BaseType::DOUBLE;
				case BaseCastType::SINT_32_TO_DOUBLE: return BaseType::DOUBLE;
				case BaseCastType::SINT_64_TO_DOUBLE: return BaseType::DOUBLE;

				// Float cast to integer
				case BaseCastType::FLOAT_TO_UINT_8:  return BaseType::UINT_8;
				case BaseCastType::FLOAT_TO_UINT_16: return BaseType::UINT_16;
				case BaseCastType::FLOAT_TO_UINT_32: return BaseType::UINT_32;
				case BaseCastType::FLOAT_TO_UINT_64: return BaseType::UINT_64;
				case BaseCastType::FLOAT_TO_SINT_8:  return BaseType::INT_8;
				case BaseCastType::FLOAT_TO_SINT_16: return BaseType::INT_16;
				case BaseCastType::FLOAT_TO_SINT_32: return BaseType::INT_32;
				case BaseCastType::FLOAT_TO_SINT_64: return BaseType::INT_64;

				case BaseCastType::DOUBLE_TO_UINT_8:  return BaseType::UINT_8;
				case BaseCastType::DOUBLE_TO_UINT_16: return BaseType::UINT_16;
				case BaseCastType::DOUBLE_TO_UINT_32: return BaseType::UINT_32;
				case BaseCastType::DOUBLE_TO_UINT_64: return BaseType::UINT_64;
				case BaseCastType::DOUBLE_TO_SINT_8:  return BaseType::INT_8;
				case BaseCastType::DOUBLE_TO_SINT_16: return BaseType::INT_16;
				case BaseCastType::DOUBLE_TO_SINT_32: return BaseType::INT_32;
				case BaseCastType::DOUBLE_TO_SINT_64: return BaseType::INT_64;

				// Float cast
				case BaseCastType::FLOAT_TO_DOUBLE: return BaseType::DOUBLE;
				case BaseCastType::DOUBLE_TO_FLOAT: return BaseType::FLOAT;

				default:
					throw std::runtime_error("Unrecognized cast type");
			}
			return BaseType::UINT_64;
		}

		static BaseType getCastTargetType(const Opcode& opcode)
		{
			return OpcodeHelper::getCastTargetType(static_cast<BaseCastType>(opcode.mParameter));
		}
	};

}
