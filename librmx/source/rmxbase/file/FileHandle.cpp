/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace
{
	const char* getModeString(uint32 flags)
	{
		switch (flags & 0x0f)
		{
			case FILE_ACCESS_READ:      return (flags & FILE_ACCESS_TEXT) ? "rt" : "rb";
			case FILE_ACCESS_WRITE:     return (flags & FILE_ACCESS_TEXT) ? "wt" : "wb";
			case FILE_ACCESS_APPEND:    return (flags & FILE_ACCESS_TEXT) ? "at" : "ab";
			case FILE_ACCESS_READWRITE: return (flags & FILE_ACCESS_TEXT) ? "rt+": "rb+";
			default:					return "";
		}
	}

	const wchar_t* getModeStringW(uint32 flags)
	{
		switch (flags & 0x0f)
		{
			case FILE_ACCESS_READ:      return (flags & FILE_ACCESS_TEXT) ? L"rt" : L"rb";
			case FILE_ACCESS_WRITE:     return (flags & FILE_ACCESS_TEXT) ? L"wt" : L"wb";
			case FILE_ACCESS_APPEND:    return (flags & FILE_ACCESS_TEXT) ? L"at" : L"ab";
			case FILE_ACCESS_READWRITE: return (flags & FILE_ACCESS_TEXT) ? L"rt+": L"rb+";
			default:					return L"";
		}
	}
}


FileHandle::FileHandle()
{
}

FileHandle::FileHandle(const String& filename, uint32 flags)
{
	open(filename, flags);
}

FileHandle::FileHandle(const WString& filename, uint32 flags)
{
	open(filename, flags);
}

FileHandle::~FileHandle()
{
	if (nullptr != mFile)
		close();
}

bool FileHandle::open(const String& filename, uint32 flags)
{
	if (nullptr != mFile)
		close();

#if defined(PLATFORM_WINDOWS)
	mFile = _fsopen(*filename, ::getModeString(flags), ((flags & 0x0f) == FILE_ACCESS_READ) ? _SH_DENYNO : _SH_DENYWR);		// Allow shared file reading
#elif defined(USE_UTF8_PATHS)
	mFile = fopen(*WString(filename).toUTF8(), ::getModeString(flags));
#else
	mFile = fopen(*filename, ::getModeString(flags));
#endif

	if (nullptr == mFile)
		return false;

	mFilename = filename.toWString();
	mFileSize = -1;
	return true;
}

bool FileHandle::open(const WString& filename, uint32 flags)
{
	if (nullptr != mFile)
		close();

	const bool isWrite = (flags & 0x0f) != FILE_ACCESS_READ;
	if (isWrite)
	{
		// Create directory if needed
		const size_t slashPosition = std::wstring_view(*filename).find_last_of(L"/\\");
		if (slashPosition != std::string::npos)
		{
			rmx::FileIO::createDirectory(std::wstring_view(*filename).substr(0, slashPosition));
		}
	}

#if defined(PLATFORM_WINDOWS)
	mFile = _wfsopen(*filename, ::getModeStringW(flags), isWrite ? _SH_DENYWR : _SH_DENYNO);	// Allow shared file reading
#elif defined(USE_UTF8_PATHS)
	mFile = fopen(*filename.toUTF8(), ::getModeString(flags));
#else
	mFile = fopen(*filename.toString(), ::getModeString(flags));
#endif

	if (nullptr == mFile)
		return false;

	mFilename = filename;
	mFileSize = -1;
	return true;
}

void FileHandle::close()
{
	if (nullptr == mFile)
		return;
	fclose(mFile);
	mFile = nullptr;
}

int64 FileHandle::getSize() const
{
	if (nullptr == mFile)
		return 0;

	if (mFileSize < 0)
	{
#ifdef _MSC_VER
		int64 pos = _ftelli64(mFile);
		_fseeki64(mFile, 0, SEEK_END);
		mFileSize = _ftelli64(mFile);
		_fseeki64(mFile, pos, SEEK_SET);
#else
		int pos = ftell(mFile);
		fseek(mFile, 0, SEEK_END);
		mFileSize = (int64)ftell(mFile);
		fseek(mFile, pos, SEEK_SET);
#endif
	}
	return mFileSize;
}

void FileHandle::seek(int64 position)
{
	if (nullptr == mFile)
		return;
#ifdef _MSC_VER
	_fseeki64(mFile, position, SEEK_SET);
#else
	fseek(mFile, position, SEEK_SET);
#endif
}

int64 FileHandle::tell() const
{
	if (nullptr == mFile)
		return 0;
#ifdef _MSC_VER
	return _ftelli64(mFile);
#else
	return (int64)ftell(mFile);
#endif
}

size_t FileHandle::read(void* output, size_t bytes) const
{
	if (nullptr == mFile)
		return 0;
	return fread(output, 1, bytes, mFile);
}

size_t FileHandle::write(const void* input, size_t bytes)
{
	if (nullptr == mFile)
		return 0;
	return fwrite(input, 1, bytes, mFile);
}

void FileHandle::flush()
{
	if (nullptr == mFile)
		return;
	fflush(mFile);
}
