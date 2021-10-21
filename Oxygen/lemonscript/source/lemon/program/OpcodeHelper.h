/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"


namespace lemon
{

	class OpcodeHelper
	{
	public:
		static BaseType getCastExecType(CastType castType)
		{
			switch (castType)
			{
				// Cast down (signed or unsigned makes no difference here)
				case CastType::INT16_TO_INT8:  return BaseType::INT_8;
				case CastType::INT32_TO_INT8:  return BaseType::INT_8;
				case CastType::INT64_TO_INT8:  return BaseType::INT_8;
				case CastType::INT32_TO_INT16: return BaseType::INT_16;
				case CastType::INT64_TO_INT16: return BaseType::INT_16;
				case CastType::INT64_TO_INT32: return BaseType::INT_32;

				// Cast up (value is unsigned -> adding zeroes)
				case CastType::UINT8_TO_INT16:  return BaseType::UINT_8;
				case CastType::UINT8_TO_INT32:  return BaseType::UINT_8;
				case CastType::UINT8_TO_INT64:  return BaseType::UINT_8;
				case CastType::UINT16_TO_INT32: return BaseType::UINT_16;
				case CastType::UINT16_TO_INT64: return BaseType::UINT_16;
				case CastType::UINT32_TO_INT64: return BaseType::UINT_32;

				// Cast up (value is signed -> adding highest bit)
				case CastType::SINT8_TO_INT16:  return BaseType::INT_8;
				case CastType::SINT8_TO_INT32:  return BaseType::INT_8;
				case CastType::SINT8_TO_INT64:  return BaseType::INT_8;
				case CastType::SINT16_TO_INT32: return BaseType::INT_16;
				case CastType::SINT16_TO_INT64: return BaseType::INT_16;
				case CastType::SINT32_TO_INT64: return BaseType::INT_32;

				default:
					throw std::runtime_error("Unrecognized cast type");
			}
			return BaseType::UINT_64;
		}

		static BaseType getCastExecType(const Opcode& opcode)
		{
			return OpcodeHelper::getCastExecType(static_cast<CastType>(opcode.mParameter));
		}
	};

}
