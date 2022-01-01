/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/


template<typename CHAR> CHAR _space();
template<typename CHAR> CHAR _tabulator();

template<> inline char _space()			{ return ' '; }
template<> inline char _tabulator()		{ return '\t'; }

template<> inline wchar_t _space()		{ return L' '; }
template<> inline wchar_t _tabulator()	{ return L'\t'; }


template<typename CHAR> const CHAR* _empty();
template<typename CHAR> const CHAR* _format_i();
template<typename CHAR> const CHAR* _format_x();
template<typename CHAR> const CHAR* _format_f();
template<typename CHAR> const CHAR* _format_0f();

template<> inline const char* _empty()			{ return ""; }
template<> inline const char* _format_i()		{ return "%i"; }
template<> inline const char* _format_x()		{ return "%x"; }
template<> inline const char* _format_f()		{ return "%f"; }
template<> inline const char* _format_0f()		{ return "%.0f"; }

template<> inline const wchar_t* _empty()		{ return L""; }
template<> inline const wchar_t* _format_i()	{ return L"%i"; }
template<> inline const wchar_t* _format_x()	{ return L"%x"; }
template<> inline const wchar_t* _format_f()	{ return L"%f"; }
template<> inline const wchar_t* _format_0f()	{ return L"%.0f"; }


#define TEMPLATE template<typename CHAR, typename CLASS>
#define STRING StringTemplate<CHAR, CLASS>


TEMPLATE void STRING::init()
{
	mData = const_cast<CHAR*>(_empty<CHAR>());
	mLength = 0;
	mSize = 0;
	mDynamic = false;
}

TEMPLATE int STRING::sprintf(CHAR* dst, size_t dstSize, const CHAR* format, ...)
{
	va_list argv;
	va_start(argv, format);
	int ret = rmx::StringTraits<CHAR>::buildFormatted(dst, dstSize, format, argv);
	va_end(argv);
	return ret;
}

TEMPLATE STRING::StringTemplate()
{
	init();
}

TEMPLATE STRING::StringTemplate(const STRING& str)
{
	// Copy of another string
	mLength = str.mLength;
	mSize = mLength + 1;
	mData = new CHAR[mSize];
	memcpy(mData, str.mData, mSize * sizeof(CHAR));
	mDynamic = true;
}

TEMPLATE STRING::StringTemplate(const CHAR* str)
{
	init();
	if (nullptr == str || str[0] == 0)
		return;

	size_t len = 0;
	while (str[len])
		++len;

	expand(len);
	memcpy(mData, str, len * sizeof(CHAR));
	mLength = len;
	mData[mLength] = 0;
}

TEMPLATE STRING::StringTemplate(const CHAR* str, size_t length)
{
	init();
	if (nullptr == str || str[0] == 0)
		return;

	expand((int)length);
	memcpy(mData, str, length * sizeof(CHAR));
	mLength = (int)length;
	mData[mLength] = 0;
}

TEMPLATE STRING::StringTemplate(const std::basic_string<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>>& str)
{
	init();
	if (str.empty())
		return;

	expand((int)str.length());
	memcpy(mData, &str[0], str.length() * sizeof(CHAR));
	mLength = (int)str.length();
	mData[mLength] = 0;
}

TEMPLATE STRING::StringTemplate(const std::basic_string_view<CHAR>& str)
{
	init();
	if (str.empty())
		return;

	expand((int)str.length());
	memcpy(mData, &str[0], str.length() * sizeof(CHAR));
	mLength = (int)str.length();
	mData[mLength] = 0;
}

TEMPLATE STRING::StringTemplate(int ignoreMe, const CHAR* format, ...)
{
	// Create from format string
	init();
	if (nullptr == format || format[0] == 0)
		return;

	va_list argv;
	va_start(argv, format);
	static CHAR buffer[1024];
	rmx::StringTraits<CHAR>::buildFormatted(buffer, 1024, format, argv);
	va_end(argv);

	int len = 0;
	while (buffer[len])
		++len;
	expand(len);
	memcpy(mData, buffer, len * sizeof(CHAR));
	mLength = len;
	mData[mLength] = 0;
}

TEMPLATE STRING::~StringTemplate()
{
	if (mDynamic)
		delete[] mData;
}

TEMPLATE void STRING::fromConst(const CHAR* str)
{
	if (mDynamic)
		delete[] mData;
	if (nullptr == str)
		str = _empty<CHAR>();
	mData = (CHAR*)str;
	recount();
	mSize = 0;
	mDynamic = false;
}

TEMPLATE void STRING::fromStatic(CHAR* memptr, int memsize, bool clear)
{
	if (mDynamic)
		delete[] mData;
	mData = memptr;
	if (clear)
	{
		mData[0] = 0;
		mLength = 0;
	}
	else
	{
		recount();
	}
	mSize = memsize;
	mDynamic = false;
}

TEMPLATE void STRING::fromDynamic(CHAR* memptr, int memsize, int len)
{
	if (mDynamic && mData && (mData != memptr))
		delete[] mData;
	mData = memptr;
	mSize = memsize;
	mLength = len;
	mDynamic = true;
}

TEMPLATE void STRING::makeDynamic()
{
	if (mDynamic)
		return;

	CHAR* newData = new CHAR[mLength+1];
	memcpy(newData, mData, (mLength+1) * sizeof(CHAR));
	mData = newData;
	mSize = mLength+1;
	mDynamic = true;
}

TEMPLATE STRING& STRING::clear()
{
	if (!mDynamic && !mSize)
		mData = const_cast<CHAR*>(_empty<CHAR>());
	else
		mData[0] = 0;
	mLength = 0;
	return *this;
}

TEMPLATE void STRING::recount()
{
	mLength = 0;
	while (mData[mLength])
		++mLength;
}

TEMPLATE void STRING::expand(int newLength)
{
	// Increase reserved size if needed
	if (newLength+1 <= (int)mSize)
		return;

	int newSize = 1;
	while (newSize < newLength+1)
		newSize *= 2;
	CHAR* newData = new CHAR[newSize];
	if (mLength > 0)
		memcpy(newData, mData, mLength * sizeof(CHAR));
	newData[mLength] = 0;
	if (mDynamic)
		delete[] mData;
	mData = newData;
	mSize = newSize;
	mDynamic = true;
}

TEMPLATE void STRING::expand(int newLength, int insertspace)
{
	// Increase reserved size if needed
	if (insertspace < 0)
		insertspace = 0;
	if (insertspace > newLength - (int)mLength)
		insertspace = newLength - (int)mLength;
	if (newLength+1 <= (int)mSize)
	{
		if (insertspace > 0)
			memmove(&mData[insertspace], mData, (mLength+1) * sizeof(CHAR));
		mLength += insertspace;
		return;
	}

	int newSize = 1;
	while (newSize < newLength+1)
		newSize *= 2;
	CHAR* newData = new CHAR[newSize];
	memcpy(&newData[insertspace], mData, mLength * sizeof(CHAR));
	mLength += insertspace;
	newData[mLength] = 0;
	if (mDynamic)
		delete[] mData;
	mData = newData;
	mSize = newSize;
	mDynamic = true;
}

TEMPLATE void STRING::setLength(int newLength)
{
	expand(newLength);
	for (int i = (int)mLength; i <= newLength; ++i)
		mData[i] = 0;
	mLength = newLength;
}

TEMPLATE void STRING::copy(const STRING& str)
{
	// Copy string
	const size_t len = std::min<size_t>(str.mLength, 0x0fffffff);	// Mostly to silence a warning in GCC build
	expand(len);
	memcpy(mData, str.mData, len * sizeof(CHAR));
	mLength = len;
	mData[mLength] = 0;
}

TEMPLATE void STRING::copy(const CHAR* str)
{
	// Copy string
	if (str == nullptr)
	{
		clear();
		return;
	}

	size_t len = 0;
	while (str[len])
		++len;

	expand(len);
	memcpy(mData, str, len * sizeof(CHAR));
	mLength = len;
	mData[mLength] = 0;
}

TEMPLATE void STRING::copy(const std::basic_string<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>>& str)
{
	// Copy string
	expand((int)str.length());
	memcpy(mData, &str[0], str.length() * sizeof(CHAR));
	mLength = (int)str.length();
	mData[mLength] = 0;
}

TEMPLATE void STRING::copy(const std::basic_string_view<CHAR>& str)
{
	// Copy string
	expand((int)str.length());
	memcpy(mData, &str[0], str.length() * sizeof(CHAR));
	mLength = (int)str.length();
	mData[mLength] = 0;
}

TEMPLATE void STRING::swap(STRING& other)
{
	// Swap strings
	std::swap(mData,    other.mData);
	std::swap(mLength,  other.mLength);
	std::swap(mSize,    other.mSize);
	std::swap(mDynamic, other.mDynamic);
}

TEMPLATE void STRING::add(const STRING& str)
{
	// Add another string
	expand(mLength + str.mLength);
	memcpy(&mData[mLength], str.mData, str.mLength * sizeof(CHAR));
	mLength += str.mLength;
	mData[mLength] = 0;
}

TEMPLATE void STRING::add(const STRING& str, int pos, int len)
{
	// Add part of another string
	if (len < 0 || pos+len > str.mLength)
		len = str.mLength - pos;
	expand(mLength + len);
	memcpy(&mData[mLength], &str.mData[pos], len * sizeof(CHAR));
	mLength += len;
	mData[mLength] = 0;
}

TEMPLATE void STRING::add(CHAR ch)
{
	// Add a single character
	expand(mLength + 1);
	mData[mLength] = ch;
	++mLength;
	mData[mLength] = 0;
}

TEMPLATE void STRING::add(CHAR ch, int count)
{
	// Add a multiple characters
	if (count <= 0)
		return;
	expand(mLength + count);
	for (int i = 0; i < count; ++i)
		mData[mLength+i] = ch;
	mLength += count;
	mData[mLength] = 0;
}

TEMPLATE void STRING::add(const CHAR* ch, int count)
{
	// Add a multiple characters
	if (count <= 0)
		return;
	expand(mLength + count);
	for (int i = 0; i < count; ++i)
		mData[mLength+i] = ch[i];
	mLength += count;
	mData[mLength] = 0;
}

TEMPLATE void STRING::addInt(int value)
{
	// Conversion: int -> String
	CHAR cstr[16];
	sprintf(cstr, 16, _format_i<CHAR>(), value);
	STRING string(cstr);
	add(string);
}

TEMPLATE void STRING::addInt(unsigned int value)
{
	// Conversion: unsigned int -> String
	CHAR cstr[16];
	sprintf(cstr, 16, _format_i<CHAR>(), value);
	STRING string(cstr);
	add(string);
}

TEMPLATE void STRING::addHex(unsigned int value, int fillzeroes)
{
	// Conversion: unsigned int -> String
	CHAR cstr[16];
	sprintf(cstr, 16, _format_x<CHAR>(), value);
	STRING string(cstr);
	add('0', fillzeroes - (int)string.mLength);
	add(string);
}

TEMPLATE void STRING::addHex(unsigned int value)
{
	// Conversion: unsigned int -> String
	CHAR cstr[16];
	sprintf(cstr, 16, _format_x<CHAR>(), value);
	STRING string(cstr);
	add(string);
}

TEMPLATE void STRING::addFloat(float value, int precision)
{
	// Conversion: float -> String
	CHAR format[8];
	if (precision <= 0)
	{
		strCpy(format, 8, _format_f<CHAR>());
	}
	else
	{
		strCpy(format, 8, _format_0f<CHAR>());
		format[2] += std::min(precision, 9);
	}
	CHAR cstr[32];
	sprintf(cstr, 32, format, value);
	STRING string(cstr);
	add(string);
}

TEMPLATE void STRING::addDouble(double value, int precision)
{
	// Conversion: double -> String
	CHAR format[5];
	if (precision <= 0)
	{
		strCpy(format, 8, _format_f<CHAR>());
	}
	else
	{
		strCpy(format, 8, _format_0f<CHAR>());
		format[2] += std::min(precision, 9);
	}
	CHAR cstr[32];
	sprintf(cstr, 32, format, value);
	STRING string(cstr);
	add(string);
}

TEMPLATE void STRING::addData(void* inputdata, int bytes)
{
	// Conversion: data -> String (as hex values)
	for (int i = 0; i < bytes; ++i)
		addHex(((uint8*)inputdata)[i], 2);
}

TEMPLATE int STRING::parseInt() const
{
	return strTol(mData);
}

TEMPLATE float STRING::parseFloat() const
{
	return (float)strTod(mData);
}

TEMPLATE double STRING::parseDouble() const
{
	return strTod(mData);
}

TEMPLATE int STRING::parseData(void* dst) const
{
	// Read data encoded as hex values in string
	for (int i = 0; i < mLength/2; ++i)
	{
		int value = 0;
		for (int j = 0; j < 2; ++j)
		{
			value *= 0x10;
			CHAR ch = mData[i*2+j];
			if (ch >= '0' && ch <= '9')
				value += ch - '0';
			else if (ch >= 'a' && ch <= 'f')
				value += ch - 'a' + 10;
			else if (ch >= 'A' && ch <= 'F')
				value += ch - 'A' + 10;
			else
				return i;
		}
		((uint8*)dst)[i] = value;
	}
	return mLength/2;
}

TEMPLATE bool STRING::equal(const STRING& str) const
{
	// Check for exact equality
	if (mLength != str.mLength)
		return false;
	for (size_t i = 0; i < mLength; ++i)
		if (mData[i] != str.mData[i])
			return false;
	return true;
}

TEMPLATE int STRING::compare(const STRING& str) const
{
	// Compare two strings
	for (size_t i = 0; i <= mLength; ++i)
	{
		if (mData[i] < str.mData[i])
			return -1;
		if (mData[i] > str.mData[i])
			return +1;
	}
	return 0;
}

TEMPLATE int STRING::countChar(CHAR ch) const
{
	// Count occurences of a character
	int count = 0;
	for (int i = 0; mData[i]; ++i)
		if (mData[i] == ch)
			++count;
	return count;
}

TEMPLATE int STRING::findChar(CHAR ch, int pos, int dir) const
{
	// Find next occurence of a character
	dir = (dir < 0) ? -1 : +1;
	while (pos >= 0 && pos < (int)mLength)
	{
		if (mData[pos] == ch)
			return pos;
		pos += dir;
	}
	if (pos >= (int)mLength)
		return (int)mLength;
	return -1;
}

TEMPLATE int STRING::skipChar(CHAR ch, int pos, int dir) const
{
	// Skip occurences of a character
	dir = (dir < 0) ? -1 : +1;
	while (pos >= 0 && pos < (int)mLength)
	{
		if (mData[pos] != ch)
			return pos;
		pos += dir;
	}
	if (pos >= (int)mLength)
		return (int)mLength;
	return -1;
}

TEMPLATE int STRING::findChars(const CHAR* chars, int pos, int dir) const
{
	// Find next occurence of any of multiple characters
	int cnt = 0;
	while (chars && chars[cnt])
		++cnt;
	dir = (dir < 0) ? -1 : +1;
	while (pos >= 0 && pos < (int)mLength)
	{
		for (int i = 0; i < cnt; ++i)
			if (mData[pos] == chars[i])
				return pos;
		pos += dir;
	}
	if (pos >= (int)mLength)
		return (int)mLength;
	return -1;
}

TEMPLATE int STRING::skipChars(const CHAR* chars, int pos, int dir) const
{
	// Skip next occurences of any of multiple characters
	int cnt = 0;
	while (chars && chars[cnt])
		++cnt;
	dir = (dir < 0) ? -1 : +1;
	while (pos >= 0 && pos < (int)mLength)
	{
		bool found = false;
		for (int i = 0; i < cnt; ++i)
			if (mData[pos] == chars[i])
			{
				found = true;
				break;
			}
		if (!found)
			return pos;
		pos += dir;
	}
	if (pos >= (int)mLength)
		return (int)mLength;
	return -1;
}

TEMPLATE int STRING::findString(const STRING& str, int pos, int dir) const
{
	// Find next occurence of a substring
	dir = (dir < 0) ? -1 : +1;
	while (pos >= 0 && pos <= (int)(mLength - str.mLength))
	{
		if (mData[pos] == str.mData[0])
		{
			bool found = true;
			for (size_t i = 1; i < str.mLength; ++i)
			{
				if (mData[pos+i] != str.mData[i])
				{
					found = false;
					break;
				}
			}
			if (found)
				return pos;
		}
		pos += dir;
	}
	return -1;
}

TEMPLATE void STRING::replace(CHAR ch, CHAR newch)
{
	// Replace all occurences of a character with another
	for (size_t i = 0; i < mLength; ++i)
	{
		if (mData[i] == ch)
			mData[i] = newch;
	}
}

TEMPLATE void STRING::replace(const STRING& toreplace, const STRING& replacement)
{
	int pos = 0;
	while ((pos = findString(toreplace, pos)) >= 0)
	{
		remove(pos, (int)toreplace.mLength);
		insert(replacement, pos);
		pos += (int)replacement.mLength;
	}
}

TEMPLATE bool STRING::includes(const CHAR* str) const
{
	int pos;
	return includes(str, pos);
}

TEMPLATE bool STRING::includes(const CHAR* str, int& pos) const
{
	// Includes the other string?
	for (size_t i = 0; i < mLength; ++i)
	{
		bool equal = true;
		for (int j = 0; str[j]; ++j)
		{
			if (mData[i+j] != str[j])
			{
				equal = false;
				break;
			}
		}
		if (!equal)
			continue;
		pos = (int)i;
		return true;
	}
	return false;
}

TEMPLATE bool STRING::includesAt(const CHAR* str, int pos) const
{
	// Includes the other string at the given position?
	if (pos < 0)
		return false;
	for (int i = 0; str[i]; ++i)
	{
		if (i+pos >= (int)mLength)
			return false;
		if (str[i] != mData[i+pos])
			return false;
	}
	return true;
}

TEMPLATE bool STRING::startsWith(const CHAR* str) const
{
	return includesAt(str, 0);
}

TEMPLATE bool STRING::endsWith(const CHAR* str) const
{
	int len = 0;
	while (str[len])
		++len;
	return includesAt(str, (int)mLength - len);
}

TEMPLATE void STRING::makeSubString(int pos, int len)
{
	// Make this a substring of itself
	if (pos < 0 || pos >= (int)mLength)
	{
		clear();
		return;
	}
	if (len < 0 || pos+len > (int)mLength)
		len = (int)mLength - pos;
	CHAR* newData = new CHAR[len+1];
	memcpy(newData, &mData[pos], len * sizeof(CHAR));
	newData[len] = 0;
	if (mDynamic)
		delete[] mData;
	mData = newData;
	mSize = (size_t)(len+1);
	mLength = (size_t)len;
	mDynamic = true;
}

TEMPLATE void STRING::makeSubString(const STRING& str, int pos, int len)
{
	// Make this a substring of a different string
	if (pos < 0 || pos >= (int)str.mLength)
	{
		clear();
		return;
	}
	if (len < 0 || pos + len > (int)str.mLength)
		len = (int)str.mLength - pos;
	len = std::min(len, 0x0fffffff);	// Mostly to silence a warning in GCC build
	expand(len);
	memcpy(mData, &str.mData[pos], (size_t)(len * sizeof(CHAR)));
	mData[len] = 0;
	mLength = (size_t)len;
}

TEMPLATE void STRING::makeSubString(const STRING& str, int pos)
{
	makeSubString(str, pos, str.mLength - pos);
}

TEMPLATE CLASS STRING::getSubString(int pos, int len) const
{
	if (pos < 0 || pos >= (int)mLength)
		return CLASS();
	if (len < 0 || pos + len > (int)mLength)
		len = (int)mLength - pos;
	return CLASS(&mData[pos], len);
}

TEMPLATE int STRING::split(CLASS** str_ptr, CHAR separator) const
{
	// Split string
	int count = countChar(separator) + 1;
	STRING* str = new STRING[count];
	int num = 0;
	int start = 0;
	for (int i = 0; mData[i]; ++i)
	{
		if (mData[i] == separator)
		{
			str[num].makeSubString(*this, start, i - start);
			++num;
			start = i+1;
		}
	}
	str[num].makeSubString(*this, start, mLength - start);
	*str_ptr = str;
	return count;
}


TEMPLATE void STRING::split(std::vector<CLASS>& output, CHAR separator) const
{
	// Split string by separating character
	int count = countChar(separator) + 1;
	output.resize(count);
	int k = 0;
	int start = 0;
	for (int i = 0; mData[i]; ++i)
	{
		if (mData[i] == separator)
		{
			output[k].makeSubString(*this, start, i - start);
			++k;
			start = i+1;
		}
	}
	output[k].makeSubString(*this, start, (int)(mLength - start));
}

TEMPLATE void STRING::compose(const std::vector<STRING>& parts, const STRING& separator)
{
	// Compose string from parts
	clear();
	int len = parts.empty() ? 0 : (((int)parts.size()-1) * separator.length());
	for (const STRING& part : parts)
	{
		len += part.length();
	}
	expand(len);

	bool isFirst = true;
	for (const STRING& part : parts)
	{
		if (isFirst)
			isFirst = false;
		else
			add(separator);
		add(part);
	}
}

TEMPLATE void STRING::overwrite(const STRING& str, int pos)
{
	// Overwrite parts of a string
	if (pos < 0)
		pos = 0;
	if (pos >= mLength)
	{
		add(str);
		return;
	}
	if (pos + str.mLength > mLength)
	{
		expand(pos + str.mLength);
		mLength = pos + str.mLength;
	}
	memcpy(&mData[pos], str.mData, str.mLength * sizeof(CHAR));
	mData[mLength] = 0;
}

TEMPLATE void STRING::insert(const STRING& str, int pos)
{
	// Insert another string without overwriting
	if (pos < 0)
		pos = 0;
	if (pos >= (int)mLength)
	{
		add(str);
		return;
	}
	expand(mLength + str.mLength);
	memmove(&mData[pos+str.mLength], &mData[pos], (mLength-pos) * sizeof(CHAR));
	memcpy(&mData[pos], str.mData, str.mLength * sizeof(CHAR));
	mLength += str.mLength;
	mData[mLength] = 0;
}

TEMPLATE void STRING::remove(int pos, int len)
{
	// Remove parts of the string
	if (pos < 0)
		pos = 0;
	if (pos >= (int)mLength)
		return;
	if (len < 0 || pos+len > (int)mLength)
		len = (int)mLength - pos;
	expand(mLength);			// In case string was constant before
	memmove(&mData[pos], &mData[pos+len], (mLength-pos-len) * sizeof(CHAR));
	mLength -= len;
	mData[mLength] = 0;
}

TEMPLATE void STRING::fillLeft(CHAR ch, int len)
{
	// Fill in characters at the start
	if (len <= (int)mLength)
		return;
	const int diff = len - (int)mLength;
	expand(len, diff);
	for (int i = 0; i < diff; ++i)
		mData[i] = ch;
	mData[mLength] = 0;
}

TEMPLATE void STRING::fillRight(CHAR ch, int len)
{
	// Fill in characters at the end
	if (len <= mLength)
		return;
	expand(len);
	for (int i = (int)mLength; i < len; ++i)
		mData[i] = ch;
	mLength = len;
	mData[mLength] = 0;
}

TEMPLATE void STRING::trimWhitespace()
{
	if (mLength == 0)
		return;

	int pos = (int)mLength - 1;
	if (mData[pos] == _space<CHAR>() || mData[pos] == _tabulator<CHAR>())
	{
		--pos;
		while (pos >= 0 && (mData[pos] == _space<CHAR>() || mData[pos] == _tabulator<CHAR>()))
			--pos;
		makeDynamic();
		mLength = pos + 1;
		mData[mLength] = 0;
	}

	if (mLength == 0)
		return;

	pos = 0;
	if (mData[pos] == _space<CHAR>() || mData[pos] == _tabulator<CHAR>())
	{
		++pos;
		while (pos < (int)mLength && (mData[pos] == _space<CHAR>() || mData[pos] == _tabulator<CHAR>()))
			++pos;
		makeSubString(pos, (int)mLength - pos);
	}
}

TEMPLATE void STRING::lowerCase()
{
	expand(mLength);
	for (size_t i = 0; i < mLength; ++i)
	{
		if (mData[i] >= 'A' && mData[i] <= 'Z')
			mData[i] += 32;
	}
}

TEMPLATE void STRING::upperCase()
{
	expand(mLength);
	for (size_t i = 0; i < mLength; ++i)
	{
		if (mData[i] >= 'a' && mData[i] <= 'z')
			mData[i] -= 32;
	}
}

TEMPLATE int STRING::getLine(STRING& output, int pos) const
{
	// Read one line
	if (pos < 0)
		pos = 0;
	const int start = pos;
	while (pos < (int)mLength && mData[pos] != 13 && mData[pos] != 10)
		++pos;
	output.makeSubString(*this, start, pos-start);
	if (pos >= (int)mLength-1)
		return (int)mLength;
	if (mData[pos] == 13 && mData[pos+1] == 10)
		return pos+2;
	return pos+1;
}

TEMPLATE int STRING::getLine(size_t& length, int pos) const
{
	// Read one line
	if (pos < 0)
		pos = 0;
	const int start = pos;
	while (pos < (int)mLength && mData[pos] != 13 && mData[pos] != 10)
		++pos;
	length = pos-start;
	if (pos >= (int)mLength-1)
		return (int)mLength;
	if (mData[pos] == 13 && mData[pos+1] == 10)
		return pos+2;
	return pos+1;
}

TEMPLATE void STRING::format(const CHAR* format, ...)
{
	va_list argv;
	va_start(argv, format);
	static CHAR buffer[1024];
	rmx::StringTraits<CHAR>::buildFormatted(buffer, 1024, format, argv);
	va_end(argv);
	copy(buffer);
}

TEMPLATE uint8* STRING::extractData(size_t& datasize)
{
	makeDynamic();
	uint8* data = (uint8*)mData;
	datasize = mLength + 1;
	mData = const_cast<CHAR*>(_empty<CHAR>());
	mLength = 0;
	mSize = 0;
	mDynamic = false;
	return data;
}

TEMPLATE bool STRING::readUnicode(const uint8* data, size_t datasize, UnicodeEncoding encoding)
{
	static const String ByteOrderMark_UTF8("\xef\xbb\xbf");
	static const String ByteOrderMark_UTF16LE("\xff\xfe");
	static const String ByteOrderMark_UTF16BE("\xfe\xff");
	static const String ByteOrderMark_UTF32LE("\xff\xfe\x00\x00");
	static const String ByteOrderMark_UTF32BE("\x00\x00\xfe\xff");

	clear();
	if (datasize == 0)
		return true;

	if (encoding == UnicodeEncoding::AUTO)
	{
		MemInputStream blob(data, datasize);

		#define IF_ENCODING(x) \
			if (blob.tryRead(*ByteOrderMark_##x, ByteOrderMark_##x.length())) encoding = UnicodeEncoding::x;

		IF_ENCODING(UTF8)    else
		IF_ENCODING(UTF32LE) else		// Check UTF-32 before UTF-16
//		IF_ENCODING(UTF32BE) else	// TODO: This is buggy
		IF_ENCODING(UTF16LE) else
		IF_ENCODING(UTF16BE) else encoding = UnicodeEncoding::ASCII;

		#undef IF_ENCODING
	}

	#define ABORT { clear(); return false; }

	MemInputStream blob(data, datasize);
	uint32 code;

	if (encoding == UnicodeEncoding::ASCII)
	{
		// ASCII
		size_t len = blob.getRemaining();
		expand(len);
		mLength = (int)len;
		for (size_t pos = 0; pos < len; ++pos)
			mData[pos] = (CHAR)(blob.read<uint8>());
	}
	else if (encoding == UnicodeEncoding::UTF8)
	{
		// UTF-8
		blob.tryRead(*ByteOrderMark_UTF8, ByteOrderMark_UTF8.length());
		int remaining = (int)blob.getRemaining();
		expand(remaining);

		// TODO: Use "WString::readUTF8"
		uint8 values[4];
		while (remaining > 0)
		{
			blob >> values[0];
			--remaining;

			if (values[0] <= 0x7f)
			{
				mData[mLength] = (CHAR)values[0];
				++mLength;
			}
			else if (values[0] <= 0xbf)
			{
				ABORT;		// No valid first byte of a sequence
			}
			else if (values[0] <= 0xc1)
			{
				ABORT;		// Forbidden alternative encoding for 0x00 to 0x7f
			}
			else if (values[0] <= 0xf4)
			{
				int seqLength = (values[0] <= 0xdf) ? 2 : (values[0] <= 0xef) ? 3 : 4;
				remaining -= (seqLength - 1);
				if (remaining < 0)
					ABORT;

				for (int k = 1; k < seqLength; ++k)
				{
					blob >> values[k];
					if (values[k] < 0x80 || values[k] >= 0xc0)
						ABORT;		// Additional bytes of a sequence always begin with bits 10xxxxxx
					values[k] &= 0x3f;
				}

				if (seqLength == 2)
					code = ((uint32)(values[0] & 0x1f) << 6) + ((uint32)values[1]);
				else if (seqLength == 3)
					code = ((uint32)(values[0] & 0x0f) << 12) + ((uint32)values[1] << 6) + ((uint32)values[2]);
				else
					code = ((uint32)(values[0] & 0x07) << 18) + ((uint32)values[1] << 12) + ((uint32)values[2] << 6) + ((uint32)values[3]);

				mData[mLength] = rmx::StringTraits<CHAR>::fromUnicode(code);
				++mLength;
			}
			else
			{
				ABORT;		// Result would be a code beyond the valid unicode region (i.e. something from 0x140000 on)
			}
		}
	}
	else if (encoding == UnicodeEncoding::UTF16LE || encoding == UnicodeEncoding::UTF16BE)
	{
		// UTF-16
		const String& bom = (encoding == UnicodeEncoding::UTF16LE) ? ByteOrderMark_UTF16LE : ByteOrderMark_UTF16BE;
		blob.tryRead(*bom, bom.length());
		size_t remaining = blob.getRemaining() / 2;
		expand(remaining);

		uint16 value;
		while (remaining > 0)
		{
			blob >> value;
			if (encoding == UnicodeEncoding::UTF16BE)
				value = swapBytes16(value);
			--remaining;

			if (value <= 0xd7ff || value >= 0xe000)
			{
				code = value;
			}
			else
			{
				if (remaining < 1)
					ABORT;

				uint16 second;
				blob >> second;
				if (encoding == UnicodeEncoding::UTF16BE)
					second = swapBytes16(second);
				--remaining;
				code = 0x10000 + ((value - 0xd800) << 10) + (second - 0xdc00);
			}

			mData[mLength] = rmx::StringTraits<CHAR>::fromUnicode(code);
			++mLength;
		}
	}
	else if (encoding == UnicodeEncoding::UTF32LE || encoding == UnicodeEncoding::UTF32BE)
	{
		// UTF-32
		const String& bom = (encoding == UnicodeEncoding::UTF32LE) ? ByteOrderMark_UTF32LE : ByteOrderMark_UTF32BE;
		blob.tryRead(*bom, bom.length());
		size_t len = blob.getRemaining() / 4;
		expand(len);
		mLength = (int)len;

		if (encoding == UnicodeEncoding::UTF32LE)
		{
			for (size_t pos = 0; pos < len; ++pos)
			{
				blob >> code;
				mData[pos] = rmx::StringTraits<CHAR>::fromUnicode(code);
			}
		}
		else
		{
			for (size_t pos = 0; pos < len; ++pos)
			{
				blob >> code;
				mData[pos] = rmx::StringTraits<CHAR>::fromUnicode(swapBytes32(code));
			}
		}
	}

	#undef ABORT

	mData[mLength] = 0;
	return true;
}

TEMPLATE void STRING::writeUnicode(std::vector<uint8>& buffer, UnicodeEncoding encoding, bool addBOM) const
{
	static const String ByteOrderMark_UTF8("\xef\xbb\xbf");
	static const String ByteOrderMark_UTF16LE("\xff\xfe");
	static const String ByteOrderMark_UTF16BE("\xfe\xff");
	static const String ByteOrderMark_UTF32LE("\xff\xfe\x00\x00");
	static const String ByteOrderMark_UTF32BE("\x00\x00\xfe\xff");

	if (encoding == UnicodeEncoding::AUTO)
	{
		if (sizeof(CHAR) == 1)
			encoding = UnicodeEncoding::ASCII;
		else
			encoding = UnicodeEncoding::UTF8;
	}

	if (encoding == UnicodeEncoding::ASCII)
	{
		// ASCII
		buffer.resize((size_t)mLength);
		for (size_t i = 0; i < (size_t)mLength; ++i)
			buffer[i] = ((uint32)mData[i] <= 0xff) ? (uint8)mData[i] : 127;
	}
	else if (encoding == UnicodeEncoding::UTF8)
	{
		// UTF-8
		const size_t size4bom = addBOM ? ByteOrderMark_UTF8.length() : 0;
		size_t size = size4bom;
		for (size_t i = 0; i < (size_t)mLength; ++i)
		{
			uint32 code = rmx::StringTraits<CHAR>::toUnicode(mData[i]);
			if (code <= 0x7f)
				++size;
			else if (code <= 0x7ff)
				size += 2;
			else if (code <= 0xffff)
				size += 3;
			else
				size += 4;
		}

		buffer.resize(size);
		uint8* ptr = &buffer[0];
		if (addBOM)
		{
			memcpy(ptr, *ByteOrderMark_UTF8, size4bom);
			ptr += size4bom;
		}

		for (size_t i = 0; i < (size_t)mLength; ++i)
		{
			uint32 code = rmx::StringTraits<CHAR>::toUnicode(mData[i]);
			if (code <= 0x7f)
			{
				ptr[0] = (uint8)code;
				++ptr;
			}
			else if (code <= 0x7ff)
			{
				ptr[0] = 0xc0 + (code >> 6);
				ptr[1] = 0x80 + (code & 0x3f);
				ptr += 2;
			}
			else if (code <= 0xffff)
			{
				ptr[0] = 0xe0 + (code >> 12);
				ptr[1] = 0x80 + ((code >> 6) & 0x3f);
				ptr[2] = 0x80 + (code & 0x3f);
				ptr += 3;
			}
			else
			{
				ptr[0] = 0xf0 + (code >> 18);
				ptr[1] = 0x80 + ((code >> 12) & 0x3f);
				ptr[2] = 0x80 + ((code >> 6) & 0x3f);
				ptr[3] = 0x80 + (code & 0x3f);
				ptr += 4;
			}
		}
		assert(ptr == &buffer[0] + size);
	}
	else if (encoding == UnicodeEncoding::UTF16LE || encoding == UnicodeEncoding::UTF16BE)
	{
		// UTF-16
		const String& bom = (encoding == UnicodeEncoding::UTF16LE) ? ByteOrderMark_UTF16LE : ByteOrderMark_UTF16BE;
		const size_t size4bom = addBOM ? bom.length() : 0;
		size_t size = size4bom;
		for (size_t i = 0; i < (size_t)mLength; ++i)
		{
			if (rmx::StringTraits<CHAR>::toUnicode(mData[i]) <= 0xffff)
				size += 2;
			else
				size += 4;
		}

		buffer.resize(size);
		if (addBOM)
			memcpy(&buffer[0], *bom, bom.length());
		uint16* start = (uint16*)&buffer[0] + size4bom;
		uint16* end = (uint16*)&buffer[0] +  size;

		uint16* ptr = start;
		for (size_t i = 0; i < (size_t)mLength; ++i)
		{
			uint32 code = rmx::StringTraits<CHAR>::toUnicode(mData[i]);
			if (code <= 0xffff)
			{
				ptr[0] = (uint16)code;
				++ptr;
			}
			else
			{
				code -= 0x10000;
				ptr[0] = 0xd800 + ((code >> 10) & 0x3ff);
				ptr[1] = 0xdc00 + (code & 0x3ff);
				ptr += 2;
			}
		}
		assert(ptr == end);

		if (encoding == UnicodeEncoding::UTF16BE)
		{
			for (ptr = start; ptr < end; ++ptr)
				*ptr = swapBytes16(*ptr);
		}
	}
	else if (encoding == UnicodeEncoding::UTF32LE || encoding == UnicodeEncoding::UTF32BE)
	{
		// UTF-32
		const String& bom = (encoding == UnicodeEncoding::UTF32LE) ? ByteOrderMark_UTF32LE : ByteOrderMark_UTF32BE;
		const size_t size4bom = addBOM ? bom.length() : 0;
		size_t size = size4bom + mLength * 4;

		buffer.resize(size);
		if (addBOM)
			memcpy(&buffer[0], *bom, bom.length());

		uint32* ptr = (uint32*)&buffer[size4bom];
		if (encoding == UnicodeEncoding::UTF32LE)
		{
			for (size_t i = 0; i < (size_t)mLength; ++i)
				ptr[i] = rmx::StringTraits<CHAR>::toUnicode(mData[i]);
		}
		else
		{
			for (size_t i = 0; i < (size_t)mLength; ++i)
				ptr[i] = swapBytes32(rmx::StringTraits<CHAR>::toUnicode(mData[i]));
		}
	}
	else
	{
		buffer.clear();
	}
}

TEMPLATE bool STRING::loadFile(const std::string& filename, UnicodeEncoding encoding)
{
	// Load contents of a text file into this string
	clear();
	std::vector<uint8> buffer;
	if (!FTX::FileSystem->readFile(filename, buffer))
		return false;

	if (buffer.empty())
	{
		clear();
		return true;
	}
	return readUnicode(&buffer[0], buffer.size(), encoding);
}

TEMPLATE bool STRING::loadFile(const std::wstring& filename, UnicodeEncoding encoding)
{
	// Load contents of a text file into this string
	clear();
	std::vector<uint8> buffer;
	if (!FTX::FileSystem->readFile(filename, buffer))
		return false;

	if (buffer.empty())
	{
		clear();
		return true;
	}
	return readUnicode(&buffer[0], buffer.size(), encoding);
}

TEMPLATE bool STRING::saveFile(const std::string& filename, UnicodeEncoding encoding) const
{
	// Save contents of string into a text file
	std::vector<uint8> buffer;
	writeUnicode(buffer, encoding, true);
	return FTX::FileSystem->saveFile(filename, buffer);
}

TEMPLATE bool STRING::saveFile(const std::wstring& filename, UnicodeEncoding encoding) const
{
	// Save contents of string into a text file
	std::vector<uint8> buffer;
	writeUnicode(buffer, encoding, true);
	return FTX::FileSystem->saveFile(filename, buffer);
}


#undef TEMPLATE
#undef STRING
