/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/StandardLibrary.h"
#include "lemon/runtime/BuiltInFunctions.h"
#include "lemon/runtime/FastStringStream.h"
#include "lemon/program/FunctionWrapper.h"
#include "lemon/program/Module.h"
#include "lemon/program/Program.h"


namespace lemon
{
	namespace functions
	{
		template<typename T>
		T minimum(T a, T b)
		{
			return std::min(a, b);
		}

		template<typename T>
		T maximum(T a, T b)
		{
			return std::max(a, b);
		}

		template<typename T>
		T clamp(T a, T b, T c)
		{
			return std::min(std::max(a, b), c);
		}

		template<typename R, typename T>
		R absolute(T a)
		{
			return std::abs(a);
		}

		uint32 sqrt_u32(uint32 a)
		{
			return (uint32)std::sqrt((float)a);
		}

		int16 sin_s16(int16 x)
		{
			return (int16)roundToInt(std::sin((float)x / (float)0x100) * (float)0x100);
		}

		int32 sin_s32(int32 x)
		{
			return (int32)roundToInt(std::sin((float)x / (float)0x10000) * (float)0x10000);
		}

		int16 cos_s16(int16 x)
		{
			return (int16)roundToInt(std::cos((float)x / (float)0x100) * (float)0x100);
		}

		int32 cos_s32(int32 x)
		{
			return (int32)roundToInt(std::cos((float)x / (float)0x10000) * (float)0x10000);
		}

		template<typename T> T Math_PI()  { return PI_DOUBLE; }
		template<> float Math_PI()		  { return PI_FLOAT; }

		template<typename T> T Math_sqrt(T value)				{ return std::sqrt(value); }
		template<typename T> T Math_pow(T base, T exponent)		{ return std::pow(base, exponent); }
		template<typename T> T Math_exp(T value)				{ return std::exp(value); }
		template<typename T> T Math_log(T value)				{ return std::log(value); }

		template<typename T> T Math_sin(T value) 				{ return std::sin(value); }
		template<typename T> T Math_cos(T value)				{ return std::cos(value); }
		template<typename T> T Math_tan(T value)				{ return std::tan(value); }
		template<typename T> T Math_asin(T value)				{ return std::asin(value); }
		template<typename T> T Math_acos(T value)				{ return std::acos(value); }
		template<typename T> T Math_atan(T value)				{ return std::atan(value); }
		template<typename T> T Math_atan2(T y, T x)				{ return std::atan2(y, x); }

		template<typename T> T Math_degreesToRadians(T degrees)	{ return degrees * (Math_PI<T>() / (T)180); }
		template<typename T> T Math_radiansToDegrees(T radians)	{ return radians * ((T)180 / Math_PI<T>()); }

		template<typename T> T Math_floor(T value)				{ return std::floor(value); }
		template<typename T> int64 Math_floorToInt(T value)		{ return (int64)std::floor(value); }
		template<typename T> T Math_ceil(T value)				{ return std::ceil(value); }
		template<typename T> int64 Math_ceilToInt(T value)		{ return (int64)std::ceil(value); }
		template<typename T> T Math_round(T value)				{ return std::round(value); }
		template<typename T> int64 Math_roundToInt(T value)		{ return (int64)std::round(value); }

		template<typename T> bool Math_isNumber(T value)		{ return std::isnormal(value) || (value == (T)0); }
		template<typename T> bool Math_isNaN(T value)			{ return std::isnan(value); }
		template<typename T> bool Math_isInfinite(T value)		{ return std::isinf(value); }

		StringRef stringformat(StringRef format, int argv, uint64* args)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			RMX_CHECK(format.isValid(), "Unable to resolve format string", return StringRef());

			std::string_view formatString = format.getString();
			const int length = (int)formatString.length();
			const char* fmtPtr = formatString.data();
			const char* fmtEnd = fmtPtr + length;

			static detail::FastStringStream result;
			result.clear();

			for (; fmtPtr < fmtEnd; ++fmtPtr)
			{
				if (argv <= 0)
				{
					// Warning: This means that additional '%' characters won't be processed at all, which also means that escaped ones won't be reduces to a single one
					//  -> There's scripts that rely on this exact behavior, so don't ever change that!
					result.addString(fmtPtr, (int)(fmtEnd - fmtPtr));
					break;
				}

				// Continue until getting a '%' character
				{
					const char* fmtStart = fmtPtr;
					while (fmtPtr != fmtEnd && *fmtPtr != '%')
					{
						++fmtPtr;
					}
					if (fmtPtr != fmtStart)
					{
						result.addString(fmtStart, (int)(fmtPtr - fmtStart));
					}
					if (fmtPtr == fmtEnd)
						break;
				}

				const int remaining = (int)(fmtEnd - fmtPtr);
				if (remaining >= 2)
				{
					char numberOutputCharacter = 0;
					int minDigits = 0;
					int charsRead = 0;

					if (fmtPtr[1] == '%')
					{
						result.addChar('%');
						charsRead = 1;
					}
					else if (fmtPtr[1] == 's')
					{
						// String argument
						const FlyweightString* argStoredString = runtime->resolveStringByKey(args[0]);
						if (nullptr == argStoredString)
							result.addString("<?>", 3);
						else
							result.addString(argStoredString->getString());
						++args;
						--argv;
						charsRead = 1;
					}
					else if (fmtPtr[1] == 'd' || fmtPtr[1] == 'b' || fmtPtr[1] == 'x')
					{
						// Integer argument
						numberOutputCharacter = fmtPtr[1];
						charsRead = 1;
					}
					else if (remaining >= 4 && fmtPtr[1] == '0' && (fmtPtr[2] >= '1' && fmtPtr[2] <= '9') && (fmtPtr[3] == 'd' || fmtPtr[3] == 'b' || fmtPtr[3] == 'x'))
					{
						// Integer argument with minimum number of digits (9 or less)
						numberOutputCharacter = fmtPtr[3];
						minDigits = (int)(fmtPtr[2] - '0');
						charsRead = 3;
					}
					else if (remaining >= 5 && fmtPtr[1] == '0' && (fmtPtr[2] >= '1' && fmtPtr[2] <= '9') && (fmtPtr[3] >= '0' && fmtPtr[3] <= '9') && (fmtPtr[4] == 'd' || fmtPtr[4] == 'b' || fmtPtr[4] == 'x'))
					{
						// Integer argument with minimum number of digits (10 or more)
						numberOutputCharacter = fmtPtr[4];
						minDigits = (int)(fmtPtr[2] - '0') * 10 + (int)(fmtPtr[3] - '0');
						charsRead = 4;
					}
					else
					{
						result.addChar('%');
					}

					if (numberOutputCharacter != 0)
					{
						if (numberOutputCharacter == 'd')
						{
							result.addDecimal(args[0], minDigits);
						}
						else if (numberOutputCharacter == 'b')
						{
							result.addBinary(args[0], minDigits);
						}
						else if (numberOutputCharacter == 'x')
						{
							result.addHex(args[0], minDigits);
						}
						++args;
						--argv;
					}

					fmtPtr += charsRead;
				}
				else
				{
					result.addChar('%');
				}
			}

			return StringRef(runtime->addString(std::string_view(result.mBuffer, result.mLength)));
		}

		StringRef stringformat1(StringRef format, uint64 arg1)
		{
			return stringformat(format, 1, &arg1);
		}

		StringRef stringformat2(StringRef format, uint64 arg1, uint64 arg2)
		{
			uint64 args[] = { arg1, arg2 };
			return stringformat(format, 2, args);
		}

		StringRef stringformat3(StringRef format, uint64 arg1, uint64 arg2, uint64 arg3)
		{
			uint64 args[] = { arg1, arg2, arg3 };
			return stringformat(format, 3, args);
		}

		StringRef stringformat4(StringRef format, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4)
		{
			uint64 args[] = { arg1, arg2, arg3, arg4 };
			return stringformat(format, 4, args);
		}

		StringRef stringformat5(StringRef format, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4, uint64 arg5)
		{
			uint64 args[] = { arg1, arg2, arg3, arg4, arg5 };
			return stringformat(format, 5, args);
		}

		StringRef stringformat6(StringRef format, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4, uint64 arg5, uint64 arg6)
		{
			uint64 args[] = { arg1, arg2, arg3, arg4, arg5, arg6 };
			return stringformat(format, 6, args);
		}

		StringRef stringformat7(StringRef format, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4, uint64 arg5, uint64 arg6, uint64 arg7)
		{
			uint64 args[] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
			return stringformat(format, 7, args);
		}

		StringRef stringformat8(StringRef format, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4, uint64 arg5, uint64 arg6, uint64 arg7, uint64 arg8)
		{
			uint64 args[] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 };
			return stringformat(format, 8, args);
		}

		uint32 string_length(StringRef str)
		{
			RMX_CHECK(str.isValid(), "Unable to resolve string", return 0);
			return (uint32)str.getString().length();
		}

		bool string_isEmpty(StringRef str)
		{
			RMX_CHECK(str.isValid(), "Unable to resolve string", return 0);
			return str.getString().empty();
		}

		uint8 string_getCharacter(StringRef string, uint32 index)
		{
			if (!string.isValid())
				return 0;
			if (index >= string.getString().length())
				return 0;
			return string.getString()[index];
		}

		StringRef string_getSubString(StringRef string, uint32 index, uint32 length)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			if (!string.isValid())
				return StringRef();

			const std::string_view part = string.getString().substr(index, length);
			return StringRef(runtime->addString(part));
		}

		StringRef getStringFromHash(uint64 hash)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			const FlyweightString* str = runtime->resolveStringByKey(hash);
			return (nullptr == str) ? StringRef() : StringRef(*str);
		}
	}


	void StandardLibrary::registerBindings(lemon::Module& module)
	{
		// Register built-in functions
		BuiltInFunctions::registerBuiltInFunctions(module);

		const BitFlagSet<Function::Flag> defaultFlags(Function::Flag::ALLOW_INLINE_EXECUTION);
		const BitFlagSet<Function::Flag> compileTimeConstant(Function::Flag::ALLOW_INLINE_EXECUTION, Function::Flag::COMPILE_TIME_CONSTANT);

		module.addNativeFunction("min", lemon::wrap(&functions::minimum<int8>), compileTimeConstant);
		module.addNativeFunction("min", lemon::wrap(&functions::minimum<uint8>), compileTimeConstant);
		module.addNativeFunction("min", lemon::wrap(&functions::minimum<int16>), compileTimeConstant);
		module.addNativeFunction("min", lemon::wrap(&functions::minimum<uint16>), compileTimeConstant);
		module.addNativeFunction("min", lemon::wrap(&functions::minimum<int32>), compileTimeConstant);
		module.addNativeFunction("min", lemon::wrap(&functions::minimum<uint32>), compileTimeConstant);

		module.addNativeFunction("max", lemon::wrap(&functions::maximum<int8>), compileTimeConstant);
		module.addNativeFunction("max", lemon::wrap(&functions::maximum<uint8>), compileTimeConstant);
		module.addNativeFunction("max", lemon::wrap(&functions::maximum<int16>), compileTimeConstant);
		module.addNativeFunction("max", lemon::wrap(&functions::maximum<uint16>), compileTimeConstant);
		module.addNativeFunction("max", lemon::wrap(&functions::maximum<int32>), compileTimeConstant);
		module.addNativeFunction("max", lemon::wrap(&functions::maximum<uint32>), compileTimeConstant);

		module.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int8>), compileTimeConstant);
		module.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint8>), compileTimeConstant);
		module.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int16>), compileTimeConstant);
		module.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint16>), compileTimeConstant);
		module.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int32>), compileTimeConstant);
		module.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint32>), compileTimeConstant);

		module.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint8, int8>), compileTimeConstant);
		module.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint16, int16>), compileTimeConstant);
		module.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint32, int32>), compileTimeConstant);

		module.addNativeFunction("sqrt", lemon::wrap(&functions::sqrt_u32), compileTimeConstant);

		module.addNativeFunction("sin_s16", lemon::wrap(&functions::sin_s16), compileTimeConstant);
		module.addNativeFunction("sin_s32", lemon::wrap(&functions::sin_s32), compileTimeConstant);
		module.addNativeFunction("cos_s16", lemon::wrap(&functions::cos_s16), compileTimeConstant);
		module.addNativeFunction("cos_s32", lemon::wrap(&functions::cos_s32), compileTimeConstant);

		// Math
		{
			module.addNativeFunction("Math.sqrt",  lemon::wrap(&functions::Math_sqrt<float>),   compileTimeConstant);
			module.addNativeFunction("Math.sqrt",  lemon::wrap(&functions::Math_sqrt<double>),  compileTimeConstant);
			module.addNativeFunction("Math.pow",   lemon::wrap(&functions::Math_pow<float>),    compileTimeConstant);
			module.addNativeFunction("Math.pow",   lemon::wrap(&functions::Math_pow<double>),   compileTimeConstant);
			module.addNativeFunction("Math.exp",   lemon::wrap(&functions::Math_exp<float>),    compileTimeConstant);
			module.addNativeFunction("Math.exp",   lemon::wrap(&functions::Math_exp<double>),   compileTimeConstant);
			module.addNativeFunction("Math.log",   lemon::wrap(&functions::Math_log<float>),    compileTimeConstant);
			module.addNativeFunction("Math.log",   lemon::wrap(&functions::Math_log<double>),   compileTimeConstant);

			module.addNativeFunction("Math.sin",   lemon::wrap(&functions::Math_sin<float>),    compileTimeConstant);
			module.addNativeFunction("Math.sin",   lemon::wrap(&functions::Math_sin<double>),   compileTimeConstant);
			module.addNativeFunction("Math.cos",   lemon::wrap(&functions::Math_cos<float>),    compileTimeConstant);
			module.addNativeFunction("Math.cos",   lemon::wrap(&functions::Math_cos<double>),   compileTimeConstant);
			module.addNativeFunction("Math.tan",   lemon::wrap(&functions::Math_tan<float>),    compileTimeConstant);
			module.addNativeFunction("Math.tan",   lemon::wrap(&functions::Math_tan<double>),   compileTimeConstant);
			module.addNativeFunction("Math.asin",  lemon::wrap(&functions::Math_asin<float>),   compileTimeConstant);
			module.addNativeFunction("Math.asin",  lemon::wrap(&functions::Math_asin<double>),  compileTimeConstant);
			module.addNativeFunction("Math.acos",  lemon::wrap(&functions::Math_acos<float>),   compileTimeConstant);
			module.addNativeFunction("Math.acos",  lemon::wrap(&functions::Math_acos<double>),  compileTimeConstant);
			module.addNativeFunction("Math.atan",  lemon::wrap(&functions::Math_atan<float>),   compileTimeConstant);
			module.addNativeFunction("Math.atan",  lemon::wrap(&functions::Math_atan<double>),  compileTimeConstant);
			module.addNativeFunction("Math.atan2", lemon::wrap(&functions::Math_atan2<float>),  compileTimeConstant);
			module.addNativeFunction("Math.atan2", lemon::wrap(&functions::Math_atan2<double>), compileTimeConstant);

			module.addNativeFunction("Math.degreesToRadians", lemon::wrap(&functions::Math_degreesToRadians<float>),  compileTimeConstant);
			module.addNativeFunction("Math.degreesToRadians", lemon::wrap(&functions::Math_degreesToRadians<double>), compileTimeConstant);
			module.addNativeFunction("Math.radiansToDegrees", lemon::wrap(&functions::Math_radiansToDegrees<float>),  compileTimeConstant);
			module.addNativeFunction("Math.radiansToDegrees", lemon::wrap(&functions::Math_radiansToDegrees<double>), compileTimeConstant);

			module.addNativeFunction("Math.floor",		lemon::wrap(&functions::Math_floor<float>),		  compileTimeConstant);
			module.addNativeFunction("Math.floor",		lemon::wrap(&functions::Math_floor<double>),	  compileTimeConstant);
			module.addNativeFunction("Math.floorToInt",	lemon::wrap(&functions::Math_floorToInt<float>),  compileTimeConstant);
			module.addNativeFunction("Math.floorToInt",	lemon::wrap(&functions::Math_floorToInt<double>), compileTimeConstant);
			module.addNativeFunction("Math.ceil",		lemon::wrap(&functions::Math_ceil<float>),		  compileTimeConstant);
			module.addNativeFunction("Math.ceil",		lemon::wrap(&functions::Math_ceil<double>),		  compileTimeConstant);
			module.addNativeFunction("Math.ceilToInt",	lemon::wrap(&functions::Math_ceilToInt<float>),   compileTimeConstant);
			module.addNativeFunction("Math.ceilToInt",	lemon::wrap(&functions::Math_ceilToInt<double>),  compileTimeConstant);
			module.addNativeFunction("Math.round",		lemon::wrap(&functions::Math_round<float>),		  compileTimeConstant);
			module.addNativeFunction("Math.round",		lemon::wrap(&functions::Math_round<double>),	  compileTimeConstant);
			module.addNativeFunction("Math.roundToInt",	lemon::wrap(&functions::Math_roundToInt<float>),  compileTimeConstant);
			module.addNativeFunction("Math.roundToInt",	lemon::wrap(&functions::Math_roundToInt<double>), compileTimeConstant);

			module.addNativeFunction("Math.isNumber",	lemon::wrap(&functions::Math_isNumber<float>),	  compileTimeConstant);
			module.addNativeFunction("Math.isNumber",	lemon::wrap(&functions::Math_isNumber<double>),   compileTimeConstant);
			module.addNativeFunction("Math.isNaN",		lemon::wrap(&functions::Math_isNaN<float>),		  compileTimeConstant);
			module.addNativeFunction("Math.isNaN",		lemon::wrap(&functions::Math_isNaN<double>),	  compileTimeConstant);
			module.addNativeFunction("Math.isInfinite",	lemon::wrap(&functions::Math_isInfinite<float>),  compileTimeConstant);
			module.addNativeFunction("Math.isInfinite",	lemon::wrap(&functions::Math_isInfinite<double>), compileTimeConstant);
		}

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat1), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat2), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat3), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2")
			.setParameterInfo(3, "arg3");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat4), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2")
			.setParameterInfo(3, "arg3")
			.setParameterInfo(4, "arg4");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat5), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2")
			.setParameterInfo(3, "arg3")
			.setParameterInfo(4, "arg4")
			.setParameterInfo(5, "arg5");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat6), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2")
			.setParameterInfo(3, "arg3")
			.setParameterInfo(4, "arg4")
			.setParameterInfo(5, "arg5")
			.setParameterInfo(6, "arg6");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat7), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2")
			.setParameterInfo(3, "arg3")
			.setParameterInfo(4, "arg4")
			.setParameterInfo(5, "arg5")
			.setParameterInfo(6, "arg6")
			.setParameterInfo(7, "arg7");

		module.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat8), defaultFlags)
			.setParameterInfo(0, "format")
			.setParameterInfo(1, "arg1")
			.setParameterInfo(2, "arg2")
			.setParameterInfo(3, "arg3")
			.setParameterInfo(4, "arg4")
			.setParameterInfo(5, "arg5")
			.setParameterInfo(6, "arg6")
			.setParameterInfo(7, "arg7")
			.setParameterInfo(8, "arg8");

		module.addNativeFunction("strlen", lemon::wrap(&functions::string_length), defaultFlags)
			.setParameterInfo(0, "str");

		module.addNativeFunction("getchar", lemon::wrap(&functions::string_getCharacter), defaultFlags)
			.setParameterInfo(0, "str")
			.setParameterInfo(1, "index");

		module.addNativeFunction("substring", lemon::wrap(&functions::string_getSubString), defaultFlags)
			.setParameterInfo(0, "str")
			.setParameterInfo(1, "index")
			.setParameterInfo(2, "length");

		module.addNativeMethod("string", "length", lemon::wrap(&functions::string_length), defaultFlags);

		module.addNativeMethod("string", "isEmpty", lemon::wrap(&functions::string_isEmpty), defaultFlags);

		module.addNativeMethod("string", "getCharacter", lemon::wrap(&functions::string_getCharacter), defaultFlags)
			.setParameterInfo(0, "str")
			.setParameterInfo(1, "index");

		module.addNativeMethod("string", "getSubString", lemon::wrap(&functions::string_getSubString), defaultFlags)
			.setParameterInfo(0, "str")
			.setParameterInfo(1, "index")
			.setParameterInfo(2, "length");

		module.addNativeFunction("getStringFromHash", lemon::wrap(&functions::getStringFromHash), defaultFlags)
			.setParameterInfo(0, "hash");
	}
}
