/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class InputStream;

namespace StreamIO
{
	template<typename T> static void read(InputStream* blob, T& value);
}


// Base class for input streams
class API_EXPORT InputStream
{
public:
	enum class StreamingState
	{
		STREAMING,		// Next read will likely return more data
		BLOCKED,		// Reading is currently blocked, try again later
		COMPLETED		// Stream is read completely, reads will not return any more data
	};

public:
	virtual ~InputStream() {}

	virtual bool valid() const = 0;
	virtual void close() = 0;
	virtual const char* getType() const = 0;

	virtual void setPosition(size_t pos) = 0;
	virtual size_t getPosition() const = 0;
	virtual size_t getSize() const = 0;
	virtual	size_t getRemaining() const  { return getSize() - getPosition(); }

	virtual size_t read(void* dst, size_t len) = 0;
	virtual void skip(size_t len) = 0;
	virtual bool tryRead(const void* data, size_t len) = 0;
	virtual StreamingState getStreamingState() = 0;
	virtual void rewind()  { setPosition(0); }

	template<typename T> T read()
	{
		T t;
		read(&t, sizeof(T));
		return t;
	}

	template<typename T> InputStream& operator>>(T& value)
	{
		StreamIO::read(this, value);
		return *this;
	}
};


// Specialized input functions for certain data types
namespace StreamIO
{
	template<typename T> static void read(InputStream* blob, T& value)
	{
		blob->read(&value, sizeof(T));
	}

	template<bool> static void read(InputStream* blob, bool& value)
	{
		blob->read(&value, 1);
	}

	template<typename CHAR> static void read(InputStream* blob, std::basic_string<CHAR>& value)
	{
		CHAR buf[1024];
		int len = 0;
		for (; len < 1024; ++len)
		{
			blob->read(&buf[len], sizeof(CHAR));
			if (buf[len] == 0)
				break;
		}
		value = buf;
	}

	template<typename CHAR, typename CLASS> static void read(InputStream* blob, StringTemplate<CHAR, CLASS>& value)
	{
		CHAR buf[1024];
		int len = 0;
		for (; len < 1024; ++len)
		{
			blob->read(&buf[len], sizeof(CHAR));
			if (buf[len] == 0)
				break;
		}
		value = buf;
	}
}


// Input from fixed memory
class API_EXPORT MemInputStream : public InputStream
{
public:
	MemInputStream(const void* data, size_t size, bool autoDelete = false);
	MemInputStream(InputStream& input);
	~MemInputStream();

	bool valid() const override { return (nullptr != mBuffer); }
	void close() override;
	const char* getType() const override { return "mem"; }

	void setPosition(size_t pos) override	{ mCursor = mBuffer + pos; assert(mCursor <= mBufferEnd); }
	size_t getPosition() const override  { return (size_t)(mCursor - mBuffer); }
	size_t getSize() const override  { return (size_t)(mBufferEnd - mBuffer); }
	size_t getRemaining() const override  { return (size_t)(mBufferEnd - mCursor); }

	using InputStream::read;
	size_t read(void* dst, size_t len) override;
	void skip(size_t len) override;
	bool tryRead(const void* data, size_t len) override;
	StreamingState getStreamingState() override;

	const uint8* getCursor()  { return mCursor; }

protected:
	const uint8* mBuffer = nullptr;
	const uint8* mBufferEnd = nullptr;
	const uint8* mCursor = nullptr;
	bool mAutoDelete = false;
};


// Input from a file
class API_EXPORT FileInputStream : public InputStream
{
public:
	FileInputStream() {}
	FileInputStream(const String& filename);
	FileInputStream(const WString& filename);
	~FileInputStream();

	bool open(const String& filename);
	bool open(const WString& filename);

	const char* getType() const override { return "file"; }
	bool valid() const override { return mFile.isOpen(); }
	void close() override;

	void  setPosition64(int64 pos);
	int64 getPosition64() const;
	int64 getSize64() const;

	void setPosition(size_t pos) override  { setPosition64((int64)pos); }
	size_t getPosition() const override  { return (size_t)getPosition64(); }
	size_t getSize() const override  { return (size_t)getSize64(); }

	using InputStream::read;
	size_t read(void* dst, size_t len) override;
	void skip(size_t len) override;
	bool tryRead(const void* data, size_t len) override;
	StreamingState getStreamingState() override;

private:
	FileHandle mFile;
	StreamingState mLastStreamingState = StreamingState::COMPLETED;
};
