/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Flags for file access
enum FileAccessFlags
{
	FILE_ACCESS_READ      = 0x0000,
	FILE_ACCESS_WRITE     = 0x0001,
	FILE_ACCESS_APPEND    = 0x0002,
	FILE_ACCESS_READWRITE = 0x0003,
	FILE_ACCESS_TEXT      = 0x0010
};


class API_EXPORT FileHandle
{
public:
	FileHandle();
	FileHandle(const String& filename, uint32 flags = FILE_ACCESS_READ);
	FileHandle(const WString& filename, uint32 flags = FILE_ACCESS_READ);
	~FileHandle();

	bool open(const String& filename, uint32 flags = FILE_ACCESS_READ);
	bool open(const WString& filename, uint32 flags = FILE_ACCESS_READ);
	void close();

	bool isOpen() const  { return (nullptr != mFile); }
	int64 getSize() const;

	void seek(int64 position);
	int64 tell() const;

	size_t read(void* output, size_t bytes) const;
	size_t write(const void* input, size_t bytes);
	void flush();

private:
	FILE* mFile = nullptr;
	WString mFilename;
	mutable int64 mFileSize = 0;
};
