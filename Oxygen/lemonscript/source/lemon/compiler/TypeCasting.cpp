/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/TypeCasting.h"
#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/Utility.h"
#include "lemon/program/DataType.h"


namespace lemon
{

	const std::vector<TypeCasting::BinaryOperatorSignature>& TypeCasting::getBinarySignaturesForOperator(Operator op)
	{
		static const std::vector<BinaryOperatorSignature> signaturesAssignment =
		{
			BinaryOperatorSignature(&PredefinedDataTypes::INT_64,  &PredefinedDataTypes::INT_64,  &PredefinedDataTypes::INT_64),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_64, &PredefinedDataTypes::UINT_64, &PredefinedDataTypes::UINT_64),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_32,  &PredefinedDataTypes::INT_32,  &PredefinedDataTypes::INT_32),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_32, &PredefinedDataTypes::UINT_32, &PredefinedDataTypes::UINT_32),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_16,  &PredefinedDataTypes::INT_16,  &PredefinedDataTypes::INT_16),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_16, &PredefinedDataTypes::UINT_16, &PredefinedDataTypes::UINT_16),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_8,   &PredefinedDataTypes::INT_8,   &PredefinedDataTypes::INT_8),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::UINT_8),
			BinaryOperatorSignature(&PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::FLOAT),
			BinaryOperatorSignature(&PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::DOUBLE),
			BinaryOperatorSignature(&PredefinedDataTypes::STRING,  &PredefinedDataTypes::STRING,  &PredefinedDataTypes::STRING),
			BinaryOperatorSignature(&PredefinedDataTypes::ANY,	   &PredefinedDataTypes::ANY,	  &PredefinedDataTypes::ANY)
		};
		static const std::vector<BinaryOperatorSignature> signaturesSymmetric =
		{
			// TODO: This is oversimplified, there are cases like multiply and left-shift (and probably also add / subtract) that require different handling
			BinaryOperatorSignature(&PredefinedDataTypes::INT_64,  &PredefinedDataTypes::INT_64,  &PredefinedDataTypes::INT_64),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_64, &PredefinedDataTypes::UINT_64, &PredefinedDataTypes::UINT_64),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_32,  &PredefinedDataTypes::INT_32,  &PredefinedDataTypes::INT_32),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_32, &PredefinedDataTypes::UINT_32, &PredefinedDataTypes::UINT_32),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_16,  &PredefinedDataTypes::INT_16,  &PredefinedDataTypes::INT_16),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_16, &PredefinedDataTypes::UINT_16, &PredefinedDataTypes::UINT_16),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_8,   &PredefinedDataTypes::INT_8,   &PredefinedDataTypes::INT_8),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::UINT_8),
			BinaryOperatorSignature(&PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::FLOAT),
			BinaryOperatorSignature(&PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::DOUBLE),
			BinaryOperatorSignature(&PredefinedDataTypes::STRING,  &PredefinedDataTypes::STRING,  &PredefinedDataTypes::STRING)		// TODO: Strings need their own binary operations (and only few of them make actual sense...)
		};
		static const std::vector<BinaryOperatorSignature> signaturesComparison =
		{
			// Result types are always bool
			BinaryOperatorSignature(&PredefinedDataTypes::INT_64,  &PredefinedDataTypes::INT_64,  &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_64, &PredefinedDataTypes::UINT_64, &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_32,  &PredefinedDataTypes::INT_32,  &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_32, &PredefinedDataTypes::UINT_32, &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_16,  &PredefinedDataTypes::INT_16,  &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_16, &PredefinedDataTypes::UINT_16, &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::INT_8,   &PredefinedDataTypes::INT_8,   &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::BOOL),
			BinaryOperatorSignature(&PredefinedDataTypes::STRING,  &PredefinedDataTypes::STRING,  &PredefinedDataTypes::BOOL)		// TODO: Strings need their own comparison operations
		};
		static const std::vector<BinaryOperatorSignature> signaturesTrinary =
		{
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::INT_64,  &PredefinedDataTypes::INT_64),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::UINT_64, &PredefinedDataTypes::UINT_64),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::INT_32,  &PredefinedDataTypes::INT_32),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::UINT_32, &PredefinedDataTypes::UINT_32),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::INT_16,  &PredefinedDataTypes::INT_16),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::UINT_16, &PredefinedDataTypes::UINT_16),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::INT_8,   &PredefinedDataTypes::INT_8),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::UINT_8,  &PredefinedDataTypes::UINT_8),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::FLOAT,   &PredefinedDataTypes::FLOAT),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::DOUBLE,  &PredefinedDataTypes::DOUBLE),
			BinaryOperatorSignature(&PredefinedDataTypes::BOOL, &PredefinedDataTypes::STRING,  &PredefinedDataTypes::STRING)
		};

		switch (OperatorHelper::getOperatorType(op))
		{
			case OperatorHelper::OperatorType::ASSIGNMENT:
				return signaturesAssignment;

			case OperatorHelper::OperatorType::SYMMETRIC:
				return signaturesSymmetric;

			case OperatorHelper::OperatorType::COMPARISON:
				return signaturesComparison;

			case OperatorHelper::OperatorType::TRINARY:
				return signaturesTrinary;

			default:
				// This should never happen
				CHECK_ERROR_NOLINE(false, "Unknown operator type");
				return signaturesTrinary;
		}
	}

	bool TypeCasting::canImplicitlyCastTypes(const DataTypeDefinition& original, const DataTypeDefinition& target) const
	{
		const CastHandling handling = getCastHandling(&original, &target, false);
		return (handling.mResult != CastHandling::Result::INVALID);
	}

	bool TypeCasting::canExplicitlyCastTypes(const DataTypeDefinition& original, const DataTypeDefinition& target) const
	{
		const CastHandling handling = getCastHandling(&original, &target, true);
		return (handling.mResult != CastHandling::Result::INVALID);
	}

	TypeCasting::CastHandling TypeCasting::getCastHandling(const DataTypeDefinition* original, const DataTypeDefinition* target, bool explicitCast) const
	{
		if (original == target)
		{
			// No cast needed
			return CastHandling(CastHandling::Result::NO_CAST, 0);
		}

		// Treat string type as u64, but only for the original type (to allow for converting a string to an integer, but not the other way round)
		//  -> We make an exception for script feature level 1, for the sake of mod compatibility
		{
			if (original == &PredefinedDataTypes::STRING)
				original = &PredefinedDataTypes::UINT_64;

			if (mCompileOptions.mScriptFeatureLevel < 2)
			{
				if (target == &PredefinedDataTypes::STRING)
					target = &PredefinedDataTypes::UINT_64;
			}

			if (original == target)
			{
				// It's a conversion between string and u64
				return CastHandling(CastHandling::Result::NO_CAST, 1);
			}
		}

		const bool originalIsBaseType = (original->getClass() == DataTypeDefinition::Class::INTEGER || original->getClass() == DataTypeDefinition::Class::FLOAT);
		const bool targetIsBaseType = (target->getClass() == DataTypeDefinition::Class::INTEGER || target->getClass() == DataTypeDefinition::Class::FLOAT);
		if (originalIsBaseType && targetIsBaseType)
		{
			if (original->getClass() == DataTypeDefinition::Class::INTEGER)
			{
				if (target->getClass() == DataTypeDefinition::Class::INTEGER)
				{
					// Cast between integers
					const IntegerDataType& originalInt = original->as<IntegerDataType>();
					const IntegerDataType& targetInt = target->as<IntegerDataType>();

					uint8 castPriority = 0;
					if (originalInt.mSemantics == IntegerDataType::Semantics::CONSTANT)
					{
						// Const may get cast to every integer type
						castPriority = 1;
					}
					else if (targetInt.mSemantics == IntegerDataType::Semantics::CONSTANT)
					{
						// Can this happen at all?
						castPriority = 1;
					}
					else if (originalInt.getBytes() == targetInt.getBytes())
					{
						castPriority = (originalInt.mIsSigned && !targetInt.mIsSigned) ? 0x02 : 0x01;
					}
					else if (originalInt.getBytes() < targetInt.getBytes())
					{
						// Up cast
						castPriority = ((originalInt.mIsSigned && !targetInt.mIsSigned) ? 0x20 : 0x10) + (targetInt.mSizeBits - originalInt.mSizeBits);
					}
					else
					{
						// Down cast
						castPriority = ((originalInt.mIsSigned && !targetInt.mIsSigned) ? 0x40 : 0x30) + (originalInt.mSizeBits - targetInt.mSizeBits);
					}

					// No need for an opcode if size does not change at all
					if (originalInt.getBytes() != targetInt.getBytes())
					{
						uint8 castTypeBits = (originalInt.mSizeBits * 4) + targetInt.mSizeBits;
						if (originalInt.mIsSigned && targetInt.getBytes() > originalInt.getBytes())		// Recognize signed up-cast
						{
							castTypeBits += 0x10;
						}
						return CastHandling((BaseCastType)castTypeBits, castPriority);
					}
					else
					{
						// No cast needed
						return CastHandling(CastHandling::Result::NO_CAST, castPriority);
					}
				}
				else
				{
					// TODO: Can we handle CONST_INT by doing a compile-time conversion of the constant value?

					// Cast from integer to floating point type
					const IntegerDataType& originalInt = original->as<IntegerDataType>();
					uint8 castTypeBits = 0x20 + originalInt.mSizeBits;
					castTypeBits += originalInt.mIsSigned ? 0x04 : 0;
					castTypeBits += (target->getBytes() == 8) ? 0x08 : 0;
					return CastHandling((BaseCastType)castTypeBits, 0x50 + originalInt.mSizeBits);
				}
			}
			else
			{
				if (target->getClass() == DataTypeDefinition::Class::INTEGER)
				{
					// Cast from floating point type to integer
					//  -> This needs to be done explicitly
					if (explicitCast)
					{
						const IntegerDataType& targetInt = target->as<IntegerDataType>();
						uint8 castTypeBits = 0x30 + targetInt.mSizeBits;
						castTypeBits += targetInt.mIsSigned ? 0x04 : 0;
						castTypeBits += (original->getBytes() == 8) ? 0x08 : 0;
						return CastHandling((BaseCastType)castTypeBits, 0x54 - targetInt.mSizeBits);
					}
				}
				else
				{
					// Cast from float to double or vice versa
					const BaseCastType baseCastType = (original->getBytes() < target->getBytes()) ? BaseCastType::FLOAT_TO_DOUBLE : BaseCastType::DOUBLE_TO_FLOAT;
					return CastHandling(baseCastType, 0x48);
				}
			}
		}

		if (target->getClass() == DataTypeDefinition::Class::ANY)
		{
			// Any cast has a very low priority
			return CastHandling(CastHandling::Result::ANY_CAST, 0xf0);
		}

		return CastHandling(CastHandling::Result::INVALID, 0xff);
	}

	TypeCasting::CastHandling TypeCasting::castBaseValue(const AnyBaseValue& originalValue, const DataTypeDefinition* originalType, AnyBaseValue& outTargetValue, const DataTypeDefinition* targetType) const
	{
		outTargetValue = originalValue;
		const CastHandling castHandling = getCastHandling(originalType, targetType, false);
		if (castHandling.mResult == CastHandling::Result::BASE_CAST)
		{
			switch (castHandling.mBaseCastType)
			{
				case BaseCastType::INVALID:														break;
				case BaseCastType::UINT_8_TO_16:		outTargetValue.cast<uint8,  uint16>();	break;
				case BaseCastType::UINT_8_TO_32:		outTargetValue.cast<uint8,  uint32>();	break;
				case BaseCastType::UINT_8_TO_64:		outTargetValue.cast<uint8,  uint64>();	break;
				case BaseCastType::UINT_16_TO_32:		outTargetValue.cast<uint16, uint32>();	break;
				case BaseCastType::UINT_16_TO_64:		outTargetValue.cast<uint16, uint64>();	break;
				case BaseCastType::UINT_32_TO_64:		outTargetValue.cast<uint32, uint64>();	break;
				case BaseCastType::INT_16_TO_8:			outTargetValue.cast<int16,  int8  >();	break;
				case BaseCastType::INT_32_TO_8:			outTargetValue.cast<int32,  int8  >();	break;
				case BaseCastType::INT_64_TO_8:			outTargetValue.cast<int64,  int8  >();	break;
				case BaseCastType::INT_32_TO_16:		outTargetValue.cast<int32,  int16 >();	break;
				case BaseCastType::INT_64_TO_16:		outTargetValue.cast<int64,  int16 >();	break;
				case BaseCastType::INT_64_TO_32:		outTargetValue.cast<int64,  int32 >();	break;
				case BaseCastType::SINT_8_TO_16:		outTargetValue.cast<int8,   int16 >();	break;
				case BaseCastType::SINT_8_TO_32:		outTargetValue.cast<int8,   int32 >();	break;
				case BaseCastType::SINT_8_TO_64:		outTargetValue.cast<int8,   int64 >();	break;
				case BaseCastType::SINT_16_TO_32:		outTargetValue.cast<int16,  int32 >();	break;
				case BaseCastType::SINT_16_TO_64:		outTargetValue.cast<int16,  int64 >();	break;
				case BaseCastType::SINT_32_TO_64:		outTargetValue.cast<int32,  int64 >();	break;
				case BaseCastType::UINT_8_TO_FLOAT:		outTargetValue.cast<uint8,  float >();	break;
				case BaseCastType::UINT_16_TO_FLOAT:	outTargetValue.cast<uint16, float >();	break;
				case BaseCastType::UINT_32_TO_FLOAT:	outTargetValue.cast<uint32, float >();	break;
				case BaseCastType::UINT_64_TO_FLOAT:	outTargetValue.cast<uint64, float >();	break;
				case BaseCastType::SINT_8_TO_FLOAT:		outTargetValue.cast<int8,   float >();	break;
				case BaseCastType::SINT_16_TO_FLOAT:	outTargetValue.cast<int16,  float >();	break;
				case BaseCastType::SINT_32_TO_FLOAT:	outTargetValue.cast<int32,  float >();	break;
				case BaseCastType::SINT_64_TO_FLOAT:	outTargetValue.cast<int64,  float >();	break;
				case BaseCastType::UINT_8_TO_DOUBLE:	outTargetValue.cast<uint8,  double>();	break;
				case BaseCastType::UINT_16_TO_DOUBLE:	outTargetValue.cast<uint16, double>();	break;
				case BaseCastType::UINT_32_TO_DOUBLE:	outTargetValue.cast<uint32, double>();	break;
				case BaseCastType::UINT_64_TO_DOUBLE:	outTargetValue.cast<uint64, double>();	break;
				case BaseCastType::SINT_8_TO_DOUBLE:	outTargetValue.cast<int8,   double>();	break;
				case BaseCastType::SINT_16_TO_DOUBLE:	outTargetValue.cast<int16,  double>();	break;
				case BaseCastType::SINT_32_TO_DOUBLE:	outTargetValue.cast<int32,  double>();	break;
				case BaseCastType::SINT_64_TO_DOUBLE:	outTargetValue.cast<int64,  double>();	break;
				case BaseCastType::FLOAT_TO_UINT_8:		outTargetValue.cast<float,  uint8 >();	break;
				case BaseCastType::FLOAT_TO_UINT_16:	outTargetValue.cast<float,  uint16>();	break;
				case BaseCastType::FLOAT_TO_UINT_32:	outTargetValue.cast<float,  uint32>();	break;
				case BaseCastType::FLOAT_TO_UINT_64:	outTargetValue.cast<float,  uint64>();	break;
				case BaseCastType::FLOAT_TO_SINT_8:		outTargetValue.cast<float,  int8  >();	break;
				case BaseCastType::FLOAT_TO_SINT_16:	outTargetValue.cast<float,  int16 >();	break;
				case BaseCastType::FLOAT_TO_SINT_32:	outTargetValue.cast<float,  int32 >();	break;
				case BaseCastType::FLOAT_TO_SINT_64:	outTargetValue.cast<float,  int64 >();	break;
				case BaseCastType::DOUBLE_TO_UINT_8:	outTargetValue.cast<double, uint8 >();	break;
				case BaseCastType::DOUBLE_TO_UINT_16:	outTargetValue.cast<double, uint16>();	break;
				case BaseCastType::DOUBLE_TO_UINT_32:	outTargetValue.cast<double, uint32>();	break;
				case BaseCastType::DOUBLE_TO_UINT_64:	outTargetValue.cast<double, uint64>();	break;
				case BaseCastType::DOUBLE_TO_SINT_8:	outTargetValue.cast<double, int8  >();	break;
				case BaseCastType::DOUBLE_TO_SINT_16:	outTargetValue.cast<double, int16 >();	break;
				case BaseCastType::DOUBLE_TO_SINT_32:	outTargetValue.cast<double, int32 >();	break;
				case BaseCastType::DOUBLE_TO_SINT_64:	outTargetValue.cast<double, int64 >();	break;
				case BaseCastType::FLOAT_TO_DOUBLE:		outTargetValue.cast<float,  double>();	break;
				case BaseCastType::DOUBLE_TO_FLOAT:		outTargetValue.cast<double, float >();	break;
			}
		}
		return castHandling;
	}

	bool TypeCasting::canMatchSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target, size_t* outFailedIndex) const
	{
		if (original.size() != target.size())
			return false;

		for (size_t i = 0; i < original.size(); ++i)
		{
			if (!canImplicitlyCastTypes(*original[i], *target[i].mDataType))
			{
				if (nullptr != outFailedIndex)
					*outFailedIndex = i;
				return false;
			}
		}
		return true;
	}

	uint16 TypeCasting::getPriorityOfSignature(const BinaryOperatorSignature& signature, const DataTypeDefinition* left, const DataTypeDefinition* right) const
	{
		const uint8 prioLeft = getImplicitCastPriority(left, signature.mLeft);
		const uint8 prioRight = getImplicitCastPriority(right, signature.mRight);
		if (prioLeft < prioRight)
		{
			return ((uint16)prioRight << 8) + (uint16)prioLeft;
		}
		else
		{
			return ((uint16)prioLeft << 8) + (uint16)prioRight;
		}
	}

	uint32 TypeCasting::getPriorityOfSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target) const
	{
		if (original.size() != target.size())
			return 0xffffffff;

		const size_t size = original.size();
		static std::vector<uint8> priorities;	// Not multi-threading safe
		priorities.resize(size);

		for (size_t i = 0; i < size; ++i)
		{
			priorities[i] = getImplicitCastPriority(original[i], target[i].mDataType);
		}

		// Highest priority should be first
		std::sort(priorities.begin(), priorities.end(), std::greater<uint8>());

		uint32 result = 0;
		for (size_t i = 0; i < std::min<size_t>(size, 4); ++i)
		{
			result |= (priorities[i] << (24 - i * 8));
		}
		return result;
	}

	std::optional<size_t> TypeCasting::getBestOperatorSignature(const std::vector<BinaryOperatorSignature>& signatures, bool exactMatchLeftRequired, const DataTypeDefinition* left, const DataTypeDefinition* right) const
	{
		std::optional<size_t> bestIndex;
		uint16 bestPriority = 0xff00;
		for (size_t index = 0; index < signatures.size(); ++index)
		{
			const BinaryOperatorSignature& signature = signatures[index];
			if (exactMatchLeftRequired)
			{
				if (signature.mLeft != left)
					continue;
			}

			const uint16 priority = getPriorityOfSignature(signature, left, right);
			if (priority < bestPriority)
			{
				bestIndex = index;
				bestPriority = priority;
			}
		}
		return bestIndex;
	}

	uint8 TypeCasting::getImplicitCastPriority(const DataTypeDefinition* original, const DataTypeDefinition* target) const
	{
		const CastHandling handling = getCastHandling(original, target, false);
		return (handling.mResult != CastHandling::Result::INVALID) ? handling.mCastPriority : CANNOT_CAST;
	}

}
