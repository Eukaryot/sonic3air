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
	struct DataTypeDefinition;
	struct GlobalCompilerConfig;

	class TypeCasting
	{
	public:
		static const constexpr uint8 CANNOT_CAST = 0xff;

		struct BinaryOperatorSignature
		{
			const DataTypeDefinition* mLeft;
			const DataTypeDefinition* mRight;
			const DataTypeDefinition* mResult;
			inline BinaryOperatorSignature(const DataTypeDefinition* left, const DataTypeDefinition* right, const DataTypeDefinition* result) : mLeft(left), mRight(right), mResult(result) {}
		};

	public:
		inline explicit TypeCasting(const GlobalCompilerConfig& config) : mConfig(config) {}

		uint8 getImplicitCastPriority(const DataTypeDefinition* original, const DataTypeDefinition* target);
		BaseCastType getBaseCastType(const DataTypeDefinition* original, const DataTypeDefinition* target);

		bool canMatchSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target, size_t* outFailedIndex = nullptr);
		uint16 getPriorityOfSignature(const BinaryOperatorSignature& signature, const DataTypeDefinition* left, const DataTypeDefinition* right);
		uint32 getPriorityOfSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target);
		bool getBestSignature(Operator op, const DataTypeDefinition* left, const DataTypeDefinition* right, const BinaryOperatorSignature** outSignature);

	private:
		const GlobalCompilerConfig& mConfig;
	};
}