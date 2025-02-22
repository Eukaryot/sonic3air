/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
			CUSTOM
		};

		struct BracketOperator
		{
			uint64 mGetterNameAndSignatureHash = 0;		// Function signature: (uint32 variableID, parameterType parameter) -> valueType
			uint64 mSetterNameAndSignatureHash = 0;		// Function signature: (uint32 variableID, parameterType parameter, valueType value) -> void
			const DataTypeDefinition* mValueType = nullptr;
			const DataTypeDefinition* mParameterType = nullptr;
		};

	public:
		inline DataTypeDefinition(const char* name, uint16 id, Class class_, size_t bytes, BaseType baseType) :
			mNameString(name),
			mID(id),
			mClass(class_),
			mBytes(bytes),
			mBaseType(baseType)
		{}
		virtual ~DataTypeDefinition() {}

		template<typename T> const T& as() const  { return static_cast<const T&>(*this); }

		FlyweightString getName() const;
		inline uint16 getID() const			 { return mID; }
		inline size_t getBytes() const		 { return mBytes; }
		inline size_t getSizeOnStack() const { return (mBytes + 7) / 8; }
		inline Class getClass() const		 { return mClass; }
		inline BaseType getBaseType() const	 { return mBaseType; }
		inline bool isPredefined() const	 { return mClass > Class::STRING; }

		virtual uint16 getDataTypeHash() const  { return mID; }

		inline const BracketOperator& getBracketOperator() const	{ return mBracketOperator; }

	protected:
		BracketOperator mBracketOperator;

	private:
		const char* mNameString;
		mutable FlyweightString mName;

		uint16 mID = 0;
		const size_t mBytes = 0;
		const Class mClass = Class::VOID;
		const BaseType mBaseType = BaseType::VOID;	// If compatible to a base type (from the runtime's point of view), set this to something different than VOID
	};


	struct VoidDataType : public DataTypeDefinition
	{
	public:
		inline VoidDataType() :
			DataTypeDefinition("void", 0, Class::VOID, 0, BaseType::VOID)
		{}
	};


	struct AnyDataType : public DataTypeDefinition
	{
	public:
		inline AnyDataType() :
			DataTypeDefinition("any", 1, Class::ANY, 16, BaseType::UINT_64)
		{}
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
		inline IntegerDataType(const char* name, uint16 id, size_t bytes, Semantics semantics, bool isSigned, BaseType baseType) :
			DataTypeDefinition(name, id, Class::INTEGER, bytes, baseType),
			mSemantics(semantics),
			mSizeBits((bytes == 1) ? 0 : (bytes == 2) ? 1 : (bytes == 4) ? 2 : 3),
			mIsSigned(isSigned)
		{}
	};


	struct FloatDataType : public DataTypeDefinition
	{
	public:
		inline FloatDataType(const char* name, uint16 id, size_t bytes) :
			DataTypeDefinition(name, id, Class::FLOAT, bytes, (bytes == 4) ? BaseType::FLOAT : BaseType::DOUBLE)
		{}
	};


	struct StringDataType : public DataTypeDefinition
	{
	public:
		inline explicit StringDataType(uint16 id) :
			DataTypeDefinition("string", id, Class::STRING, 8, BaseType::UINT_64)
		{}

		// Rather unfortunately, the data type hash for string needs to be the same as for u64, for feature level 1 compatibility regarding function overloading
		uint16 getDataTypeHash() const override;
	};


	struct CustomDataType : public DataTypeDefinition
	{
	public:
		explicit CustomDataType(const char* name, uint16 id, BaseType baseType);
	};


	struct PredefinedDataTypes
	{
		inline static const VoidDataType VOID		  = VoidDataType();
		inline static const AnyDataType ANY			  = AnyDataType();

		inline static const IntegerDataType BOOL	  = IntegerDataType("bool", 2, 1, IntegerDataType::Semantics::BOOLEAN, false, BaseType::UINT_8);	// Using the same ID as u8, to not break overriding from before introduction of bool
		inline static const IntegerDataType UINT_8	  = IntegerDataType("u8",   2, 1, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_8);
		inline static const IntegerDataType UINT_16	  = IntegerDataType("u16",  3, 2, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_16);
		inline static const IntegerDataType UINT_32	  = IntegerDataType("u32",  4, 4, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_32);
		inline static const IntegerDataType UINT_64	  = IntegerDataType("u64",  5, 8, IntegerDataType::Semantics::DEFAULT, false, BaseType::UINT_64);
		inline static const IntegerDataType INT_8	  = IntegerDataType("s8",   6, 1, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_8);
		inline static const IntegerDataType INT_16	  = IntegerDataType("s16",  7, 2, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_16);
		inline static const IntegerDataType INT_32	  = IntegerDataType("s32",  8, 4, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_32);
		inline static const IntegerDataType INT_64	  = IntegerDataType("s64",  9, 8, IntegerDataType::Semantics::DEFAULT, true,  BaseType::INT_64);
		inline static const IntegerDataType CONST_INT = IntegerDataType("const_int", 10, 8, IntegerDataType::Semantics::CONSTANT, true, BaseType::INT_CONST);

		inline static const FloatDataType FLOAT		  = FloatDataType("float", 11, 4);
		inline static const FloatDataType DOUBLE	  = FloatDataType("double", 12, 8);

		inline static const StringDataType STRING	  = StringDataType(13);

		static void collectPredefinedDataTypes(std::vector<const DataTypeDefinition*>& outDataTypes);
	};


	struct DataTypeHelper
	{
		static size_t getSizeOfBaseType(BaseType baseType);
		static const DataTypeDefinition* getDataTypeDefinitionForBaseType(BaseType baseType);

		static bool isPureIntegerBaseCast(BaseCastType baseCastType);
	};

}
