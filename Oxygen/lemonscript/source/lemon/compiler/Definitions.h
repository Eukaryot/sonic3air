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
		inline AnyBaseValue() {}
		inline AnyBaseValue(uint64 value) : mValue(value) {}

		template<typename T> T get() const  { return T::INVALID; }
		
		void reset()  { mValue = 0; }

		void operator=(int64 value)   { mValue = value; }
		void operator=(uint64 value)  { mValue = value; }
		void operator=(bool value)    { mValue = (uint64)value; }
		void operator=(float value)   { mValue = (uint64)*reinterpret_cast<uint32*>(&value); }	// This only works on little endian machines, but that's probably the case for a lot of lemonscript code...
		void operator=(double value)  { mValue = *reinterpret_cast<uint64*>(&value); }

	private:
		uint64 mValue;
	};

	template<> inline int64  AnyBaseValue::get() const  { return mValue; }
	template<> inline uint64 AnyBaseValue::get() const  { return mValue; }
	template<> inline bool   AnyBaseValue::get() const  { return (bool)mValue; }
	template<> inline float  AnyBaseValue::get() const  { return *reinterpret_cast<float*>(mValue); }
	template<> inline double AnyBaseValue::get() const  { return *reinterpret_cast<double*>(mValue); }

}
