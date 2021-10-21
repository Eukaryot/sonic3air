/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Font
*		Rendering of text.
*/

#pragma once


struct StringReader
{
	const char* mString;
	const wchar_t* mWString;
	size_t mLength;

	StringReader(const char* str)		  : mString(str),  mWString(nullptr), mLength(strlen(str))  {}
	StringReader(const wchar_t* str)	  : mString(nullptr), mWString(str),  mLength(wcslen(str))  {}
	StringReader(const String& str)		  : mString(*str), mWString(nullptr), mLength(str.length()) {}
	StringReader(const WString& str)	  : mString(nullptr), mWString(*str), mLength(str.length()) {}
	StringReader(const std::string& str)  : mString(str.c_str()), mWString(nullptr), mLength(str.length()) {}
	StringReader(const std::wstring& str) : mString(nullptr), mWString(str.c_str()), mLength(str.length()) {}

	uint32 operator[](size_t index) const  { return mString ? (uint32)(uint8)mString[index] : (uint32)mWString[index]; }
};


struct FontProcessingData
{
	Bitmap mBitmap;
	int mBorderLeft = 0;
	int mBorderRight = 0;
	int mBorderTop = 0;
	int mBorderBottom = 0;
};

struct FontProcessor
{
	virtual void process(FontProcessingData& data) = 0;
};


struct FontSourceKey
{
	String mName;
	float mSize;

	FontSourceKey() : mSize(-1.0f) {}

	int compare(const FontSourceKey& other) const
	{
		COMPARE_CASCADE(::compare(mSize, other.mSize));
		COMPARE_CASCADE(mName.compare(other.mName));
		return 0;
	}

	bool operator==(const FontSourceKey& other) const { return (compare(other) == 0); }
	bool operator<(const FontSourceKey& other) const  { return (compare(other) < 0); }
};


struct FontKey : public FontSourceKey
{
	FontProcessor* mProcessor = nullptr;
	bool mShadowEnabled = false;
	Vec2f mShadowOffset = Vec2f(1.0f, 1.0f);
	float mShadowBlur = 1.0f;
	float mShadowAlpha = 1.0f;

	int compare(const FontKey& other) const
	{
		COMPARE_CASCADE(::compare(mProcessor, other.mProcessor));
		COMPARE_CASCADE(FontSourceKey::compare(other));
		COMPARE_CASCADE(::compare(mShadowEnabled, other.mShadowEnabled));
		if (mShadowEnabled)
		{
			COMPARE_CASCADE(::compare(mShadowOffset.x, other.mShadowOffset.x));
			COMPARE_CASCADE(::compare(mShadowOffset.y, other.mShadowOffset.y));
			COMPARE_CASCADE(::compare(mShadowBlur, other.mShadowBlur));
			COMPARE_CASCADE(::compare(mShadowAlpha, other.mShadowAlpha));
		}
		return 0;
	}

	bool operator==(const FontKey& other) const { return (compare(other) == 0); }
	bool operator<(const FontKey& other) const  { return (compare(other) < 0); }
};


class Font
{
public:
	struct TypeInfo
	{
		uint32 unicode = 0;
		const Bitmap* bitmap = nullptr;
		Vec2f pos;
	};

public:
	Font();
	Font(const String& filename, float size);
	Font(float size);
	~Font();

	void load(const String& filename, float size);
	void setSize(float size);
	void setShadow(bool enable, const Vec2f offset = Vec2f(1,1), float blur = 1.0f, float alpha = 1.0f);
	void addFontProcessor(FontProcessor& processor);

	const FontKey& getKey() const	{ return mKey; }
	const String& getName() const	{ return mKey.mName; }
	float getSize() const			{ return mKey.mSize; }

	int getWidth(const StringReader& text);
	int getWidth(const StringReader& text, int pos, int len = -1);
	int getHeight();
	int getLineHeight();

	Vec2f alignText(const Rectf& rect, const StringReader& text, int alignment);
	void wordWrapText(std::vector<std::wstring>& output, int lineWidth, const StringReader& text, int spacing = 0);
	void getTypeInfos(std::vector<TypeInfo>& output, Vec2f pos, const StringReader& text, int spacing = 0);

	void print(int x, int y, const StringReader& text, int alignment = 1);
	void print(int x, int y, int w, int h, const StringReader& text, int alignment = 1);
	void print(const Rectf& rect, const StringReader& text, int alignment = 1);

	void printBitmap(Bitmap& outBitmap, Vec2i& outDrawPosition, const Recti& rect, const StringReader& text, int alignment = 1, int spacing = 0);

private:
	void rebuildFontSource();

private:
	FontSource* mFontSource = nullptr;
	FontKey mKey;
	float mAdvance;

public:
	struct API_EXPORT CodecList
	{
		std::vector<class IFontSourceFactory*> mList;
		template<class CLASS> void add() { mList.push_back(new CLASS()); }
	};
	static CodecList mCodecs;
};



class IFontSourceFactory
{
public:
	virtual FontSource* construct(const FontSourceKey& key) = 0;
};

class FontSourceStdFactory : public IFontSourceFactory
{
public:
	virtual FontSource* construct(const FontSourceKey& key) override;
};

class FontSourceBitmapFactory : public IFontSourceFactory
{
public:
	virtual FontSource* construct(const FontSourceKey& key) override;
};
