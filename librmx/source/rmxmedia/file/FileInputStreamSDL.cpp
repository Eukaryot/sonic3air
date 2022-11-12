/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


namespace rmx
{

	FileInputStreamSDL::FileInputStreamSDL(const String& filename)
	{
		open(filename);
	}

	FileInputStreamSDL::FileInputStreamSDL(const WString& filename)
	{
		open(filename);
	}

	FileInputStreamSDL::~FileInputStreamSDL()
	{
		close();
	}

	bool FileInputStreamSDL::open(const String& filename)
	{
		close();
		mContext = SDL_RWFromFile(*filename, "r");
		mLastStreamingState = (nullptr != mContext) ? StreamingState::STREAMING : StreamingState::COMPLETED;
		return (nullptr != mContext);
	}

	bool FileInputStreamSDL::open(const WString& filename)
	{
		close();
		mContext = SDL_RWFromFile(*filename.toUTF8(), "r");
		mLastStreamingState = (nullptr != mContext) ? StreamingState::STREAMING : StreamingState::COMPLETED;
		return (nullptr != mContext);
	}

	void FileInputStreamSDL::close()
	{
		if (nullptr != mContext)
		{
			SDL_RWclose(mContext);
			mContext = nullptr;
		}
		mLastStreamingState = StreamingState::COMPLETED;
	}

	int64 FileInputStreamSDL::getSize64() const
	{
		return (nullptr == mContext) ? 0 : std::max<int64>(0, SDL_RWsize(mContext));
	}

	int64 FileInputStreamSDL::getPosition64() const
	{
		return (nullptr == mContext) ? 0 : SDL_RWtell(mContext);
	}

	void FileInputStreamSDL::setPosition64(int64 pos)
	{
		if (nullptr == mContext)
			return;

		SDL_RWseek(mContext, pos, RW_SEEK_SET);
		mLastStreamingState = StreamingState::STREAMING;
	}

	size_t FileInputStreamSDL::read(void* dst, size_t len)
	{
		if (nullptr == mContext)
			return 0;

		const size_t readBytes = SDL_RWread(mContext, dst, 1, len);
		mLastStreamingState = (readBytes == 0) ? StreamingState::COMPLETED : StreamingState::STREAMING;
		return readBytes;
	}

	void FileInputStreamSDL::skip(size_t len)
	{
		setPosition64(getPosition64() + len);
	}

	bool FileInputStreamSDL::tryRead(const void* data, size_t len)
	{
		uint8 buffer[1024];
		uint8* buf = buffer;
		if (len > 1024)
			buf = new uint8[len];

		size_t readBytes = read(buf, len);
		if (readBytes == len && memcmp(buf, data, len) == 0)
			return true;

		setPosition64(getPosition64() - readBytes);
		return false;
	}

    InputStream::StreamingState FileInputStreamSDL::getStreamingState()
    {
		StreamingState result = StreamingState::BLOCKED;
		if (mLastStreamingState == StreamingState::BLOCKED)
		{
			// Getting here means that "getStreamingState" got called multiple times in a row
			uint8 value = 0;
			size_t readBytes = read(&value, 1);
			if (readBytes > 0)
			{
				SDL_RWseek(mContext, SDL_RWtell(mContext) - 1, RW_SEEK_SET);
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

}
