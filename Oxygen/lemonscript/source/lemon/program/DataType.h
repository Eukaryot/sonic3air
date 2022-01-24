/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{

	enum class BaseType : uint8
	{
		VOID	  = 0x00,
		UINT_8	  = 0x10 + 0x00,
		UINT_16	  = 0x10 + 0x01,
		UINT_32	  = 0x10 + 0x02,
		UINT_64	  = 0x10 + 0x03,
		INT_8	  = 0x18 + 0x00,
		INT_16	  = 0x18 + 0x01,
		INT_32	  = 0x18 + 0x02,
		INT_64	  = 0x18 + 0x03,
		BOOL	  = UINT_8,
		INT_CONST = 0x1f			// Constants have an undefined int type
	};

	enum class BaseCastType : uint8
	{
		INVALID = 0xff,
		NONE    = 0x00,

		// Cast up (value is unsigned -> adding zeroes)
		UINT_8_TO_16  = 0x01,	// 0x00 * 4 + 0x01
		UINT_8_TO_32  = 0x02,	// 0x00 * 4 + 0x02
		UINT_8_TO_64  = 0x03,	// 0x00 * 4 + 0x03
		UINT_16_TO_32 = 0x06,	// 0x01 * 4 + 0x02
		UINT_16_TO_64 = 0x07,	// 0x01 * 4 + 0x03
		UINT_32_TO_64 = 0x0b,	// 0x02 * 4 + 0x03

		// Cast down (signed or unsigned makes no difference here)
		INT_16_TO_8  = 0x04,	// 0x01 * 4 + 0x00
		INT_32_TO_8  = 0x08,	// 0x02 * 4 + 0x00
		INT_64_TO_8  = 0x0c,	// 0x03 * 4 + 0x00
		INT_32_TO_16 = 0x09,	// 0x02 * 4 + 0x01
		INT_64_TO_16 = 0x0d,	// 0x03 * 4 + 0x01
		INT_64_TO_32 = 0x0e,	// 0x03 * 4 + 0x02

		// Cast up (value is signed -> adding highest bit)
		SINT_8_TO_16  = 0x11,	// 0x10 + 0x00 * 4 + 0x01
		SINT_8_TO_32  = 0x12,	// 0x10 + 0x00 * 4 + 0x02
		SINT_8_TO_64  = 0x13,	// 0x10 + 0x00 * 4 + 0x03
		SINT_16_TO_32 = 0x16,	// 0x10 + 0x01 * 4 + 0x02
		SINT_16_TO_64 = 0x17,	// 0x10 + 0x01 * 4 + 0x03
		SINT_32_TO_64 = 0x1b	// 0x10 + 0x02 * 4 + 0x03
	};



	struct DataTypeDefinition
	{
		enum class Class : uint8
		{
			VOID,
			INTEGER,
			STRING
		};

		Class mClass = Class::VOID;
		size_t mBytes = 0;

		inline DataTypeDefinition(Class class_, size_t bytes) : mClass(class_), mBytes(bytes) {}
		virtual ~DataTypeDefinition() {}

		template<typename T> const T& as() const  { return static_cast<const T&>(*this); }

		virtual uint32 getDataTypeHash() const = 0;
		virtual const std::string& toString() const = 0;
	};

	struct VoidDataType : public DataTypeDefinition
	{
		inline VoidDataType() :
			DataTypeDefinition(Class::VOID, 0)
		{}

		uint32 getDataTypeHash() const override  { return 0; }
		const std::string& toString() const override;
	};

	struct IntegerDataType : public DataTypeDefinition
	{
		enum class Semantics
		{
			DEFAULT,
			CONSTANT,
			BOOLEAN
		};

		Semantics mSemantics = Semantics::DEFAULT;
		bool mIsSigned = false;
		uint8 mSizeBits = 0;	// 0 for 8-bit data types, 1 for 16-bit, 2 for 32-bit, 3 for 64-bit

		inline IntegerDataType(size_t bytes, Semantics semantics, bool isSigned) :
			DataTypeDefinition(Class::INTEGER, bytes),
			mSemantics(semantics),
			mSizeBits((bytes == 1) ? 0 : (bytes == 2) ? 1 : (bytes == 4) ? 2 : 3),
			mIsSigned(isSigned)
		{}

		uint32 getDataTypeHash() const override;
		const std::string& toString() const override;
	};

	struct StringDataType : public DataTypeDefinition
	{
		inline StringDataType() :
			DataTypeDefinition(Class::STRING, 8)
		{}

		// Rather unfortunately, the data type hash for string needs to be the same as for u64, for feature level 1 compatibility regarding function overloading
		uint32 getDataTypeHash() const override  { return 0x01000008; }
		const std::string& toString() const override;
	};


	struct PredefinedDataTypes
	{
		inline static const VoidDataType VOID		  = VoidDataType();

		inline static const IntegerDataType UINT_8	  = IntegerDataType(1, IntegerDataType::Semantics::DEFAULT, false);
		inline static const IntegerDataType UINT_16	  = IntegerDataType(2, IntegerDataType::Semantics::DEFAULT, false);
		inline static const IntegerDataType UINT_32	  = IntegerDataType(4, IntegerDataType::Semantics::DEFAULT, false);
		inline static const IntegerDataType UINT_64	  = IntegerDataType(8, IntegerDataType::Semantics::DEFAULT, false);
		inline static const IntegerDataType INT_8	  = IntegerDataType(1, IntegerDataType::Semantics::DEFAULT, true);
		inline static const IntegerDataType INT_16	  = IntegerDataType(2, IntegerDataType::Semantics::DEFAULT, true);
		inline static const IntegerDataType INT_32	  = IntegerDataType(4, IntegerDataType::Semantics::DEFAULT, true);
		inline static const IntegerDataType INT_64	  = IntegerDataType(8, IntegerDataType::Semantics::DEFAULT, true);
		inline static const IntegerDataType CONST_INT = IntegerDataType(8, IntegerDataType::Semantics::CONSTANT, true);
		//inline static const IntegerDataType BOOL	  = IntegerDataType(1, IntegerDataType::Semantics::BOOLEAN, false);
		inline static const IntegerDataType& BOOL	  = UINT_8;

		inline static const StringDataType STRING     = StringDataType();
	};


	struct DataTypeHelper
	{
		// TODO: Remove these on the long run
		static inline BaseType getBaseType(const DataTypeDefinition* dataType)
		{
			if (nullptr == dataType || dataType->mClass == DataTypeDefinition::Class::VOID)
			{
				return BaseType::VOID;
			}
			else if (dataType->mClass == DataTypeDefinition::Class::INTEGER)
			{
				const IntegerDataType& integerType = dataType->as<IntegerDataType>();
				if (integerType.mSemantics == IntegerDataType::Semantics::BOOLEAN)
				{
					return BaseType::BOOL;
				}
				else if (integerType.mSemantics == IntegerDataType::Semantics::CONSTANT)
				{
					return BaseType::INT_CONST;
				}

				BaseType result = (integerType.mBytes == 1) ? BaseType::UINT_8 : (integerType.mBytes == 2) ? BaseType::UINT_16 : (integerType.mBytes == 4) ? BaseType::UINT_32 : BaseType::UINT_64;
				if (integerType.mIsSigned)
					result = (BaseType)((uint8)result + 0x08);
				return result;
			}
			else if (dataType->mClass == DataTypeDefinition::Class::STRING)
			{
				return BaseType::UINT_64;
			}

			RMX_ASSERT(false, "Unsupported type");
			return BaseType::VOID;
		}

		static const DataTypeDefinition* getDefinitionFromBaseType(BaseType baseType)
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

		static inline const DataTypeDefinition* readDataType(VectorBinarySerializer& serializer)
		{
			// TODO: We definitely need a more sophisticated serialization to support more data types
			const BaseType baseType = (BaseType)serializer.read<uint8>();
			if ((uint8)baseType == 0x40)	// Some dumb placeholder magic number for string
				return &PredefinedDataTypes::STRING;
			else
				return getDefinitionFromBaseType(baseType);
		}

		static inline void writeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition* const dataTypeDefinition)
		{
			// TODO: We definitely need a more sophisticated serialization to support more data types
			if (dataTypeDefinition == &PredefinedDataTypes::STRING)
			{
				serializer.write<uint8>(0x40);	// The same dumb magic number as above
			}
			else
			{
				const BaseType baseType = getBaseType(dataTypeDefinition);
				serializer.writeAs<uint8>(baseType);
			}
		}

		static inline void serializeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition*& dataTypeDefinition)
		{
			if (serializer.isReading())
			{
				dataTypeDefinition = readDataType(serializer);
			}
			else
			{
				const BaseType baseType = getBaseType(dataTypeDefinition);
				serializer.writeAs<uint8>(baseType);
			}
		}
	};

}
