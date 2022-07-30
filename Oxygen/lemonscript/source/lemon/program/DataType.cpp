/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/DataType.h"


namespace lemon
{

	FlyweightString DataTypeDefinition::getName() const
	{
		if (!mName.isValid())
			mName.set(mNameString);
		return mName;
	}


	uint32 IntegerDataType::getDataTypeHash() const
	{
		static const constexpr uint32 baseHash = ((uint32)Class::INTEGER) << 24;
		switch (mSemantics)
		{
			case IntegerDataType::Semantics::BOOLEAN:	return baseHash + (uint32)BaseType::BOOL;
			case IntegerDataType::Semantics::CONSTANT:	return baseHash + (uint32)BaseType::INT_CONST;
			default:									return baseHash + (uint32)getBytes() + ((uint32)mIsSigned * 0x80);
		}
	}


	size_t DataTypeHelper::getSizeOfBaseType(BaseType baseType)
	{
		switch (baseType)
		{
			case BaseType::UINT_8:		return 1;
			case BaseType::UINT_16:		return 2;
			case BaseType::UINT_32:		return 4;
			case BaseType::UINT_64:		return 8;
			case BaseType::INT_8:		return 1;
			case BaseType::INT_16:		return 2;
			case BaseType::INT_32:		return 4;
			case BaseType::INT_64:		return 8;
			case BaseType::INT_CONST:	return 8;
			default:					return 0;
		}
	}

	const DataTypeDefinition* DataTypeHelper::getDataTypeDefinitionForBaseType(BaseType baseType)
	{
		switch (baseType)
		{
			case BaseType::UINT_8:		return &PredefinedDataTypes::UINT_8;
			case BaseType::UINT_16:		return &PredefinedDataTypes::UINT_16;
			case BaseType::UINT_32:		return &PredefinedDataTypes::UINT_32;
			case BaseType::UINT_64:		return &PredefinedDataTypes::UINT_64;
			case BaseType::INT_8:		return &PredefinedDataTypes::INT_8;
			case BaseType::INT_16:		return &PredefinedDataTypes::INT_16;
			case BaseType::INT_32:		return &PredefinedDataTypes::INT_32;
			case BaseType::INT_64:		return &PredefinedDataTypes::INT_64;
			case BaseType::INT_CONST:	return &PredefinedDataTypes::CONST_INT;
			default:					return nullptr;
		}
	}


	const DataTypeDefinition* DataTypeSerializer::readDataType(VectorBinarySerializer& serializer)
	{
		// TODO: We definitely need a more sophisticated serialization to support more data types
		const BaseType baseType = (BaseType)serializer.read<uint8>();
		if ((uint8)baseType == 0x40)	// Some dumb placeholder magic number for string
		{
			return &PredefinedDataTypes::STRING;
		}
		else
		{
			switch (baseType)
			{
				case BaseType::VOID:		return &PredefinedDataTypes::VOID;
				case BaseType::UINT_8:		return &PredefinedDataTypes::UINT_8;
				case BaseType::UINT_16:		return &PredefinedDataTypes::UINT_16;
				case BaseType::UINT_32:		return &PredefinedDataTypes::UINT_32;
				case BaseType::UINT_64:		return &PredefinedDataTypes::UINT_64;
				case BaseType::INT_8:		return &PredefinedDataTypes::INT_8;
				case BaseType::INT_16:		return &PredefinedDataTypes::INT_16;
				case BaseType::INT_32:		return &PredefinedDataTypes::INT_32;
				case BaseType::INT_64:		return &PredefinedDataTypes::INT_64;
					//case BaseType::BOOL:
				case BaseType::INT_CONST:	return &PredefinedDataTypes::CONST_INT;
			}
			return &PredefinedDataTypes::VOID;
		}
	}

	void DataTypeSerializer::writeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition* const dataTypeDefinition)
	{
		// TODO: We definitely need a more sophisticated serialization to support more data types
		if (nullptr == dataTypeDefinition)
		{
			serializer.write<uint8>(0);
		}
		else if (dataTypeDefinition == &PredefinedDataTypes::STRING)
		{
			serializer.write<uint8>(0x40);	// The same dumb magic number as above
		}
		else if (dataTypeDefinition->getClass() == DataTypeDefinition::Class::INTEGER && dataTypeDefinition->as<IntegerDataType>().mSemantics == IntegerDataType::Semantics::BOOLEAN)
		{
			serializer.writeAs<uint8>(BaseType::BOOL);
		}
		else
		{
			serializer.writeAs<uint8>(dataTypeDefinition->getBaseType());
		}
	}

	void DataTypeSerializer::serializeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition*& dataTypeDefinition)
	{
		if (serializer.isReading())
		{
			dataTypeDefinition = readDataType(serializer);
		}
		else
		{
			writeDataType(serializer, dataTypeDefinition);
		}
	}

}
