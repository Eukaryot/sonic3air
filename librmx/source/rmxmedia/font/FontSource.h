/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
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
		uint32 mUnicode = 0;
		Bitmap mBitmap;
		int mAdvance = 0;
		Vec2f mIndent;
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
	explicit FontSourceStd(float size);

protected:
	virtual bool fillGlyphInfo(GlyphInfo& info);

private:
	float mSize;
};


class API_EXPORT FontSourceBitmap : public FontSource
{
public:
	explicit FontSourceBitmap(const String& jsonFilename);

	bool isValid() const  { return mLoadingSucceeded; }

protected:
	virtual bool fillGlyphInfo(GlyphInfo& info);

private:
	std::unordered_map<wchar_t, Bitmap>  mCharacterBitmaps;
	std::unordered_map<wchar_t, wchar_t> mCharacterRedirects;
	int mSpaceBetweenCharacters = 0;
	bool mLoadingSucceeded = false;
};
