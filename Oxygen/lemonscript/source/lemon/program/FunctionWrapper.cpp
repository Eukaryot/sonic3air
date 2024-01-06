/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/FunctionWrapper.h"
#include "lemon/program/Program.h"


namespace lemon
{
	namespace traits
	{
		template<> const DataTypeDefinition* getDataType<void>()			{ return &PredefinedDataTypes::VOID; }
		template<> const DataTypeDefinition* getDataType<bool>()			{ return &PredefinedDataTypes::BOOL; }
		template<> const DataTypeDefinition* getDataType<int8>()			{ return &PredefinedDataTypes::INT_8; }
		template<> const DataTypeDefinition* getDataType<uint8>()			{ return &PredefinedDataTypes::UINT_8; }
		template<> const DataTypeDefinition* getDataType<int16>()			{ return &PredefinedDataTypes::INT_16; }
		template<> const DataTypeDefinition* getDataType<uint16>()			{ return &PredefinedDataTypes::UINT_16; }
		template<> const DataTypeDefinition* getDataType<int32>()			{ return &PredefinedDataTypes::INT_32; }
		template<> const DataTypeDefinition* getDataType<uint32>()			{ return &PredefinedDataTypes::UINT_32; }
		template<> const DataTypeDefinition* getDataType<int64>()			{ return &PredefinedDataTypes::INT_64; }
		template<> const DataTypeDefinition* getDataType<uint64>()			{ return &PredefinedDataTypes::UINT_64; }
		template<> const DataTypeDefinition* getDataType<float>()			{ return &PredefinedDataTypes::FLOAT; }
		template<> const DataTypeDefinition* getDataType<double>()			{ return &PredefinedDataTypes::DOUBLE; }
		template<> const DataTypeDefinition* getDataType<StringRef>()		{ return &PredefinedDataTypes::STRING; }
		template<> const DataTypeDefinition* getDataType<AnyTypeWrapper>()	{ return &PredefinedDataTypes::ANY; }
	}

	namespace internal
	{

		template<>
		void pushStackGeneric(StringRef value, const NativeFunction::Context context)
		{
			context.mControlFlow.pushValueStack<uint64>(value.getHash());
		};

		template<>
		StringRef popStackGeneric(const NativeFunction::Context context)
		{
			const uint64 stringHash = context.mControlFlow.popValueStack<uint64>();
			const FlyweightString* str = context.mControlFlow.getRuntime().resolveStringByKey(stringHash);
			return (nullptr != str) ? StringRef(*str) : StringRef();
		}

		template<>
		void pushStackGeneric(AnyTypeWrapper value, const NativeFunction::Context context)
		{
			context.mControlFlow.pushValueStack(value.mValue);
			context.mControlFlow.pushValueStack(value.mType->getID());
		};

		template<>
		AnyTypeWrapper popStackGeneric(const NativeFunction::Context context)
		{
			AnyTypeWrapper result;
			const uint16 dataTypeId = context.mControlFlow.popValueStack<uint16>();
			result.mType = context.mControlFlow.getProgram().getDataTypeByID(dataTypeId);
			result.mValue = context.mControlFlow.popValueStack<AnyBaseValue>();
			return result;
		}
	}
}
