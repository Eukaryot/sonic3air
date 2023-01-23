/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


/* ----- MemInputStream ---------------------------------------------------------------------------------- */

MemInputStream::MemInputStream(const void* data, size_t size, bool autoDelete)
{
	assert(data);
	mBuffer = mCursor = (uint8*)(data);
	mBufferEnd = mBuffer + size;
	mAutoDelete = autoDelete;
}

MemInputStream::MemInputStream(InputStream& input)
{
	if (strcmp(input.getType(), getType()) == 0)
	{
		MemInputStream& minput = (MemInputStream&)input;
		mBuffer = minput.mBuffer;
		mCursor = minput.mCursor;
		mBufferEnd = minput.mBufferEnd;
		mAutoDelete = false;
	}
	else
	{
		const size_t size = input.getRemaining();
		mBuffer = mCursor = new uint8[size];
		mBufferEnd = mBuffer + size;
		mAutoDelete = true;
		const size_t read = input.read(const_cast<uint8*>(mBuffer), size);
		assert(read == size);
	}
}

MemInputStream::~MemInputStream()
{
	close();
}

void MemInputStream::close()
{
	if (mAutoDelete)
		delete[] mBuffer;
	mBuffer = mCursor = mBufferEnd = nullptr;
}

size_t MemInputStream::read(void* dst, size_t len)
{
	if (mCursor + len > mBufferEnd)
	{
		len = (size_t)(mBufferEnd - mCursor);
		if (len <= 0)
			return 0;
	}
	memcpy(dst, mCursor, len);
	mCursor += len;
	return len;
}

void MemInputStream::skip(size_t len)
{
	mCursor += len;
}

bool MemInputStream::tryRead(const void* data, size_t len)
{
	if (mCursor + len > mBufferEnd)
		return false;
	if (memcmp(mCursor, data, len) != 0)
		return false;
	mCursor += len;
	return true;
}

InputStream::StreamingState MemInputStream::getStreamingState()
{
	return (mCursor < mBufferEnd) ? StreamingState::STREAMING : StreamingState::COMPLETED;
}


/* ----- FileInputStream --------------------------------------------------------------------------------- */

FileInputStream::FileInputStream(const String& filename)
{
	open(filename);
}

FileInputStream::FileInputStream(const WString& filename)
{
	open(filename);
}

FileInputStream::~FileInputStream()
{
	close();
}

bool FileInputStream::open(const String& filename)
{
	const bool result = mFile.open(filename, FILE_ACCESS_READ);
	mLastStreamingState = result ? StreamingState::STREAMING : StreamingState::COMPLETED;
	return result;
}

bool FileInputStream::open(const WString& filename)
{
	const bool result = mFile.open(filename, FILE_ACCESS_READ);
	mLastStreamingState = result ? StreamingState::STREAMING : StreamingState::COMPLETED;
	return result;
}

void FileInputStream::close()
{
	mFile.close();
	mLastStreamingState = StreamingState::COMPLETED;
}

int64 FileInputStream::getSize64() const
{
	return mFile.getSize();
}

int64 FileInputStream::getPosition64() const
{
	return mFile.tell();
}

void FileInputStream::setPosition64(int64 pos)
{
	mFile.seek(pos);
	mLastStreamingState = StreamingState::STREAMING;
}

size_t FileInputStream::read(void* dst, size_t len)
{
	const size_t readuint8s = mFile.read(dst, len);
	mLastStreamingState = (readuint8s == 0) ? StreamingState::COMPLETED : StreamingState::STREAMING;
	return readuint8s;
}

void FileInputStream::skip(size_t len)
{
	mFile.seek(mFile.tell() + len);
}

bool FileInputStream::tryRead(const void* data, size_t len)
{
	uint8 buffer[1024];
	uint8* buf = buffer;
	if (len > 1024)
		buf = new uint8[len];

	const size_t readuint8s = read(buf, len);
	if (readuint8s == len && memcmp(buf, data, len) == 0)
		return true;

	mFile.seek(mFile.tell() - readuint8s);
	return false;
}

InputStream::StreamingState FileInputStream::getStreamingState()
{
	StreamingState result = StreamingState::BLOCKED;
	if (mLastStreamingState == StreamingState::BLOCKED)
	{
		// Getting here means that "getStreamingState" got called multiple times in a row
		uint8 value = 0;
		size_t readuint8s = read(&value, 1);
		if (readuint8s > 0)
		{
			mFile.seek(mFile.tell() - 1);
			result = StreamingState::STREAMING;
		}
		else
		{
			result = StreamingState::COMPLETED;
		}
	}
	else
	{
		// Streaming state was set in last "read", so return its result
		result = mLastStreamingState;
	}
	mLastStreamingState = StreamingState::BLOCKED;
	return result;
}
