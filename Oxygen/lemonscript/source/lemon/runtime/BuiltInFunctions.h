/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/utility/FlyweightString.h"


namespace lemon
{
	class Module;

	class API_EXPORT BuiltInFunctions
	{
	public:
		struct FunctionName
		{
			std::string mName;
			uint64 mHash = 0;

			inline explicit FunctionName(const std::string& name) : mName(name), mHash(rmx::getMurmur2_64(name)) {}

			inline FlyweightString makeFlyweightString() const  { FlyweightString str; str.set(mHash, mName); return str; }
		};

	public:
		static FunctionName CONSTANT_ARRAY_ACCESS;
		static FunctionName STRING_OPERATOR_PLUS;
		static FunctionName STRING_OPERATOR_LESS;
		static FunctionName STRING_OPERATOR_LESS_OR_EQUAL;
		static FunctionName STRING_OPERATOR_GREATER;
		static FunctionName STRING_OPERATOR_GREATER_OR_EQUAL;
		static FunctionName STRING_LENGTH;

	public:
		static void registerBuiltInFunctions(Module& module);
	};
}
