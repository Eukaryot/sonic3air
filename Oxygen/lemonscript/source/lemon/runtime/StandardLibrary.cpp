/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/StandardLibrary.h"
#include "lemon/runtime/BuiltInFunctions.h"
#include "lemon/program/ModuleBindingsBuilder.h"
#include "lemon/program/Program.h"
#include "lemon/utility/FastStringStream.h"
#include "lemon/utility/StringFormatter.h"


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
			return (R)std::abs(a);
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

		template<typename T> T Math_sqr(T value)				{ return value * value; }
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

		template<typename T> T Math_degreesToRadians(T degrees)		{ return degrees * (Math_PI<T>() / (T)180); }
		template<typename T> T Math_radiansToDegrees(T radians)		{ return radians * ((T)180 / Math_PI<T>()); }
		template<typename T> T Math_u8ToDegrees(uint8 angle)		{ return (T)angle * ((T)360 / (T)256); }
		template<typename T> T Math_u8ToRadians(uint8 angle)		{ return (T)angle * (Math_PI<T>() / (T)128); }
		template<typename T> uint8 Math_u8FromDegrees(T degrees)	{ return (uint8)std::round(degrees * ((T)256 / (T)360)); }
		template<typename T> uint8 Math_u8FromRadians(T radians)	{ return (uint8)std::round(radians * ((T)128 / Math_PI<T>())); }

		template<typename T> T Math_floor(T value)				{ return std::floor(value); }
		template<typename T> int64 Math_floorToInt(T value)		{ return (int64)std::floor(value); }
		template<typename T> T Math_ceil(T value)				{ return std::ceil(value); }
		template<typename T> int64 Math_ceilToInt(T value)		{ return (int64)std::ceil(value); }
		template<typename T> T Math_round(T value)				{ return std::round(value); }
		template<typename T> int64 Math_roundToInt(T value)		{ return (int64)std::round(value); }
		template<typename T> T Math_frac(T value)				{ return value - std::floor(value); }

		template<typename T> bool Math_isNumber(T value)		{ return std::isnormal(value) || (value == (T)0); }
		template<typename T> bool Math_isNaN(T value)			{ return std::isnan(value); }
		template<typename T> bool Math_isInfinite(T value)		{ return std::isinf(value); }

		template<typename T> T Math_lerp(T a, T b, T factor)			{ return a + (b - a) * factor; }
		template<typename T> T Math_lerp_int(T a, T b, float factor)	{ return a + roundToInt((float)(signed)(b - a) * factor); }
		template<typename T> T Math_invlerp(T a, T b, T value)			{ return (a == b) ? 0.0f : (value - a) / (b - a); }
		template<typename T> float Math_invlerp_int(T a, T b, T value)	{ return (a == b) ? 0.0f : (float)(value - a) / (float)(b - a); }

		StringRef stringformat(StringRef format, int numArguments, const uint64* args)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			RMX_CHECK(format.isValid(), "Unable to resolve format string", return StringRef());

			static detail::FastStringStream result;
			result.clear();
			StringFormatter::buildFormattedString_Legacy(result, format.getString(), numArguments, args);

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

		StringRef string_build(StringRef format, size_t numArguments, const AnyTypeWrapper* args)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			RMX_CHECK(format.isValid(), "Unable to resolve format string", return StringRef());

			static detail::FastStringStream result;
			result.clear();
			StringFormatter::buildFormattedString(result, format.getString(), numArguments, args);

			return StringRef(runtime->addString(std::string_view(result.mBuffer, result.mLength)));
		}

		StringRef string_build1(StringRef format, AnyTypeWrapper arg1)
		{
			return string_build(format, 1, &arg1);
		}

		StringRef string_build2(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2)
		{
			AnyTypeWrapper args[] = { arg1, arg2 };
			return string_build(format, 2, args);
		}

		StringRef string_build3(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2, AnyTypeWrapper arg3)
		{
			AnyTypeWrapper args[] = { arg1, arg2, arg3 };
			return string_build(format, 3, args);
		}

		StringRef string_build4(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2, AnyTypeWrapper arg3, AnyTypeWrapper arg4)
		{
			AnyTypeWrapper args[] = { arg1, arg2, arg3, arg4 };
			return string_build(format, 4, args);
		}

		StringRef string_build5(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2, AnyTypeWrapper arg3, AnyTypeWrapper arg4, AnyTypeWrapper arg5)
		{
			AnyTypeWrapper args[] = { arg1, arg2, arg3, arg4, arg5 };
			return string_build(format, 5, args);
		}

		StringRef string_build6(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2, AnyTypeWrapper arg3, AnyTypeWrapper arg4, AnyTypeWrapper arg5, AnyTypeWrapper arg6)
		{
			AnyTypeWrapper args[] = { arg1, arg2, arg3, arg4, arg5, arg6 };
			return string_build(format, 6, args);
		}

		StringRef string_build7(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2, AnyTypeWrapper arg3, AnyTypeWrapper arg4, AnyTypeWrapper arg5, AnyTypeWrapper arg6, AnyTypeWrapper arg7)
		{
			AnyTypeWrapper args[] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
			return string_build(format, 7, args);
		}

		StringRef string_build8(StringRef format, AnyTypeWrapper arg1, AnyTypeWrapper arg2, AnyTypeWrapper arg3, AnyTypeWrapper arg4, AnyTypeWrapper arg5, AnyTypeWrapper arg6, AnyTypeWrapper arg7, AnyTypeWrapper arg8)
		{
			AnyTypeWrapper args[] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 };
			return string_build(format, 8, args);
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

		bool string_startsWith(StringRef string, StringRef substring)
		{
			if (!string.isValid() || !substring.isValid())
				return false;
			return rmx::startsWith(string.getString(), substring.getString());
		}

		bool string_endsWith(StringRef string, StringRef substring)
		{
			if (!string.isValid() || !substring.isValid())
				return false;
			return rmx::endsWith(string.getString(), substring.getString());
		}

		int16 string_find(StringRef string, StringRef substring)
		{
			if (!string.isValid() || !substring.isValid())
				return false;
			const size_t position = string.getString().find(substring.getString());
			return (position == std::string_view::npos) ? -1 : (int16)position;
		}

		StringRef getStringFromCharacter(uint8 character)
		{
			Runtime* runtime = Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			const char str[2] = { (char)character, '\0' };
			return StringRef(runtime->addString(str));
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

		lemon::ModuleBindingsBuilder builder(module);

		// Constants
		builder.addConstant<float>("PI_FLOAT", PI_FLOAT);
		builder.addConstant<double>("PI_DOUBLE", PI_DOUBLE);

		// Functions
		const BitFlagSet<Function::Flag> defaultFlags(Function::Flag::ALLOW_INLINE_EXECUTION);
		const BitFlagSet<Function::Flag> compileTimeConstant(Function::Flag::ALLOW_INLINE_EXECUTION, Function::Flag::COMPILE_TIME_CONSTANT);
		const BitFlagSet<Function::Flag> excludeFromDefinitions(Function::Flag::EXCLUDE_FROM_DEFINITIONS);

		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<int8>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<uint8>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<int16>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<uint16>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<int32>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<uint32>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<int64>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<uint64>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<float>), compileTimeConstant);
		builder.addNativeFunction("min", lemon::wrap(&functions::minimum<double>), compileTimeConstant);

		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<int8>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<uint8>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<int16>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<uint16>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<int32>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<uint32>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<int64>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<uint64>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<float>), compileTimeConstant);
		builder.addNativeFunction("max", lemon::wrap(&functions::maximum<double>), compileTimeConstant);

		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int8>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint8>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int16>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint16>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int32>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint32>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<int64>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<uint64>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<float>), compileTimeConstant);
		builder.addNativeFunction("clamp", lemon::wrap(&functions::clamp<double>), compileTimeConstant);

		builder.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint8, int8>), compileTimeConstant);
		builder.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint16, int16>), compileTimeConstant);
		builder.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint32, int32>), compileTimeConstant);
		builder.addNativeFunction("abs", lemon::wrap(&functions::absolute<uint64, int64>), compileTimeConstant);
		builder.addNativeFunction("abs", lemon::wrap(&functions::absolute<float, float>), compileTimeConstant);
		builder.addNativeFunction("abs", lemon::wrap(&functions::absolute<double, double>), compileTimeConstant);

		builder.addNativeFunction("sqrt", lemon::wrap(&functions::sqrt_u32), compileTimeConstant);

		builder.addNativeFunction("sin_s16", lemon::wrap(&functions::sin_s16), compileTimeConstant);
		builder.addNativeFunction("sin_s32", lemon::wrap(&functions::sin_s32), compileTimeConstant);
		builder.addNativeFunction("cos_s16", lemon::wrap(&functions::cos_s16), compileTimeConstant);
		builder.addNativeFunction("cos_s32", lemon::wrap(&functions::cos_s32), compileTimeConstant);

		// Math
		{
			builder.addNativeFunction("Math.sqr",   lemon::wrap(&functions::Math_sqr<float>),	compileTimeConstant);
			builder.addNativeFunction("Math.sqr",   lemon::wrap(&functions::Math_sqr<double>),	compileTimeConstant);
			builder.addNativeFunction("Math.sqrt",  lemon::wrap(&functions::Math_sqrt<float>),   compileTimeConstant);
			builder.addNativeFunction("Math.sqrt",  lemon::wrap(&functions::Math_sqrt<double>),  compileTimeConstant);
			builder.addNativeFunction("Math.pow",   lemon::wrap(&functions::Math_pow<float>),    compileTimeConstant);
			builder.addNativeFunction("Math.pow",   lemon::wrap(&functions::Math_pow<double>),   compileTimeConstant);
			builder.addNativeFunction("Math.exp",   lemon::wrap(&functions::Math_exp<float>),    compileTimeConstant);
			builder.addNativeFunction("Math.exp",   lemon::wrap(&functions::Math_exp<double>),   compileTimeConstant);
			builder.addNativeFunction("Math.log",   lemon::wrap(&functions::Math_log<float>),    compileTimeConstant);
			builder.addNativeFunction("Math.log",   lemon::wrap(&functions::Math_log<double>),   compileTimeConstant);

			builder.addNativeFunction("Math.sin",   lemon::wrap(&functions::Math_sin<float>),    compileTimeConstant);
			builder.addNativeFunction("Math.sin",   lemon::wrap(&functions::Math_sin<double>),   compileTimeConstant);
			builder.addNativeFunction("Math.cos",   lemon::wrap(&functions::Math_cos<float>),    compileTimeConstant);
			builder.addNativeFunction("Math.cos",   lemon::wrap(&functions::Math_cos<double>),   compileTimeConstant);
			builder.addNativeFunction("Math.tan",   lemon::wrap(&functions::Math_tan<float>),    compileTimeConstant);
			builder.addNativeFunction("Math.tan",   lemon::wrap(&functions::Math_tan<double>),   compileTimeConstant);
			builder.addNativeFunction("Math.asin",  lemon::wrap(&functions::Math_asin<float>),   compileTimeConstant);
			builder.addNativeFunction("Math.asin",  lemon::wrap(&functions::Math_asin<double>),  compileTimeConstant);
			builder.addNativeFunction("Math.acos",  lemon::wrap(&functions::Math_acos<float>),   compileTimeConstant);
			builder.addNativeFunction("Math.acos",  lemon::wrap(&functions::Math_acos<double>),  compileTimeConstant);
			builder.addNativeFunction("Math.atan",  lemon::wrap(&functions::Math_atan<float>),   compileTimeConstant);
			builder.addNativeFunction("Math.atan",  lemon::wrap(&functions::Math_atan<double>),  compileTimeConstant);
			builder.addNativeFunction("Math.atan2", lemon::wrap(&functions::Math_atan2<float>),  compileTimeConstant);
			builder.addNativeFunction("Math.atan2", lemon::wrap(&functions::Math_atan2<double>), compileTimeConstant);

			builder.addNativeFunction("Math.degreesToRadians", lemon::wrap(&functions::Math_degreesToRadians<float>),  compileTimeConstant);
			builder.addNativeFunction("Math.degreesToRadians", lemon::wrap(&functions::Math_degreesToRadians<double>), compileTimeConstant);
			builder.addNativeFunction("Math.radiansToDegrees", lemon::wrap(&functions::Math_radiansToDegrees<float>),  compileTimeConstant);
			builder.addNativeFunction("Math.radiansToDegrees", lemon::wrap(&functions::Math_radiansToDegrees<double>), compileTimeConstant);
			builder.addNativeFunction("Math.u8ToDegrees",	  lemon::wrap(&functions::Math_u8ToDegrees<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.u8ToRadians",	  lemon::wrap(&functions::Math_u8ToRadians<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.u8FromDegrees",	  lemon::wrap(&functions::Math_u8FromDegrees<float>),	  compileTimeConstant);
			builder.addNativeFunction("Math.u8FromRadians",	  lemon::wrap(&functions::Math_u8FromRadians<float>),	  compileTimeConstant);

			builder.addNativeFunction("Math.floor",		lemon::wrap(&functions::Math_floor<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.floor",		lemon::wrap(&functions::Math_floor<double>),	  compileTimeConstant);
			builder.addNativeFunction("Math.floorToInt",	lemon::wrap(&functions::Math_floorToInt<float>),  compileTimeConstant);
			builder.addNativeFunction("Math.floorToInt",	lemon::wrap(&functions::Math_floorToInt<double>), compileTimeConstant);
			builder.addNativeFunction("Math.ceil",		lemon::wrap(&functions::Math_ceil<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.ceil",		lemon::wrap(&functions::Math_ceil<double>),		  compileTimeConstant);
			builder.addNativeFunction("Math.ceilToInt",	lemon::wrap(&functions::Math_ceilToInt<float>),   compileTimeConstant);
			builder.addNativeFunction("Math.ceilToInt",	lemon::wrap(&functions::Math_ceilToInt<double>),  compileTimeConstant);
			builder.addNativeFunction("Math.round",		lemon::wrap(&functions::Math_round<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.round",		lemon::wrap(&functions::Math_round<double>),	  compileTimeConstant);
			builder.addNativeFunction("Math.roundToInt",	lemon::wrap(&functions::Math_roundToInt<float>),  compileTimeConstant);
			builder.addNativeFunction("Math.roundToInt",	lemon::wrap(&functions::Math_roundToInt<double>), compileTimeConstant);
			builder.addNativeFunction("Math.frac",		lemon::wrap(&functions::Math_frac<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.frac",		lemon::wrap(&functions::Math_frac<double>),		  compileTimeConstant);

			builder.addNativeFunction("Math.isNumber",	lemon::wrap(&functions::Math_isNumber<float>),	  compileTimeConstant);
			builder.addNativeFunction("Math.isNumber",	lemon::wrap(&functions::Math_isNumber<double>),   compileTimeConstant);
			builder.addNativeFunction("Math.isNaN",		lemon::wrap(&functions::Math_isNaN<float>),		  compileTimeConstant);
			builder.addNativeFunction("Math.isNaN",		lemon::wrap(&functions::Math_isNaN<double>),	  compileTimeConstant);
			builder.addNativeFunction("Math.isInfinite",	lemon::wrap(&functions::Math_isInfinite<float>),  compileTimeConstant);
			builder.addNativeFunction("Math.isInfinite",	lemon::wrap(&functions::Math_isInfinite<double>), compileTimeConstant);

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp<float>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp<double>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<int8>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<uint8>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<int16>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<uint16>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<int32>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<uint32>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<int64>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.lerp", lemon::wrap(&functions::Math_lerp_int<uint64>), compileTimeConstant)
				.setParameters("a", "b", "factor");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp<float>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp<double>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<int8>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<uint8>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<int16>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<uint16>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<int32>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<uint32>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<int64>), compileTimeConstant)
				.setParameters("a", "b", "value");

			builder.addNativeFunction("Math.invlerp", lemon::wrap(&functions::Math_invlerp_int<uint64>), compileTimeConstant)
				.setParameters("a", "b", "value");
		}

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat1), defaultFlags)
			.setParameters("format", "arg1");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat2), defaultFlags)
			.setParameters("format", "arg1", "arg2");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat3), defaultFlags)
			.setParameters("format", "arg1", "arg2", "arg3");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat4), defaultFlags)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat5), defaultFlags)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat6), defaultFlags)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat7), defaultFlags)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6", "arg7");

		builder.addNativeFunction("stringformat", lemon::wrap(&functions::stringformat8), defaultFlags)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6", "arg7", "arg8");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build1), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build2), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build3), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2", "arg3");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build4), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build5), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build6), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build7), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6", "arg7");

		builder.addNativeFunction("string.build", lemon::wrap(&functions::string_build8), defaultFlags | excludeFromDefinitions)
			.setParameters("format", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6", "arg7", "arg8");

		builder.addNativeFunction("strlen", lemon::wrap(&functions::string_length), defaultFlags)
			.setParameters("str");

		builder.addNativeFunction("getchar", lemon::wrap(&functions::string_getCharacter), defaultFlags)
			.setParameters("str", "index");

		builder.addNativeFunction("substring", lemon::wrap(&functions::string_getSubString), defaultFlags)
			.setParameters("str", "index", "length");

		builder.addNativeMethod("string", "length", lemon::wrap(&functions::string_length), defaultFlags);

		builder.addNativeMethod("string", "isEmpty", lemon::wrap(&functions::string_isEmpty), defaultFlags);

		builder.addNativeMethod("string", "getCharacter", lemon::wrap(&functions::string_getCharacter), defaultFlags)
			.setParameters("this", "index");

		builder.addNativeMethod("string", "getSubString", lemon::wrap(&functions::string_getSubString), defaultFlags)
			.setParameters("this", "index", "length");

		builder.addNativeMethod("string", "startsWith", lemon::wrap(&functions::string_startsWith), defaultFlags)
			.setParameters("this", "substring");

		builder.addNativeMethod("string", "endsWith", lemon::wrap(&functions::string_endsWith), defaultFlags)
			.setParameters("this", "substring");

		builder.addNativeMethod("string", "find", lemon::wrap(&functions::string_find), defaultFlags)
			.setParameters("this", "substring");

		builder.addNativeFunction("getStringFromCharacter", lemon::wrap(&functions::getStringFromCharacter), defaultFlags)
			.setParameters("character");

		builder.addNativeFunction("getStringFromHash", lemon::wrap(&functions::getStringFromHash), defaultFlags)
			.setParameters("hash");
	}
}
