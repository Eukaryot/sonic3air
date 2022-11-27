/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/BaseType.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{

	struct DataTypeDefinition
	{
	public:
		enum class Class : uint8
		{
			VOID,
			ANY,
			INTEGER,
			FLOAT,
			STRING,
		};

	public:
		inline DataTypeDefinition(const char* name, Class class_, size_t bytes, BaseType baseType) :
			mNameString(name),
			mClass(class_),
			mBytes(bytes),
			mBaseType(baseType)
		{}
		virtual ~DataTypeDefinition() {}

		template<typename T> const T& as() const  { return static_cast<const T&>(*this); }

		FlyweightString getName() const;
		inline size_t getBytes() const		{ return mBytes; }
		inline Class getClass() const		{ return mClass; }
		inline BaseType getBaseType() const	{ return mBaseType; }

		virtual uint32 getDataTypeHash() const = 0;

	private:
		const char* mNameString;
		mutable FlyweightString mName;

		const size_t mBytes = 0;
		const Class mClass = Class::VOID;
		const BaseType mBaseType = BaseType::VOID;	// If compatible to a base type (from the runtime's point of view), set this to something different than VOID
	};


	struct VoidDataType : public DataTypeDefinition
	{
	public:
		inline VoidDataType() :
			DataTypeDefinition("void", Class::VOID, 0, BaseType::VOID)
		{}

		uint32 getDataTypeHash() const override  { return 0; }
	};


	struct AnyDataType : public DataTypeDefinition
	{
	public:
		inline AnyDataType() :
			DataTypeDefinition("any", Class::ANY, 0, BaseType::UINT_64)
		{}

		uint32 getDataTypeHash() const override  { return 1; }
	};


	struct IntegerDataType : public DataTypeDefinition
	{
	public:
		enum class Semantics
		{
			DEFAULT,
			CONSTANT,
			BOOLEAN
		};

		const Semantics mSemantics = Semantics::DEFAULT;
		const bool mIsSigned = false;
		const uint8 mSizeBits = 0;	// 0 for 8-bit data types, 1 for 16-bit, 2 for 32-bit, 3 for 64-bit

	public:
		inline IntegerDataType(const char* name, size_t bytes, Semantics semantics, bool isSigned, BaseType baseType) :
			DataTypeDefinition(name, Class::INTEGER, bytes, baseType),
			mSemantics(semantics),
			mSizeBits((bytes == 1) ? 0 : (bytes == 2) ? 1 : (bytes == 4) ? 2 : 3),
			mIsSigned(isSigned)
		{}

		uint32 getDataTypeHash() const override;
	};


	struct FloatDataType : public DataTypeDefinition
	{
	public:
		inline FloatDataType(const char* name, size_t bytes) :
			DataTypeDefinition(name, Class::FLOAT, bytes, (bytes == 4) ? BaseType::FLOAT : BaseType::DOUBLE)
		{}

		uint32 getDataTypeHash() const override;
	};


	struct StringDataType : public DataTypeDefinition
	{
	public:
		inline StringDataType() :
			DataTypeDefinition("string", Class::STRING, 8, BaseType::UINT_64)
		{}

		// Rather unfortunately, the data type hash for string needs to be the same as for u64, for feature level 1 compatibility regarding function overloading
		uint32 getDataTypeHash() const override  { return (((uint32)Class::INTEGER) << 24) + 8; }
	};


	struct PredefinedDataTypes
	{
		inline static const VoidDataType VOID		  = VoidDataType();
		inline static const AnyDataType ANY			  = AnyDataType();

		inline static const IntegerDataType UINT_8	  = IntegerDataType("u8",  1, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_8);
		inline static const IntegerDataType UINT_16	  = IntegerDataType("u16", 2, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_16);
		inline static const IntegerDataType UINT_32	  = IntegerDataType("u32", 4, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_32);
		inline static const IntegerDataType UINT_64	  = IntegerDataType("u64", 8, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_64);
		inline static const IntegerDataType INT_8	  = IntegerDataType("s8",  1, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_8);
		inline static const IntegerDataType INT_16	  = IntegerDataType("s16", 2, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_16);
		inline static const IntegerDataType INT_32	  = IntegerDataType("s32", 4, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_32);
		inline static const IntegerDataType INT_64	  = IntegerDataType("s64", 8, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_64);
		inline static const IntegerDataType CONST_INT = IntegerDataType("const_int", 8, IntegerDataType::Semantics::CONSTANT, true, BaseType::INT_CONST);
		//inline static const IntegerDataType BOOL	  = IntegerDataType("bool", 1, IntegerDataType::Semantics::BOOLEAN, false, BaseType::UINT_8);
		inline static const IntegerDataType& BOOL	  = UINT_8;

		inline static const FloatDataType& FLOAT	  = FloatDataType("float", 4);
		inline static const FloatDataType& DOUBLE	  = FloatDataType("double", 8);

		inline static const StringDataType STRING     = StringDataType();
	};


	struct DataTypeHelper
	{
		static size_t getSizeOfBaseType(BaseType baseType);
		static const DataTypeDefinition* getDataTypeDefinitionForBaseType(BaseType baseType);

		static bool isPureIntegerBaseCast(BaseCastType baseCastType);
	};


	struct DataTypeSerializer
	{
		static const DataTypeDefinition* getDataTypeFromSerializedId(uint8 dataTypeId);
		static uint8 getSerializedIdForDataType(const DataTypeDefinition* const dataTypeDefinition);
		static const DataTypeDefinition* readDataType(VectorBinarySerializer& serializer);
		static void writeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition* const dataTypeDefinition);
		static void serializeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition*& dataTypeDefinition);
	};

}
