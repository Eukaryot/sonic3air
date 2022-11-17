/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"


namespace lemon
{
	struct DataTypeDefinition;

	struct CompileOptions
	{
		// Options to be set before compilation
		const DataTypeDefinition* mExternalAddressType = &PredefinedDataTypes::UINT_64;
		std::wstring mOutputCombinedSource;
		std::wstring mOutputNativizedSource;
		std::wstring mOutputTranslatedSource;
		bool mConsumeProcessedPragmas = true;

		// Set during compilation
		uint32 mScriptFeatureLevel = 1;
	};

	enum class Keyword : uint8
	{
		_INVALID = 0,
		BLOCK_BEGIN,
		BLOCK_END,
		FUNCTION,
		GLOBAL,
		CONSTANT,
		DEFINE,
		DECLARE,
		RETURN,
		CALL,
		JUMP,
		BREAK,
		CONTINUE,
		IF,
		ELSE,
		WHILE,
		FOR,
		ADDRESSOF
	};

	struct AnyBaseValue
	{
	public:
		inline AnyBaseValue()  {}
		inline explicit AnyBaseValue(int8   value)  { mUint64 = (uint64)value; }
		inline explicit AnyBaseValue(uint8  value)  { mUint64 = (int64)value; }
		inline explicit AnyBaseValue(int16  value)  { mUint64 = (uint64)value; }
		inline explicit AnyBaseValue(uint16 value)  { mUint64 = (int64)value; }
		inline explicit AnyBaseValue(int32  value)  { mUint64 = (uint64)value; }
		inline explicit AnyBaseValue(uint32 value)  { mUint64 = (int64)value; }
		inline explicit AnyBaseValue(int64  value)  { mUint64 = value; }
		inline explicit AnyBaseValue(uint64 value)  { mUint64 = value; }
		inline explicit AnyBaseValue(bool   value)  { mUint64 = (uint64)value; }
		inline explicit AnyBaseValue(float  value)  { mFloat  = value; }
		inline explicit AnyBaseValue(double value)  { mDouble = value; }

		template<typename T> T get() const		{ return T::INVALID; }
		template<typename T> void set(T value)	{ return T::INVALID; }
	
		inline void reset()  { mUint64 = 0; }

	private:
		union
		{
			uint64 mUint64;
			float  mFloat;
			double mDouble;
		};
	};

	template<> FORCE_INLINE int8   AnyBaseValue::get() const  { return (int8)mUint64; }
	template<> FORCE_INLINE uint8  AnyBaseValue::get() const  { return (uint8)mUint64; }
	template<> FORCE_INLINE int16  AnyBaseValue::get() const  { return (int16)mUint64; }
	template<> FORCE_INLINE uint16 AnyBaseValue::get() const  { return (uint16)mUint64; }
	template<> FORCE_INLINE int32  AnyBaseValue::get() const  { return (int32)mUint64; }
	template<> FORCE_INLINE uint32 AnyBaseValue::get() const  { return (uint32)mUint64; }
	template<> FORCE_INLINE int64  AnyBaseValue::get() const  { return mUint64; }
	template<> FORCE_INLINE uint64 AnyBaseValue::get() const  { return mUint64; }
	template<> FORCE_INLINE bool   AnyBaseValue::get() const  { return (bool)mUint64; }
	template<> FORCE_INLINE float  AnyBaseValue::get() const  { return mFloat; }
	template<> FORCE_INLINE double AnyBaseValue::get() const  { return mDouble; }

	template<> FORCE_INLINE void AnyBaseValue::set(int8 value)    { mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint8 value)   { mUint64 = (int64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(int16 value)   { mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint16 value)  { mUint64 = (int64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(int32 value)   { mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint32 value)  { mUint64 = (int64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(int64 value)   { mUint64 = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint64 value)  { mUint64 = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(bool value)    { mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(float value)   { mFloat  = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(double value)  { mDouble = value; }

}
