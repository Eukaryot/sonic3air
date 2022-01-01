/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/FunctionWrapper.h"


namespace lemon
{
	namespace traits
	{
		template<> const DataTypeDefinition* getDataType<void>()		{ return &PredefinedDataTypes::VOID; }
		template<> const DataTypeDefinition* getDataType<bool>()		{ return &PredefinedDataTypes::INT_8; }
		template<> const DataTypeDefinition* getDataType<int8>()		{ return &PredefinedDataTypes::INT_8; }
		template<> const DataTypeDefinition* getDataType<uint8>()		{ return &PredefinedDataTypes::UINT_8; }
		template<> const DataTypeDefinition* getDataType<int16>()		{ return &PredefinedDataTypes::INT_16; }
		template<> const DataTypeDefinition* getDataType<uint16>()		{ return &PredefinedDataTypes::UINT_16; }
		template<> const DataTypeDefinition* getDataType<int32>()		{ return &PredefinedDataTypes::INT_32; }
		template<> const DataTypeDefinition* getDataType<uint32>()		{ return &PredefinedDataTypes::UINT_32; }
		template<> const DataTypeDefinition* getDataType<int64>()		{ return &PredefinedDataTypes::INT_64; }
		template<> const DataTypeDefinition* getDataType<uint64>()		{ return &PredefinedDataTypes::UINT_64; }
		template<> const DataTypeDefinition* getDataType<StringRef>()	{ return &PredefinedDataTypes::STRING; }
	}

	namespace internal
	{
		template<>
		void handleResult(StringRef result, const UserDefinedFunction::Context context)
		{
			context.mControlFlow.pushValueStack(traits::getDataType<StringRef>(), result.mHash);
		};

		template<>
		StringRef popStackGeneric(const UserDefinedFunction::Context context)
		{
			const uint64 stringHash = context.mControlFlow.popValueStack(traits::getDataType<uint64>());
			return StringRef(stringHash, context.mControlFlow.getRuntime().resolveStringByKey(stringHash));
		}
	}
}
