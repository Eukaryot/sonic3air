/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/DataType.h"


namespace lemon
{

	inline DataTypeDefinition::DataTypeDefinition(std::string_view name, uint16 id, Class class_, size_t bytes, BaseType baseType) :
		mNameString(name),
		mID(id),
		mClass(class_),
		mBytes(bytes),
		mBaseType(baseType)
	{}

	FlyweightString DataTypeDefinition::getName() const
	{
		if (!mName.isValid())
			mName.set(mNameString);
		return mName;
	}

	const std::vector<FunctionReference>& DataTypeDefinition::getMethodsByName(uint64 methodNameHash) const
	{
		static const std::vector<FunctionReference> EMPTY_FUNCTIONS;
		const auto it = mMethodsByName.find(methodNameHash);
		return (it == mMethodsByName.end()) ? EMPTY_FUNCTIONS : it->second;
	}

	void DataTypeDefinition::addMethod(uint64 nameHash, Function& func)
	{
		FunctionReference& ref = vectorAdd(mMethodsByName[nameHash]);
		ref.mFunction = &func;
		ref.mIsDeprecated = false;
	}


	VoidDataType::VoidDataType() :
		DataTypeDefinition("void", 0, Class::VOID, 0, BaseType::VOID)
	{}


	AnyDataType::AnyDataType() :
		DataTypeDefinition("any", 1, Class::ANY, 16, BaseType::UINT_64)
	{}


	IntegerDataType::IntegerDataType(const char* name, uint16 id, size_t bytes, Semantics semantics, bool isSigned, BaseType baseType) :
		DataTypeDefinition(name, id, Class::INTEGER, bytes, baseType),
		mSemantics(semantics),
		mSizeBits((bytes == 1) ? 0 : (bytes == 2) ? 1 : (bytes == 4) ? 2 : 3),
		mIsSigned(isSigned)
	{}


	FloatDataType::FloatDataType(const char* name, uint16 id, size_t bytes) :
		DataTypeDefinition(name, id, Class::FLOAT, bytes, (bytes == 4) ? BaseType::FLOAT : BaseType::DOUBLE)
	{}


	StringDataType::StringDataType(uint16 id) :
		DataTypeDefinition("string", id, Class::STRING, 8, BaseType::UINT_64)
	{
		mBracketOperator.mGetter = nullptr;	// This gets filled in later
		mBracketOperator.mSetter = nullptr;
		mBracketOperator.mParameterType = &PredefinedDataTypes::INT_32;
		mBracketOperator.mValueType = &PredefinedDataTypes::INT_32;
	}

	uint16 StringDataType::getDataTypeHash() const
	{
		return PredefinedDataTypes::UINT_64.getID();
	}


	ArrayDataType::ArrayDataType(uint16 id, const DataTypeDefinition& elementType, size_t arraySize) :
		DataTypeDefinition(buildArrayDataTypeName(elementType, arraySize).getString(), id, Class::ARRAY, elementType.getBytes() * arraySize, BaseType::UINT_32),
		mElementType(elementType),
		mArraySize(arraySize)
	{}

	FlyweightString ArrayDataType::buildArrayDataTypeName(const DataTypeDefinition& elementType, size_t arraySize)
	{
		const std::string str = std::string(elementType.getName().getString()) + '[' + std::to_string(arraySize) + ']';
		return FlyweightString(str);
	}


	CustomDataType::CustomDataType(const char* name, uint16 id, BaseType baseType) :
		DataTypeDefinition(name, id, Class::CUSTOM, BaseTypeHelper::getSizeOfBaseType(baseType), baseType)
	{}


	const DataTypeDefinition* PredefinedDataTypes::getDataTypeDefinitionForBaseType(BaseType baseType)
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

}
