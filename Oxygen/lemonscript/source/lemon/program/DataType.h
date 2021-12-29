/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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
		enum class Class
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

		virtual const std::string& toString() const;
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

		inline IntegerDataType(size_t bytes, Semantics semantics, bool isSigned) :
			DataTypeDefinition(Class::INTEGER, bytes), mSemantics(semantics), mIsSigned(isSigned)
		{}

		const std::string& toString() const override;
	};

	struct StringDataType : public DataTypeDefinition
	{
		inline StringDataType() :
			DataTypeDefinition(Class::STRING, 8)
		{}

		const std::string& toString() const override;
	};


	struct PredefinedDataTypes
	{
		inline static const DataTypeDefinition VOID   = DataTypeDefinition(DataTypeDefinition::Class::VOID, 0);

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
			else
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
			// TODO: How about some more sophisticated serialization...?
			const BaseType baseType = (BaseType)serializer.read<uint8>();
			return getDefinitionFromBaseType(baseType);
		}

		static inline void writeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition* const dataTypeDefinition)
		{
			// TODO: How about some more sophisticated serialization...?
			const BaseType baseType = getBaseType(dataTypeDefinition);
			serializer.writeAs<uint8>(baseType);
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
