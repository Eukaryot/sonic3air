/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

class VectorBinarySerializer;


struct API_EXPORT Color : public Vec4f
{
public:
	static const Color BLACK;
	static const Color GRAY;
	static const Color WHITE;
	static const Color RED;
	static const Color YELLOW;
	static const Color GREEN;
	static const Color CYAN;
	static const Color BLUE;
	static const Color MAGENTA;
	static const Color TRANSPARENT;

	enum class Encoding
	{
		RGBA_32,
		ARGB_32,
		ABGR_32
	};

public:
	inline static Color fromRGBA32(uint32 colorRGBA)  { return Color(colorRGBA, Encoding::RGBA_32); }
	inline static Color fromARGB32(uint32 colorARGB)  { return Color(colorARGB, Encoding::ARGB_32); }
	inline static Color fromABGR32(uint32 colorABGR)  { return Color(colorABGR, Encoding::ABGR_32); }

	inline static Color fromHSL(const Vec3f& hsl)	  { Color color;  color.setFromHSL(hsl);  color.a = 1.0f;  return color; }
	inline static Color fromHSV(const Vec3f& hsv)	  { Color color;  color.setFromHSV(hsv);  color.a = 1.0f;  return color; }
	inline static Color fromYUV(const Vec3f& yuv)	  { Color color;  color.setFromYUV(yuv);  color.a = 1.0f;  return color; }

	static Color interpolateColor(const Color& c0, const Color& c1, float factor);

public:
	inline Color() {}
	inline Color(const Vec4f& vec) : Vec4f(vec) {}
	inline Color(float r_, float g_, float b_, float a_ = 1.0f) : Vec4f(r_, g_, b_, a_) {}
	Color(uint32 color, Encoding encoding) : Vec4f(true)  { setByEncoding(color, encoding); }

	inline void set(const Color& color) { r = color.r; g = color.g; b = color.b; a = color.a; }
	inline void set(float r_, float g_, float b_, float a_ = 1.0f) { r = r_; g = g_; b = b_; a = a_; }

	uint32 getRGBA32() const;			// R is the highest 8 bits, A is the lowest 8 bits
	uint32 getARGB32() const;			// A is the highest 8 bits, G is the lowest 8 bits
	uint32 getABGR32() const;			// A is the highest 8 bits, R is the lowest 8 bits

	void setByEncoding(uint32 color, Encoding encoding);
	void setRGBA32(uint32 colorRGBA);	// R is the highest 8 bits, A is the lowest 8 bits
	void setARGB32(uint32 colorARGB);	// A is the highest 8 bits, B is the lowest 8 bits
	void setABGR32(uint32 colorABGR);	// A is the highest 8 bits, R is the lowest 8 bits

	inline void setGray(float gray, float alpha = 1.0f) { r = gray; g = gray; b = gray; a = alpha; }
	void setFromHSL(const Vec3f& hsl);
	Vec3f getHSL() const;
	void setFromHSV(const Vec3f& hsv);
	Vec3f getHSV() const;
	void setFromYUV(const Vec3f& yuv);
	Vec3f getYUV() const;
	inline float getGray() const		{ return (r + g + b) / 3.0f; }
	inline float getIntensity() const	{ return r * 0.299f + g * 0.587f + b * 0.114f; }
	inline void saturate()				{ r = ::saturate(r); g = ::saturate(g); b = ::saturate(b); a = ::saturate(a); }

	void swapRedBlue();
	Color blendOver(const Color& dest) const;

	void serialize(VectorBinarySerializer& serializer);

	Color operator+(const Color& other) const;
	Color operator-(const Color& other) const;
	Color operator*(const Color& other) const;
	Color operator*(float factor) const;
	const Color& operator+=(const Color& other);
	const Color& operator-=(const Color& other);
	const Color& operator*=(const Color& other);
	const Color& operator*=(float factor);

private:
	inline explicit Color(uint32 colorRGBA) : Vec4f(true)  { setRGBA32(colorRGBA); }
};
