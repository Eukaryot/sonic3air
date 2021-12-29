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
		static BaseType getCastExecType(BaseCastType castType)
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

				default:
					throw std::runtime_error("Unrecognized cast type");
			}
			return BaseType::UINT_64;
		}

		static BaseType getCastExecType(const Opcode& opcode)
		{
			return OpcodeHelper::getCastExecType(static_cast<BaseCastType>(opcode.mParameter));
		}
	};

}
