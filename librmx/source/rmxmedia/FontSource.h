/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	FontSource
*		Source of font data (glyph bitmaps etc.).
*/

#pragma once


class API_EXPORT FontSource
{
public:
	struct GlyphInfo
	{
		uint32 unicode;
		Bitmap bitmap;
		int leftIndent;
		int topIndent;
		int advance;
	};

public:
	virtual ~FontSource() {}
	virtual const GlyphInfo* getGlyph(uint32 unicode);

	inline int getAscender() const	 { return mAscender; }
	inline int getDescender() const	 { return mDescender; }
	inline int getHeight() const	 { return mAscender + mDescender; }
	inline int getLineHeight() const { return mLineHeight; }

protected:
	virtual bool fillGlyphInfo(GlyphInfo& info)  { return false; }

protected:
	std::map<uint32, GlyphInfo> mGlyphMap;
	int mAscender = 0;		// Maximum height in pixels
	int mDescender = 0;		// Maximum depth in pixels
	int mLineHeight = 0;	// Line height in pixels
};


class API_EXPORT FontSourceStd : public FontSource
{
public:
	FontSourceStd(float size);

protected:
	virtual bool fillGlyphInfo(GlyphInfo& info);

private:
	float mSize;
};


class API_EXPORT FontSourceBitmap : public FontSource
{
public:
	FontSourceBitmap(const String& jsonFilename);

protected:
	virtual bool fillGlyphInfo(GlyphInfo& info);

private:
	std::map<wchar_t, Bitmap>  mCharacterBitmaps;
	std::map<wchar_t, wchar_t> mCharacterRedirects;
	int mSpaceBetweenCharacters = 0;
};
