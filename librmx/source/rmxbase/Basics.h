/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef _MSC_VER
	#include <intrin.h>		// for _byteswap_*
#endif


// Safe deletion
#define SAFE_DELETE(x)		 { if (nullptr != x) { delete (x); (x) = nullptr; } }
#define SAFE_DELETE_ARRAY(x) { if (nullptr != x) { delete[] (x); (x) = nullptr; } }


// Conversion Little Endian <-> Big Endian
FORCE_INLINE static uint16 swapBytes16(uint16 value)
{
#ifdef _MSC_VER
	return _byteswap_ushort(value);
#elif defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap16(value);
#else
	return (value << 8) | (value >> 8);
#endif
}

FORCE_INLINE static uint32 swapBytes32(uint32 value)
{
#ifdef _MSC_VER
	return _byteswap_ulong(value);
#elif defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap32(value);
#else
	return (value << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | (value >> 24);
#endif
}

FORCE_INLINE static uint64 swapBytes64(uint64 value)
{
#ifdef _MSC_VER
	return _byteswap_uint64(value);
#elif defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap64(value);
#else
	value = ((value >> 8) & 0x00ff00ff00ff00ffULL) | ((value << 8) & 0xff00ff00ff00ff00ULL);
	value = ((value >> 16) & 0x0000ffff0000ffffULL) | ((value << 16) & 0xffff0000ffff0000ULL);
	return ((value >> 32) & 0x00000000ffffffffULL) | ((value << 32) & 0xffffffff00000000ULL);
#endif
}


// Invert bool
FORCE_INLINE void toggle(bool& variable)	{ variable = !variable; }

// Clamp to interval
FORCE_INLINE int clamp(int value, int vmin, int vmax)				{ return ((value < vmin) ? vmin : (value > vmax) ? vmax : value); }
FORCE_INLINE float clamp(float value, float vmin, float vmax)		{ return ((value < vmin) ? vmin : (value > vmax) ? vmax : value); }
FORCE_INLINE double clamp(double value, double vmin, double vmax)	{ return ((value < vmin) ? vmin : (value > vmax) ? vmax : value); }

// Clamp to interval [0, 1]
FORCE_INLINE float saturate(float value)	{ return ((value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value); }
FORCE_INLINE double saturate(double value)	{ return ((value < 0.0)  ? 0.0  : (value > 1.0)  ? 1.0  : value); }

inline float wrapToInterval(float value, float a, float b)
{
	const float length = b - a;
	assert(length > 0.0f);
	const float normalized = (value - a) / length;
	return a + (normalized - floor(normalized)) * length;
}

// Range check
FORCE_INLINE bool inside(int value, int vmin, int vmax)				{ return (value >= vmin && value <= vmax); }
FORCE_INLINE bool inside(float value, float vmin, float vmax)		{ return (value >= vmin && value <= vmax); }
FORCE_INLINE bool inside(double value, double vmin, double vmax)	{ return (value >= vmin && value <= vmax); }

// Round to integer
FORCE_INLINE int roundToInt(float value)		{ return (int)floor(value + 0.5f); }
FORCE_INLINE float roundToFloat(float value)	{ return floor(value + 0.5f); }

// Round for integer, identity for floats
template<typename T> static T roundForInt(float value)  { return (T)value; }


// Linear interpolation
FORCE_INLINE float interpolate(float v0, float v1, float factor)		{ return v0 + (v1 - v0) * factor; }
FORCE_INLINE double interpolate(double v0, double v1, double factor)	{ return v0 + (v1 - v0) * factor; }

// Cubic interpolation
float interpolate(float v0, float v1, float v2, float v3, float factor);
double interpolate(double v0, double v1, double v2, double v3, double factor);

namespace rmx
{
	// Base 2 logarithm (floored)
	FUNCTION_EXPORT int log2(unsigned int value);
}


// Random number generation initialization (using time())
FUNCTION_EXPORT void randomize();

// Random number generation initialization with custom seed
void randomize(unsigned int seed);

// Random integer using uniform distribution in [0, range-1]
FUNCTION_EXPORT int random(int range);

// Random float using uniform distribution in [0.0f, 1.0f]
FUNCTION_EXPORT float randomf();

// Random float using a standard normal distribution (E = 0.0, sigma = 1.0)
FUNCTION_EXPORT float nrandom();


#ifndef __STDC_WANT_SECURE_LIB__
	#define strcpy_s(dst, size, src)					strcpy(dst, src);
	#define wcscpy_s(dst, size, src)					wcscpy(dst, src);
	#define sprintf_s(dst, size, format, arg)			sprintf(dst, format, arg)
	#define swprintf_s(dst, size, format, arg)			swprintf(dst, format, arg)
	#define vsnprintf_s(dst, size, s2, format, arg)		vsnprintf(dst, size, format, arg)
	#ifdef __GNUC__
		#define _vsnwprintf_s(dst, size, s2, format, arg)	vswprintf(dst, size, format, arg)
	#else
		#define _vsnwprintf_s(dst, size, s2, format, arg)	vsnwprintf(dst, size, format, arg)
	#endif
	#define wcstombs_s(ret, dst, size, src, len)		{ *ret = wcstombs(dst, src, len); }
	#define mbstowcs_s(ret, dst, size, src, len)		{ *ret = mbstowcs(dst, src, len); }
#endif

#ifdef __GNUC__
	#define OFFSETOF(TYPE, member) ((int)&((TYPE*)0x100)->member - 0x100)
#else
	#define OFFSETOF offsetof
#endif


// String functions
FORCE_INLINE long strTol(const char* str)		{ char* ptr;    return strtol(str, &ptr, 0); }
FORCE_INLINE long strTol(const wchar_t* str)	{ wchar_t* ptr; return wcstol(str, &ptr, 0); }

FORCE_INLINE double strTod(const char* str)		{ char* ptr;    return strtod(str, &ptr); }
FORCE_INLINE double strTod(const wchar_t* str)	{ wchar_t* ptr; return wcstod(str, &ptr); }

FORCE_INLINE void strCpy(char* dst, int dstSize, const char* src)		{ strcpy_s(dst, (size_t)dstSize, src); }
FORCE_INLINE void strCpy(wchar_t* dst, int dstSize, const wchar_t* src)	{ wcscpy_s(dst, (size_t)dstSize, src); }


// Comparisons
FORCE_INLINE int compare(int first, int second)		{ return (first < second) ? -1 : (first > second) ? +1 : 0; }
FORCE_INLINE int compare(float first, float second)	{ return (first < second) ? -1 : (first > second) ? +1 : 0; }
FORCE_INLINE int compare(void* first, void* second)	{ return (first < second) ? -1 : (first > second) ? +1 : 0; }
FORCE_INLINE int compare(bool first, bool second)	{ return (first == second) ? 0 : second ? +1 : -1; }

#define COMPARE_CASCADE(x) \
{ \
	int cmp = x; \
	if (cmp != 0) \
		return cmp; \
}



// Add element to std::vector
template<typename T>
T& vectorAdd(std::vector<T>& vec)
{
	vec.emplace_back();
	return vec.back();
}

// Remove element from std::vector by swapping with the last one
template<typename T>
bool vectorRemoveSwap(std::vector<T>& vec, size_t index)
{
	if (index + 1 < vec.size())
		std::swap(vec[index], vec.back());
	else if (index >= vec.size())
		return false;
	vec.pop_back();
	return true;
}


// Find an element in a std::map
template<typename K, typename V>
V* mapFind(std::map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template<typename K, typename V>
const V* mapFind(const std::map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template<typename K, typename V>
V* mapFind(std::unordered_map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template<typename K, typename V>
const V* mapFind(const std::unordered_map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}
