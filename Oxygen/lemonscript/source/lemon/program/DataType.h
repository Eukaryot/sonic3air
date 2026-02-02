/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/BaseType.h"
#include "lemon/program/function/FunctionReference.h"
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
			ARRAY,
			CUSTOM
		};

		struct BracketOperator
		{
			const Function* mGetter = 0;		// Function signature: (uint32 variableID, parameterType parameter) -> valueType
			const Function* mSetter = 0;		// Function signature: (valueType value, uint32 variableID, parameterType parameter) -> void
			const DataTypeDefinition* mValueType = nullptr;
			const DataTypeDefinition* mParameterType = nullptr;
		};

	public:
		DataTypeDefinition(std::string_view name, uint16 id, Class class_, size_t bytes, BaseType baseType);
		virtual ~DataTypeDefinition() {}

		template<typename T> bool isA() const		{ return getClass() == T::CLASS; }
		template<typename T> const T& as() const	{ return static_cast<const T&>(*this); }
		template<typename T> const T* cast() const	{ return isA<T>() ? static_cast<const T*>(*this) : nullptr; }

		FlyweightString getName() const;
		inline uint16 getID() const			 { return mID; }
		inline size_t getBytes() const		 { return mBytes; }
		inline size_t getSizeOnStack() const { return (mBytes + 7) / 8; }
		inline Class getClass() const		 { return mClass; }
		inline BaseType getBaseType() const	 { return mBaseType; }

		inline bool isVoid() const			 { return mClass == Class::VOID; }
		inline bool isPredefined() const	 { return mClass <= Class::STRING; }

		virtual uint16 getDataTypeHash() const  { return mID; }

		inline BracketOperator& getBracketOperator()			  { return mBracketOperator; }
		inline const BracketOperator& getBracketOperator() const  { return mBracketOperator; }

		const std::vector<FunctionReference>& getMethodsByName(uint64 methodNameHash) const;
		void addMethod(uint64 nameHash, Function& func);

	protected:
		BracketOperator mBracketOperator;
		std::unordered_map<uint64, std::vector<FunctionReference>> mMethodsByName;		// Key is just the hashed function name (without context name)

	private:
		std::string_view mNameString;
		mutable FlyweightString mName;

		uint16 mID = 0;
		const size_t mBytes = 0;
		const Class mClass = Class::VOID;
		const BaseType mBaseType = BaseType::VOID;	// If compatible to a base type (from the runtime's point of view), set this to something different than VOID
	};


	struct VoidDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::VOID;

	public:
		VoidDataType();
	};


	struct AnyDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::ANY;

	public:
		AnyDataType();
	};


	struct IntegerDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::INTEGER;

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
		IntegerDataType(const char* name, uint16 id, size_t bytes, Semantics semantics, bool isSigned, BaseType baseType);
	};


	struct FloatDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::FLOAT;

	public:
		FloatDataType(const char* name, uint16 id, size_t bytes);
	};


	struct StringDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::STRING;

	public:
		explicit StringDataType(uint16 id);

		// Rather unfortunately, the data type hash for string needs to be the same as for u64, for feature level 1 compatibility regarding function overloading
		uint16 getDataTypeHash() const override;
	};


	struct ArrayDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::ARRAY;

	public:
		ArrayDataType(uint16 id, const DataTypeDefinition& elementType, size_t arraySize);

		static FlyweightString buildArrayDataTypeName(const DataTypeDefinition& elementType, size_t arraySize);

		const DataTypeDefinition& mElementType;
		size_t mArraySize;
	};

	struct ArrayBaseWrapper { uint32 mVariableID = 0; };	// TODO: Move this somewhere else


	struct CustomDataType : public DataTypeDefinition
	{
	public:
		static const Class CLASS = Class::CUSTOM;

	public:
		explicit CustomDataType(const char* name, uint16 id, BaseType baseType);
	};


	struct PredefinedDataTypes
	{
	public:
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

		inline static const CustomDataType ARRAY_BASE = CustomDataType("$array_base", 14, BaseType::INT_32);

	public:
		static const DataTypeDefinition* getDataTypeDefinitionForBaseType(BaseType baseType);
		static void collectPredefinedDataTypes(std::vector<const DataTypeDefinition*>& outDataTypes);
	};

}
