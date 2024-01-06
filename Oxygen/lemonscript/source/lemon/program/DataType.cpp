/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
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


	uint16 StringDataType::getDataTypeHash() const
	{
		return PredefinedDataTypes::UINT_64.getID();
	}


	CustomDataType::CustomDataType(const char* name, uint16 id, BaseType baseType) :
		DataTypeDefinition(name, id, Class::CUSTOM, DataTypeHelper::getSizeOfBaseType(baseType), baseType)
	{}


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

	void PredefinedDataTypes::collectPredefinedDataTypes(std::vector<const DataTypeDefinition*>& outDataTypes)
	{
		outDataTypes.clear();
		outDataTypes.push_back(&PredefinedDataTypes::VOID);
		outDataTypes.push_back(&PredefinedDataTypes::ANY);
		outDataTypes.push_back(&PredefinedDataTypes::UINT_8);
		outDataTypes.push_back(&PredefinedDataTypes::UINT_16);
		outDataTypes.push_back(&PredefinedDataTypes::UINT_32);
		outDataTypes.push_back(&PredefinedDataTypes::UINT_64);
		outDataTypes.push_back(&PredefinedDataTypes::INT_8);
		outDataTypes.push_back(&PredefinedDataTypes::INT_16);
		outDataTypes.push_back(&PredefinedDataTypes::INT_32);
		outDataTypes.push_back(&PredefinedDataTypes::INT_64);
		outDataTypes.push_back(&PredefinedDataTypes::CONST_INT);
		//outDataTypes.push_back(&PredefinedDataTypes::BOOL);
		outDataTypes.push_back(&PredefinedDataTypes::FLOAT);
		outDataTypes.push_back(&PredefinedDataTypes::DOUBLE);
		outDataTypes.push_back(&PredefinedDataTypes::STRING);

	#ifdef DEBUG
		for (size_t i = 0; i < outDataTypes.size(); ++i)
			RMX_ASSERT(outDataTypes[i]->getID() == (uint16)i, "Discrepancy in data type IDs for predefined data type");
	#endif
	}

	bool DataTypeHelper::isPureIntegerBaseCast(BaseCastType baseCastType)
	{
		const uint8 value = (uint8)baseCastType;
		return (value > 0 && value < 0x20);
	}

}
