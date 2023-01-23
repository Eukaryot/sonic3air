/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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

	uint32 FloatDataType::getDataTypeHash() const
	{
		static const constexpr uint32 baseHash = ((uint32)Class::FLOAT) << 24;
		return baseHash + (uint32)getBytes();
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
			case BaseType::FLOAT:		return 4;
			case BaseType::DOUBLE:		return 8;
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
			case BaseType::FLOAT:		return &PredefinedDataTypes::FLOAT;
			case BaseType::DOUBLE:		return &PredefinedDataTypes::DOUBLE;
			default:					return nullptr;
		}
	}

	bool DataTypeHelper::isPureIntegerBaseCast(BaseCastType baseCastType)
	{
		const uint8 value = (uint8)baseCastType;
		return (value > 0 && value < 0x20);
	}


	const DataTypeDefinition* DataTypeSerializer::getDataTypeFromSerializedId(uint8 dataTypeId)
	{
		// TODO: We definitely need a more sophisticated serialization to support more data types
		if (dataTypeId == 0x41)			// Some dumb placeholder magic number for any
		{
			return &PredefinedDataTypes::ANY;
		}
		else if (dataTypeId == 0x40)	// Some dumb placeholder magic number for string
		{
			return &PredefinedDataTypes::STRING;
		}
		else
		{
			const BaseType baseType = (BaseType)dataTypeId;
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
				case BaseType::FLOAT:		return &PredefinedDataTypes::FLOAT;
				case BaseType::DOUBLE:		return &PredefinedDataTypes::DOUBLE;
				default:					return &PredefinedDataTypes::VOID;
			}
		}
	}

	uint8 DataTypeSerializer::getSerializedIdForDataType(const DataTypeDefinition* const dataTypeDefinition)
	{
		// TODO: We definitely need a more sophisticated serialization to support more data types
		if (nullptr == dataTypeDefinition)
		{
			return 0;
		}
		else if (dataTypeDefinition == &PredefinedDataTypes::ANY)
		{
			return 0x41;	// The same dumb magic number as above
		}
		else if (dataTypeDefinition == &PredefinedDataTypes::STRING)
		{
			return 0x40;	// The same dumb magic number as above
		}
		else if (dataTypeDefinition->getClass() == DataTypeDefinition::Class::INTEGER && dataTypeDefinition->as<IntegerDataType>().mSemantics == IntegerDataType::Semantics::BOOLEAN)
		{
			return (uint8)BaseType::BOOL;
		}
		else
		{
			return (uint8)dataTypeDefinition->getBaseType();
		}
	}

	const DataTypeDefinition* DataTypeSerializer::readDataType(VectorBinarySerializer& serializer)
	{
		return getDataTypeFromSerializedId(serializer.read<uint8>());
	}

	void DataTypeSerializer::writeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition* const dataTypeDefinition)
	{
		serializer.write<uint8>(getSerializedIdForDataType(dataTypeDefinition));
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
