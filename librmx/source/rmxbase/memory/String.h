/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "rmxbase/base/Basics.h"

#include <vector>


template<typename CHAR, typename CLASS> class StringTemplate;


enum class UnicodeEncoding
{
	AUTO = -1,
	ASCII,
	UTF8,
	UTF16BE,
	UTF16LE,
	UTF32BE,
	UTF32LE
};


// Template for concrete classes
template<typename CHAR, typename CLASS>
class StringTemplate
{
friend class String;
friend class WString;

public:
	static const StringTemplate EMPTY;

	typedef typename std::basic_string<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> StdString;
	typedef typename std::basic_string_view<CHAR> StdStringView;

public:
	StringTemplate();
	StringTemplate(const StringTemplate& str);
	StringTemplate(const CHAR* str);
	StringTemplate(const CHAR* str, size_t length);
	StringTemplate(const StdString& str);
	StringTemplate(const StdStringView& str);
	StringTemplate(int ignoreMe, const CHAR* format, ...);
	~StringTemplate();

	void fromConst(const CHAR* str);
	void fromStatic(CHAR* memptr, int memsize, bool clear = true);
	void fromDynamic(CHAR* memptr, int memsize, int len);
	void makeDynamic();

	StringTemplate& clear();
	void recount();
	void reserve(size_t newReservedLength);
	void expand(int newLength);
	void expand(size_t newLength) { expand((int)newLength); }
	void expand(int newLength, int insertspace);
	void setLength(int newLength);
	void setLength(size_t newLength) { setLength((int)newLength); }

	inline const CHAR* getData() const		{ return mData; }
	inline CHAR* accessData()				{ return mData; }
	inline CHAR getChar(size_t index)		{ return (index < mLength) ? mData[index] : 0; }
	inline CHAR getChar(int index)			{ return (index >= 0 && index < (int)mLength) ? mData[index] : 0; }
	inline uint32 getUnicode(size_t index)	{ return toUnicode(getChar(index)); }
	inline uint32 getUnicode(int index)		{ return toUnicode(getChar(index)); }

	inline int length() const			{ return (int)mLength; }
	inline bool empty() const			{ return (mLength == 0); }
	inline bool isEmpty() const			{ return (mLength == 0); }
	inline bool nonEmpty() const		{ return (mLength != 0); }
	inline int getReservedSize() const	{ return mSize; }

	void copy(const StringTemplate& str);
	void copy(const CHAR* str);
	void copy(const StdString& str);
	void copy(const StdStringView& str);

	void swap(StringTemplate& other);

	void add(const StringTemplate& str);
	void add(const StringTemplate& str, int pos, int len);
	void add(CHAR ch);
	void add(CHAR ch, int count);
	void add(const CHAR* ch, int count);

	void addInt(int value);
	void addInt(unsigned int value);
	void addHex(unsigned int value, int fillzeroes);
	void addHex(unsigned int value);
	void addFloat(float value, int precision = 0);
	void addDouble(double value, int precision = 0);
	void addDouble(double value);
	void addData(void* inputdata, int bytes);

	int parseInt() const;
	float parseFloat() const;
	double parseDouble() const;
	int parseData(void* dst) const;

	bool equal(const StringTemplate& str) const;
	int compare(const StringTemplate& str) const;
	int compare(const CHAR* str) const;

	int countChar(CHAR ch) const;
	int findChar(CHAR ch, int pos, int dir) const;
	int skipChar(CHAR ch, int pos, int dir) const;
	int findChars(const CHAR* chars, int pos, int dir) const;
	int skipChars(const CHAR* chars, int pos, int dir) const;

	int findString(const StringTemplate& str, int pos = 0, int dir = +1) const;		// Returns -1 if substring not found

	void replace(CHAR ch, CHAR newch);
	void replace(const StringTemplate& toreplace, const StringTemplate& replacement);

	bool includes(const CHAR* str) const;
	bool includes(const CHAR* str, int& pos) const;
	bool includesAt(const CHAR* str, int pos) const;
	bool includesAt(StdStringView str, int pos) const;
	bool startsWith(const CHAR* str) const;
	bool startsWith(StdStringView str) const;
	bool endsWith(const CHAR* str) const;
	bool endsWith(const CHAR* str, int length) const;
	bool endsWith(StdStringView str) const;

	void makeSubString(int pos, int len);
	void makeSubString(const StringTemplate& str, int pos, int len);
	void makeSubString(const StringTemplate& str, int pos);
	CLASS getSubString(int pos, int len) const;
	CLASS getSubString(int pos) const;

	void split(std::vector<CLASS>& output, CHAR separator) const;
	void split(std::vector<StdStringView>& output, CHAR separator) const;
	void compose(const std::vector<CLASS>& parts, const StringTemplate& separator);

	void overwrite(const StringTemplate& str, int pos);
	void insert(const StringTemplate& str, int pos);
	void remove(int pos, int len);

	void fillLeft(CHAR ch, int len);
	void fillRight(CHAR ch, int len);

	void trimWhitespace();

	void lowerCase();
	void upperCase();

	void format(const CHAR* format, ...);

	int getLine(StringTemplate& output, int pos) const;
	int getLine(size_t& length, int pos) const;

	uint8* extractData(size_t& datasize);

	bool readUnicode(const uint8* data, size_t datasize, UnicodeEncoding encoding = UnicodeEncoding::AUTO);
	void writeUnicode(std::vector<uint8>& buffer, UnicodeEncoding encoding = UnicodeEncoding::AUTO, bool addBOM = true) const;

	bool loadFile(std::string_view filename, UnicodeEncoding encoding = UnicodeEncoding::AUTO);
	bool loadFile(std::wstring_view filename, UnicodeEncoding encoding = UnicodeEncoding::AUTO);
	bool saveFile(std::string_view filename, UnicodeEncoding encoding = UnicodeEncoding::AUTO) const;
	bool saveFile(std::wstring_view filename, UnicodeEncoding encoding = UnicodeEncoding::AUTO) const;

	CLASS& operator=(const CLASS& str)			{ copy(str); return (CLASS&)*this; }
	CLASS& operator=(const CHAR* str)			{ copy(str); return (CLASS&)*this; }
	CLASS& operator=(const StdString& str)		{ copy(str); return *this; }
	CLASS& operator=(const StdStringView& str)	{ copy(str); return *this; }

	CLASS& operator<<(const CLASS& str)		{ add(str);			return (CLASS&)*this; }
	CLASS& operator<<(CHAR ch)				{ add(ch);			return (CLASS&)*this; }
	CLASS& operator<<(int value)			{ addInt(value);    return (CLASS&)*this; }
	CLASS& operator<<(unsigned int value)	{ addInt(value);    return (CLASS&)*this; }
	CLASS& operator<<(float value)			{ addFloat(value);  return (CLASS&)*this; }
	CLASS& operator<<(double value)			{ addDouble(value); return (CLASS&)*this; }

	CLASS operator+(const CLASS& str) const		{ CLASS result(*this); result.add(str);			return result; }
	CLASS operator+(CHAR ch) const				{ CLASS result(*this); result.add(ch);			return result; }
	CLASS operator+(int value) const			{ CLASS result(*this); result.addInt(value);    return result; }
	CLASS operator+(unsigned int value) const	{ CLASS result(*this); result.addInt(value);    return result; }
	CLASS operator+(float value) const			{ CLASS result(*this); result.addFloat(value);  return result; }
	CLASS operator+(double value) const			{ CLASS result(*this); result.addDouble(value); return result; }

	inline bool operator==(const CHAR* str) const	{ return compare(str) == 0; }
	inline bool operator!=(const CHAR* str) const	{ return compare(str) != 0; }
	inline bool operator<=(const CHAR* str) const	{ return compare(str) <= 0; }
	inline bool operator>=(const CHAR* str) const	{ return compare(str) >= 0; }
	inline bool operator<(const CHAR* str) const	{ return compare(str) < 0; }
	inline bool operator>(const CHAR* str) const	{ return compare(str) > 0; }

	inline bool operator==(const CLASS& str) const	{ return equal(str); }
	inline bool operator!=(const CLASS& str) const	{ return !equal(str); }
	inline bool operator<=(const CLASS& str) const	{ return compare(str) <= 0; }
	inline bool operator>=(const CLASS& str) const	{ return compare(str) >= 0; }
	inline bool operator<(const CLASS& str) const	{ return compare(str) < 0; }
	inline bool operator>(const CLASS& str) const	{ return compare(str) > 0; }

	inline const CHAR* operator*() const				{ return mData; }
	inline const CHAR operator[](size_t index) const	{ return mData[index]; }
	inline CHAR& operator[](size_t index)				{ return mData[index]; }

private:
	void init();
	int sprintf(CHAR* dst, size_t dstSize, const CHAR* format, ...);

protected:
	CHAR* mData = nullptr;		// Pointer to the actual data
	size_t mLength = 0;			// Length of the string, without the terminating zero
	size_t mSize = 0;			// Size of dynamically allocated memory including terminating zero, in characters (not bytes!); note that this is 0 for constant strings
	bool mDynamic = false;		// true if memory was dynamically allocated
};



class WString;

class API_EXPORT String : public StringTemplate<char, String>
{
public:
	typedef StringTemplate<char, String> BASE;

public:
	String() : BASE() {}
	String(const String& str) : BASE(str) {}
	String(const StringTemplate& str) : BASE(str) {}
	String(const char* str) : BASE(str) {}
	String(const char* str, size_t length) : BASE(str, length) {}
	String(const StdString& str) : BASE(str) {}
	String(const StdStringView& str) : BASE(str) {}
	String(int ignoreMe, const char* format, ...);

	void formatString(const char* format, ...);

	String& operator=(const String& str) { copy(str); return *this; }

	WString toWString() const;
	std::string toStdString() const;
	std::wstring toStdWString() const;

	operator const std::string_view() const  { return std::string_view(mData, mLength); }
};


class API_EXPORT WString : public StringTemplate<wchar_t, WString>
{
public:
	typedef StringTemplate<wchar_t, WString> BASE;

public:
	WString() : BASE() {}
	WString(const WString& str) : BASE(str) {}
	WString(const StringTemplate& str) : BASE(str) {}
	WString(const wchar_t* str) : BASE(str) {}
	WString(const wchar_t* str, size_t length) : BASE(str, length) {}
	WString(const StdString& str) : BASE(str) {}
	WString(const StdStringView& str) : BASE(str) {}
	explicit WString(const String& str) : BASE(str.toWString()) {}
	explicit WString(const char* str) : WString(String(str)) {}
	explicit WString(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& str) : WString(String(str)) {}
	explicit WString(const std::basic_string_view<char>& str) : WString(String(str)) {}
	WString(int ignoreMe, const wchar_t* format, ...);

	void formatString(const wchar_t* format, ...);

	WString& operator=(const WString& str) { copy(str); return *this; }

	String toString() const;
	String toUTF8() const;
	std::string toStdString() const;
	std::wstring toStdWString() const;

	operator const std::wstring_view() const  { return std::wstring_view(mData, mLength); }

	void fromUTF8(const char* str, size_t length);
	void fromUTF8(const String& str);
	void fromUTF8(const std::string& str);

	static uint32 readUTF8(const char*& str, size_t& length);
};



namespace rmx
{
	template<typename CHAR>
	struct StringTraits
	{
		static CHAR fromUnicode(uint32 code);
		static uint32 toUnicode(CHAR ch);
		static int buildFormatted(CHAR* dst, size_t dstSize, const CHAR* format, va_list argv);
	};
}
