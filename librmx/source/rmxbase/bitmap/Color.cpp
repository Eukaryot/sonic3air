/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


const Color Color::BLACK  (0.0f, 0.0f, 0.0f);
const Color Color::GRAY   (0.5f, 0.5f, 0.5f);
const Color Color::WHITE  (1.0f, 1.0f, 1.0f);
const Color Color::RED    (1.0f, 0.0f, 0.0f);
const Color Color::YELLOW (1.0f, 1.0f, 0.0f);
const Color Color::GREEN  (0.0f, 1.0f, 0.0f);
const Color Color::CYAN   (0.0f, 1.0f, 1.0f);
const Color Color::BLUE   (0.0f, 0.0f, 1.0f);
const Color Color::MAGENTA(1.0f, 0.0f, 1.0f);
const Color Color::TRANSPARENT(0.0f, 0.0f, 0.0f, 0.0f);


Color Color::interpolateColor(const Color& c0, const Color& c1, float factor)
{
	Color result;
	result.interpolate(c0, c1, factor);
	return result;
}


uint32 Color::getRGBA32() const
{
	return ((uint32)(::saturate(r) * 255) << 24)
		 + ((uint32)(::saturate(g) * 255) << 16)
		 + ((uint32)(::saturate(b) * 255) << 8)
		 + ((uint32)(::saturate(a) * 255));
}

uint32 Color::getARGB32() const
{
	return ((uint32)(::saturate(r) * 255) << 16)
		 + ((uint32)(::saturate(g) * 255) << 8)
		 + ((uint32)(::saturate(b) * 255))
		 + ((uint32)(::saturate(a) * 255) << 24);
}

uint32 Color::getABGR32() const
{
	return ((uint32)(::saturate(r) * 255))
		 + ((uint32)(::saturate(g) * 255) << 8)
		 + ((uint32)(::saturate(b) * 255) << 16)
		 + ((uint32)(::saturate(a) * 255) << 24);
}

void Color::setByEncoding(uint32 color, Encoding encoding)
{
	switch (encoding)
	{
		case Encoding::RGBA_32:  setRGBA32(color);  break;
		case Encoding::ARGB_32:  setARGB32(color);  break;
		case Encoding::ABGR_32:  setABGR32(color);  break;
	}
}

void Color::setRGBA32(uint32 colorRGBA)
{
	r = (float)((colorRGBA >> 24) & 0xff) / 255.0f;
	g = (float)((colorRGBA >> 16) & 0xff) / 255.0f;
	b = (float)((colorRGBA >> 8)  & 0xff) / 255.0f;
	a = (float)((colorRGBA)       & 0xff) / 255.0f;
}

void Color::setARGB32(uint32 colorARGB)
{
	r = (float)((colorARGB >> 16) & 0xff) / 255.0f;
	g = (float)((colorARGB >> 8)  & 0xff) / 255.0f;
	b = (float)((colorARGB)       & 0xff) / 255.0f;
	a = (float)((colorARGB >> 24) & 0xff) / 255.0f;
}

void Color::setABGR32(uint32 colorABGR)
{
	r = (float)((colorABGR)       & 0xff) / 255.0f;
	g = (float)((colorABGR >> 8)  & 0xff) / 255.0f;
	b = (float)((colorABGR >> 16) & 0xff) / 255.0f;
	a = (float)((colorABGR >> 24) & 0xff) / 255.0f;
}

void Color::setHSL(const Vec3f& hsl)
{
	// Conversion HSL -> RGB
	float maximum = (hsl.z < 0.5f) ? (hsl.z * (1.0f + hsl.y)) : (hsl.z * (1.0f - hsl.y) + hsl.y);
	float minimum = 2.0f * hsl.z - maximum;
	float hue = hsl.x * 6.0f;

	r = (hue < 4.0f) ? (hue + 2.0f) : (hue - 4.0f);
	g = hue;
	b = (hue < 2.0f) ? (hue + 4.0f) : (hue - 2.0f);

	for (int i = 0; i < 3; ++i)
	{
		if (data[i] < 1.0f)
			data[i] = minimum + (maximum - minimum) * data[i];
		else if (data[i] < 3.0f)
			data[i] = maximum;
		else if (data[i] < 4.0f)
			data[i] = minimum + (maximum - minimum) * (4.0f - data[i]);
		else
			data[i] = minimum;
	}
}

Vec3f Color::getHSL() const
{
	// Conversion RGB -> HSL
	float minimum = std::min(std::min(r, g), b);
	float maximum = std::max(std::max(r, g), b);
	float delta = maximum - minimum;
	if (delta < 0.001f)
	{
		return Vec3f(0.0f, 0.0f, maximum);
	}

	float lightness = (minimum + maximum) / 2.0f;
	float saturation = delta / ((lightness < 0.5f) ? (minimum + maximum) : (2.0f - maximum - minimum));

	float hue;
	if (maximum == r)
		hue = (g - b) / delta + ((g < b) ? 6.0f : 0.0f);
	else if (maximum == g)
		hue = (b - r) / delta + 2.0f;
	else
		hue = (r - g) / delta + 4.0f;

	return Vec3f(hue / 6.0f, saturation, lightness);
}

void Color::setYUV(const Vec3f& yuv)
{
	// Conversion YUV -> RGB
	r = yuv.x + yuv.z * 1.14f;
	g = yuv.x - yuv.y * 0.39466f - yuv.z * 0.5806f;
	b = yuv.x + yuv.y * 2.028f;
}

Vec3f Color::getYUV() const
{
	// Conversion RGB -> YUV
	Vec3f yuv;
	yuv.x = 0.299f * r + 0.587f * g + 0.114f * b;
	yuv.y = (b - yuv.x) * 0.493f;
	yuv.z = (r - yuv.x) * 0.877f;
	return yuv;
}

void Color::swapRedBlue()
{
	std::swap(x, z);
}

Color Color::blendOver(const Color& dest) const
{
	Color result;
	result.a = 1.0f - (1.0f - a) * (1.0f - dest.a);
	if (result.a > 0.0f)
	{
		float fs = a / result.a;
		float fd = dest.a * (1.0f - a) / result.a;
		result.r = r * fs + dest.r * fd;
		result.g = g * fs + dest.g * fd;
		result.b = b * fs + dest.b * fd;
	}
	return result;
}

void Color::serialize(VectorBinarySerializer& serializer)
{
	if (serializer.isReading())
	{
		setABGR32(serializer.read<uint32>());
	}
	else
	{
		serializer.write<uint32>(getABGR32());
	}
}

Color Color::operator+(const Color& other) const
{
	return Color(::saturate(r + other.r), ::saturate(g + other.g), ::saturate(b + other.b), ::saturate(a + other.a));
}

Color Color::operator-(const Color& other) const
{
	return Color(::saturate(r - other.r), ::saturate(g - other.g), ::saturate(b - other.b), ::saturate(a - other.a));
}

Color Color::operator*(const Color& other) const
{
	return Color(::saturate(r * other.r), ::saturate(g * other.g), ::saturate(b * other.b), ::saturate(a * other.a));
}

Color Color::operator*(float factor) const
{
	return Color(::saturate(r * factor), ::saturate(g * factor), ::saturate(b * factor), ::saturate(a * factor));
}

const Color& Color::operator+=(const Color& other)
{
	r += other.r;
	g += other.g;
	b += other.b;
	a += other.a;
	saturate();
	return *this;
}

const Color& Color::operator-=(const Color& other)
{
	r -= other.r;
	g -= other.g;
	b -= other.b;
	a -= other.a;
	saturate();
	return *this;
}

const Color& Color::operator*=(const Color& other)
{
	r *= other.r;
	g *= other.g;
	b *= other.b;
	a *= other.a;
	saturate();
	return *this;
}

const Color& Color::operator*=(float factor)
{
	r *= factor;
	g *= factor;
	b *= factor;
	a *= factor;
	saturate();
	return *this;
}
