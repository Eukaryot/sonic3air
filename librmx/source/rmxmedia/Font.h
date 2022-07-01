/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Font
*		Rendering of text.
*/

#pragma once

class FontOutput;
class FontProcessor;
class FontSource;


struct StringReader
{
	const char* mString = nullptr;
	const wchar_t* mWString = nullptr;
	size_t mLength = 0;

	StringReader(const char* str)		  : mString(str), mLength(strlen(str)) {}
	StringReader(const wchar_t* str)	  : mWString(str), mLength(wcslen(str)) {}
	StringReader(const String& str)		  : mString(*str), mLength(str.length()) {}
	StringReader(const WString& str)	  : mWString(*str), mLength(str.length()) {}
	StringReader(const std::string& str)  : mString(str.c_str()), mLength(str.length()) {}
	StringReader(const std::wstring& str) : mWString(str.c_str()), mLength(str.length()) {}
	StringReader(std::string_view str)	  : mString(str.data()), mLength(str.length()) {}
	StringReader(std::wstring_view str)	  : mWString(str.data()), mLength(str.length()) {}

	uint32 operator[](size_t index) const  { return (nullptr != mString) ? (uint32)(uint8)mString[index] : (uint32)mWString[index]; }
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

	inline bool operator==(const FontSourceKey& other) const { return (compare(other) == 0); }
	inline bool operator<(const FontSourceKey& other) const  { return (compare(other) < 0); }
};


struct FontKey : public FontSourceKey
{
	std::vector<std::shared_ptr<FontProcessor>> mProcessors;

	int compare(const FontKey& other) const;
	inline bool operator==(const FontKey& other) const { return (compare(other) == 0); }
	inline bool operator<(const FontKey& other) const  { return (compare(other) < 0); }
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
	explicit Font(float size);
	~Font();

	bool loadFromFile(const String& filename, float size = 0.0f);
	void setSize(float size);

	void clearFontProcessors();
	void addFontProcessor(const std::shared_ptr<FontProcessor>& processor);

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

	void printBitmap(Bitmap& outBitmap, Vec2i& outDrawPosition, const Recti& drawRect, const StringReader& text, int alignment = 1, int spacing = 0);
	void printBitmap(Bitmap& outBitmap, Recti& outInnerRect, const StringReader& text, int spacing = 0);

	FontOutput& getFontOutput();

public:
	static Vec2i applyAlignment(const Recti& drawRect, const Recti& innerRect, int alignment);

private:
	void invalidateFontSource();
	FontSource* getFontSource();

private:
	FontSource* mFontSource = nullptr;
	bool mFontSourceDirty = true;
	FontOutput* mFontOutput = nullptr;
	FontKey mKey;
	float mAdvance = 0.0f;

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
