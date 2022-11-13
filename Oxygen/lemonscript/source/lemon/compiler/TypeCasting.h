/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Function.h"
#include "lemon/compiler/Operators.h"


namespace lemon
{
	struct CompileOptions;
	struct DataTypeDefinition;

	class TypeCasting
	{
	public:
		static const constexpr uint8 CANNOT_CAST = 0xff;

		struct CastHandling
		{
			enum class Result
			{
				NO_CAST,	// No cast needed
				INVALID,	// Cast not possible
				BASE_CAST,	// Cast between base types
				ANY_CAST,	// Cast from source type to "any"
			};

			Result mResult = Result::INVALID;
			BaseCastType mBaseCastType = BaseCastType::INVALID;
			uint8 mCastPriority = 0xff;

			inline CastHandling() {}
			inline CastHandling(Result result, uint8 castPriority) : mResult(result), mCastPriority(castPriority) {}
			inline CastHandling(BaseCastType baseCastType, uint8 castPriority) : mResult(Result::BASE_CAST), mBaseCastType(baseCastType), mCastPriority(castPriority) {}
		};

		struct BinaryOperatorSignature
		{
			const DataTypeDefinition* mLeft;
			const DataTypeDefinition* mRight;
			const DataTypeDefinition* mResult;
			inline BinaryOperatorSignature(const DataTypeDefinition* left, const DataTypeDefinition* right, const DataTypeDefinition* result) : mLeft(left), mRight(right), mResult(result) {}
		};

	public:
		inline explicit TypeCasting(const CompileOptions& compileOptions) : mCompileOptions(compileOptions) {}

		bool canImplicitlyCastTypes(const DataTypeDefinition& original, const DataTypeDefinition& target) const;
		bool canExplicitlyCastTypes(const DataTypeDefinition& original, const DataTypeDefinition& target) const;
		CastHandling getCastHandling(const DataTypeDefinition* original, const DataTypeDefinition* target) const;

		bool canMatchSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target, size_t* outFailedIndex = nullptr) const;
		uint16 getPriorityOfSignature(const BinaryOperatorSignature& signature, const DataTypeDefinition* left, const DataTypeDefinition* right) const;
		uint32 getPriorityOfSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target) const;
		const BinaryOperatorSignature* getBestOperatorSignature(Operator op, const DataTypeDefinition* left, const DataTypeDefinition* right) const;

	private:
		uint8 getImplicitCastPriority(const DataTypeDefinition* original, const DataTypeDefinition* target) const;

	private:
		const CompileOptions& mCompileOptions;
	};
}