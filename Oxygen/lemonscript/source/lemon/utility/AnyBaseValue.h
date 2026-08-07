/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{

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

		template<typename S, typename T> void cast() { set<T>(static_cast<T>(get<S>())); }

	private:
		union
		{
			uint64 mUint64 = 0;
			float  mFloat;
			double mDouble;
		};
	};

	template<> FORCE_INLINE int8   AnyBaseValue::get() const		{ return (int8)mUint64; }
	template<> FORCE_INLINE uint8  AnyBaseValue::get() const		{ return (uint8)mUint64; }
	template<> FORCE_INLINE int16  AnyBaseValue::get() const		{ return (int16)mUint64; }
	template<> FORCE_INLINE uint16 AnyBaseValue::get() const		{ return (uint16)mUint64; }
	template<> FORCE_INLINE int32  AnyBaseValue::get() const		{ return (int32)mUint64; }
	template<> FORCE_INLINE uint32 AnyBaseValue::get() const		{ return (uint32)mUint64; }
	template<> FORCE_INLINE int64  AnyBaseValue::get() const		{ return mUint64; }
	template<> FORCE_INLINE uint64 AnyBaseValue::get() const		{ return mUint64; }
	template<> FORCE_INLINE bool   AnyBaseValue::get() const		{ return (bool)mUint64; }
	template<> FORCE_INLINE float  AnyBaseValue::get() const		{ return mFloat; }
	template<> FORCE_INLINE double AnyBaseValue::get() const		{ return mDouble; }
	template<> FORCE_INLINE AnyBaseValue AnyBaseValue::get() const  { return *this; }

	template<> FORCE_INLINE void AnyBaseValue::set(int8 value)			{ mUint64 = (int64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint8 value)			{ mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(int16 value)			{ mUint64 = (int64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint16 value)		{ mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(int32 value)			{ mUint64 = (int64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint32 value)		{ mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(int64 value)			{ mUint64 = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(uint64 value)		{ mUint64 = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(bool value)			{ mUint64 = (uint64)value; }
	template<> FORCE_INLINE void AnyBaseValue::set(float value)			{ mFloat  = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(double value)		{ mDouble = value; }
	template<> FORCE_INLINE void AnyBaseValue::set(AnyBaseValue value)  { *this = value; }


	struct BaseTypeConversion
	{
		template<typename S, typename T>
		static FORCE_INLINE T convert(S value)  { return AnyBaseValue(value).get<T>(); }
	};

	template<> FORCE_INLINE int8   BaseTypeConversion::convert(int8 value)		{ return value; }
	template<> FORCE_INLINE uint8  BaseTypeConversion::convert(uint8 value)		{ return value; }
	template<> FORCE_INLINE int16  BaseTypeConversion::convert(int16 value)		{ return value; }
	template<> FORCE_INLINE uint16 BaseTypeConversion::convert(uint16 value)	{ return value; }
	template<> FORCE_INLINE int32  BaseTypeConversion::convert(int32 value)		{ return value; }
	template<> FORCE_INLINE uint32 BaseTypeConversion::convert(uint32 value)	{ return value; }
	template<> FORCE_INLINE int64  BaseTypeConversion::convert(int64 value)		{ return value; }
	template<> FORCE_INLINE uint64 BaseTypeConversion::convert(uint64 value)	{ return value; }
	template<> FORCE_INLINE float  BaseTypeConversion::convert(float value)		{ return value; }
	template<> FORCE_INLINE double BaseTypeConversion::convert(double value)	{ return value; }
	template<> FORCE_INLINE AnyBaseValue BaseTypeConversion::convert(AnyBaseValue value)	{ return value; }

}
