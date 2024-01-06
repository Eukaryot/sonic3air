/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


/* ----- MemOutputStream --------------------------------------------------------------------------------- */

MemOutputStream::MemOutputStream(int size)
{
	assert(size > 0);
	mCursor = mBuffer = new uint8[size];
	mBufferEnd = mBuffer + size;
}

MemOutputStream::~MemOutputStream()
{
	delete[] mBuffer;
}

int MemOutputStream::write(const void* ptr, int len)
{
	RMX_ASSERT(mCursor + len <= mBufferEnd, "Reached end of MemOutputStream");
	memcpy(mCursor, ptr, len);
	mCursor += len;
	return len;
}

bool MemOutputStream::saveTo(OutputStream& stream)
{
	const int size = getPosition();
	const int written = stream.write(mBuffer, size);
	return (size == written);
}

bool MemOutputStream::saveToFile(const String& filename)
{
	FileHandle file;
	if (!file.open(filename, FILE_ACCESS_WRITE))
		return false;
	file.write(mBuffer, getPosition());
	return true;
}


/* ----- DynOutputStream --------------------------------------------------------------------------------- */

DynOutputStream::DynOutputStream()
{
}

DynOutputStream::~DynOutputStream()
{
	clear();
}

void DynOutputStream::clear()
{
	for (uint8* page : mPages)
		delete[] page;
	mPages.clear();
	mCurrPage = 0;
	mCursor = nullptr;
	mPageEnd = nullptr;
}

void DynOutputStream::AccessPage(int pageIndex)
{
	for (int i = (int)mPages.size(); i <= pageIndex; ++i)
		mPages.push_back(nullptr);

	if (nullptr == mPages[pageIndex])
		mPages[pageIndex] = new uint8[mPageSize];

	mCursor = mPages[pageIndex];
	mPageEnd = mCursor + mPageSize;
}

void DynOutputStream::setPosition(int pos)
{
	assert(pos >= 0);
	mCurrPage = pos / mPageSize;
	AccessPage(mCurrPage);
	mCursor += (pos % mPageSize);
}

int DynOutputStream::getPosition() const
{
	return mCurrPage * mPageSize + (int)(mCursor - mPages[mCurrPage]);
}

int DynOutputStream::write(const void* ptr, int len)
{
	int rest = len;
	while (rest > 0)
	{
		if (mCursor + rest <= mPageEnd)
		{
			memcpy(mCursor, ptr, rest);
			mCursor += rest;
			break;
		}

		const int remaining = (int)(mPageEnd - mCursor);
		if (remaining > 0)
		{
			memcpy(mCursor, ptr, remaining);
			ptr = (const void*)((const uint8*)ptr + remaining);
			rest -= remaining;
		}

		if (mCursor)
			++mCurrPage;
		AccessPage(mCurrPage);
	}
	return len;
}

bool DynOutputStream::saveTo(OutputStream& stream)
{
	if (nullptr == mCursor)
		return true;

	uint8* scratch = nullptr;
	for (int page = 0; page <= mCurrPage; ++page)
	{
		uint8* pageStart = mPages[page];
		if (nullptr == pageStart)
		{
			assert(page != mCurrPage);
			if (nullptr == scratch)
				scratch = new uint8[mPageSize];
			stream.write(scratch, mPageSize);
		}
		else
		{
			int size = (page == mCurrPage) ? (int)(mCursor - pageStart) : mPageSize;
			assert(size > 0 && size <= mPageSize);
			stream.write(pageStart, size);
		}
	}
	delete[] scratch;
	return true;
}
