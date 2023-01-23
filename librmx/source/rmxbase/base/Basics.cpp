/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"
#include <time.h>


namespace rmx
{
	template<> int16  swapBytes(int16 value)  { return swapBytes16(value); }
	template<> uint16 swapBytes(uint16 value) { return swapBytes16(value); }
	template<> int32  swapBytes(int32 value)  { return swapBytes32(value); }
	template<> uint32 swapBytes(uint32 value) { return swapBytes32(value); }
	template<> int64  swapBytes(int64 value)  { return swapBytes64(value); }
	template<> uint64 swapBytes(uint64 value) { return swapBytes64(value); }
}


float interpolate(float v0, float v1, float v2, float v3, float factor)
{
	// Cubic interpolation
	float a0 = v3 - v2 - v0 + v1;
	float a1 = v0 - v1 - a0;
	float a2 = v2 - v0;
	float a3 = v1;
	return ((a0 * factor + a1) * factor + a2) * factor + a3;
}

double interpolate(double v0, double v1, double v2, double v3, double factor)
{
	// Cubic interpolation
	double a0 = v3 - v2 - v0 + v1;
	double a1 = v0 - v1 - a0;
	double a2 = v2 - v0;
	double a3 = v1;
	return ((a0 * factor + a1) * factor + a2) * factor + a3;
}

namespace rmx
{
	int log2(unsigned int value)
	{
		// Get integer logarithm
		unsigned int result = 0;
		if (value >= 0x10000)  { result += 16; value >>= 16; }
		if (value >= 0x100)    { result += 8;  value >>= 8;  }
		if (value >= 0x10)     { result += 4;  value >>= 4;  }
		if (value >= 0x04)     { result += 2;  value >>= 2;  }
		if (value >= 0x02)     { result += 1;  value >>= 1;  }
		return result;
	}
}

void randomize()
{
	time_t t;
	time(&t);
	srand((unsigned int)t);
	(void)rand();		// Skip the first random result, which might be just the seed again
}

void randomize(unsigned int seed)
{
	srand(seed);
}

int random(int range)
{
	if (range < 1)
		return 0;
	return rand() % range;
}

float randomf()
{
	// Uniformly distributed random number in [0.0f, 1.0f]
	return (float)rand() / (float)(RAND_MAX - 1);
}

float nrandom()
{
	// Standard normal distributed random number
	static const float lookup[52] =
	{
		0.00000f, 0.02506f, 0.05015f, 0.07527f, 0.10043f, 0.12566f, 0.15096f, 0.17637f, 0.20189f, 0.22754f,
		0.25334f, 0.27931f, 0.30548f, 0.33185f, 0.35845f, 0.38532f, 0.41246f, 0.43991f, 0.46769f, 0.49585f,
		0.52440f, 0.55338f, 0.58284f, 0.61281f, 0.64334f, 0.67449f, 0.70630f, 0.73884f, 0.77219f, 0.80642f,
		0.84162f, 0.87789f, 0.91536f, 0.95416f, 0.99445f, 1.03643f, 1.08032f, 1.12639f, 1.17498f, 1.22652f,
		1.28155f, 1.34075f, 1.40507f, 1.47579f, 1.55477f, 1.64485f, 1.75068f, 1.88079f, 2.05375f, 2.32635f, 3.0000f, 3.0000f
	};
	const float rnd = (randomf() - 0.5f) * 100.0f;
	if (rnd < 0.0f)
	{
		const int irnd = (int)(-rnd);
		const float frnd = (-rnd) - (float)irnd;
		return -(lookup[irnd] * (1.0f - frnd) + lookup[irnd+1] * frnd);
	}
	else
	{
		const int irnd = (int)rnd;
		const float frnd = rnd - (float)irnd;
		return lookup[irnd] * (1.0f - frnd) + lookup[irnd+1] * frnd;
	}
}

template<> inline signed char roundForInt(float value)		{ return (signed char)round(value); }
template<> inline unsigned char roundForInt(float value)	{ return (unsigned char)round(value); }
template<> inline signed short roundForInt(float value)		{ return (signed short)round(value); }
template<> inline unsigned short roundForInt(float value)	{ return (unsigned short)round(value); }
template<> inline signed int roundForInt(float value)		{ return (signed int)round(value); }
template<> inline unsigned int roundForInt(float value)		{ return (unsigned int)round(value); }
