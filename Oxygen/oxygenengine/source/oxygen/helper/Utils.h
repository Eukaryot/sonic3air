/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Font;


namespace utils
{
	void splitTextIntoLines(std::vector<std::string>& outLines, const std::string& text, Font& font, int maxLineWidth);
	void splitTextIntoLines(std::vector<std::string_view>& outLines, std::string_view text, Font& font, int maxLineWidth);
	void shortenTextToFit(std::string& text, Font& font, int maxLineWidth);

	uint32 getVersionNumberFromString(const std::string& versionString);
	std::string getVersionStringFromNumber(uint32 versionNumber);

	void buildSpriteAtlas(const std::wstring& outputFilename, const std::wstring& imagesFileMask);	// For 32-bit bitmaps (uses RMX SpriteAtlas)
	void buildSpriteAtlas2(const std::wstring& outputFilename, const std::wstring& imagesFileMask);	// For 8-bit bitmaps (uses fixed grid)
}


class BitVector
{
public:
	inline void initialize(size_t size)
	{
		const size_t elements = (size + 31) / 32;
		mInternal.resizeTo(elements);
		mInternal.count = elements;
		memset(mInternal.list, 0, elements * 4);
	}

	inline bool get(size_t index) const
	{
		const uint32 bit = (1 << (index % 32));
		return (mInternal[index / 32] & bit) != 0;
	}

	inline void set(size_t index)
	{
		const uint32 bit = (1 << (index % 32));
		mInternal.list[index / 32] |= bit;
	}

private:
	CArray<uint32> mInternal;
};


template<typename VALUE, int TABLESIZE, int SHIFT, int RMX_PAGESIZE = 64>
class LinearLookupTable
{
public:
	inline LinearLookupTable()
	{
		mTable.resize(TABLESIZE >> SHIFT, nullptr);
	}

	inline size_t size() const  { return mNumEntries; }

	inline void clear()
	{
		mEntriesPool.clear();
		memset(&mTable[0], 0, mTable.size() * sizeof(Entry*));
		mNumEntries = 0;
	}

	inline VALUE* add(uint32 key)
	{
		const uint32 index = getIndex(key);
		if (index >= (uint32)mTable.size())
			return nullptr;

		Entry& newEntry = mEntriesPool.createObject();
		newEntry.mKey = key;
		newEntry.mNext = mTable[index];

		mTable[index] = &newEntry;
		++mNumEntries;
		return &newEntry.mValue;
	}

	inline void add(uint32 key, VALUE value)
	{
		VALUE* result = add(key);
		if (nullptr != result)
		{
			*result = value;
		}
	}

	inline VALUE* find(uint32 key)
	{
		const uint32 index = getIndex(key);
		if (index < (uint32)mTable.size())
		{
			Entry* entry = mTable[index];
			while (nullptr != entry)
			{
				if (entry->mKey == key)
					return &entry->mValue;
				entry = entry->mNext;
			}
		}
		return nullptr;
	}

private:
	inline uint32 getIndex(uint32 key) const
	{
		return key >> SHIFT;
	}

private:
	struct Entry
	{
		uint32 mKey = 0;
		VALUE mValue = VALUE();
		Entry* mNext = nullptr;
	};
	ObjectPool<Entry, RMX_PAGESIZE> mEntriesPool;
	std::vector<Entry*> mTable;
	size_t mNumEntries = 0;
};
