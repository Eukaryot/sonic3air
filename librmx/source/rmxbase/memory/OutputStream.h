/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class OutputStream;

namespace StreamIO
{
	template<typename T> static void write(OutputStream* blob, const T& value);
}


// Base class for output streams
class API_EXPORT OutputStream
{
public:
	virtual void setPosition(int pos) = 0;
	virtual int  getPosition() const = 0;
	virtual void rewind()  { setPosition(0); }

	virtual int  write(const void* ptr, int len) = 0;

	template<typename T> OutputStream& operator<<(const T& value)
	{
		StreamIO::write(this, value);
		return *this;
	}
};


// Specialized output functions for certain data types
namespace StreamIO
{
	template<typename T> static void write(OutputStream* blob, const T& value)
	{
		blob->write(&value, sizeof(T));
	}

	template<bool> static void write(OutputStream* blob, const bool& value)
	{
		blob->write(&value, 1);
	}

	template<typename CHAR> static void write(OutputStream* blob, const std::basic_string<CHAR>& value)
	{
		blob->write(&value[0], (value.size()+1) * sizeof(CHAR));
	}

	template<typename CHAR, typename CLASS> static void write(OutputStream* blob, const StringTemplate<CHAR, CLASS>& value)
	{
		blob->write(value.getData(), (value.length()+1) * sizeof(CHAR));
	}
}


// Output to memory of a fixed size
class API_EXPORT MemOutputStream : public OutputStream
{
public:
	MemOutputStream(int size);
	~MemOutputStream();

	void setPosition(int pos)	{ mCursor = mBuffer + pos; assert(mCursor <= mBufferEnd); }
	int getPosition() const		{ return (int)(mCursor - mBuffer); }
	int getCapacity() const		{ return (int)(mBufferEnd - mBuffer); }
	uint8* getBuffer()			{ return mBuffer; }

	int  write(const void* ptr, int len);
	bool saveTo(OutputStream& stream);
	bool saveToFile(const String& filename);

protected:
	uint8* mBuffer = nullptr;
	uint8* mBufferEnd = nullptr;
	uint8* mCursor = nullptr;
};


// Output to dynamically growing memory
class API_EXPORT DynOutputStream : public OutputStream
{
public:
	DynOutputStream();
	~DynOutputStream();

	void clear();

	void setPosition(int pos);
	int getPosition() const;
	int getCapacity() const  { return std::numeric_limits<int>::max(); }

	int write(const void* ptr, int len);
	bool saveTo(OutputStream& stream);

protected:
	void AccessPage(int pageIndex);

protected:
	std::vector<uint8*> mPages;
	int mPageSize = 1024;
	int mCurrPage = 0;
	uint8* mCursor = nullptr;
	uint8* mPageEnd = nullptr;
};
