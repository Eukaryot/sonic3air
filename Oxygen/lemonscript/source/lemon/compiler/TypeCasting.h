/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Function.h"
#include "lemon/compiler/Operators.h"

struct DataTypeDefinition;


namespace lemon
{
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
		static uint8 getImplicitCastPriority(const DataTypeDefinition* original, const DataTypeDefinition* target);
		static int getCastType(const DataTypeDefinition* original, const DataTypeDefinition* target);

		static uint16 getPriorityOfSignature(const BinaryOperatorSignature& signature, const DataTypeDefinition* left, const DataTypeDefinition* right);
		static uint32 getPriorityOfSignature(const std::vector<const DataTypeDefinition*>& original, const Function::ParameterList& target);
		static bool getBestSignature(Operator op, const DataTypeDefinition* left, const DataTypeDefinition* right, const BinaryOperatorSignature** outSignature);
	};
}