/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"

class Font;


class PrintedTextCache : public SingleInstance<PrintedTextCache>
{
public:
	struct Key
	{
		uint64 mFontKeyHash = 0;
		uint64 mTextHash = 0;
		int8 mSpacing = 0;

		inline Key() {}
		inline Key(uint64 fontKeyHash, uint64 textHash, int8 spacing) : mFontKeyHash(fontKeyHash), mTextHash(textHash), mSpacing(spacing) {}
		inline uint64 combined() const  { return mFontKeyHash ^ mTextHash ^ mSpacing; }
	};

	struct CacheItem
	{
		Key mKey;
		DrawerTexture mTexture;
		Recti mInnerRect;
		bool mRecentlyUsed = true;
	};

public:
	PrintedTextCache();
	~PrintedTextCache();

	void clear();
	CacheItem* getCacheItem(const Key& key);
	CacheItem& addCacheItem(const Key& key, Font& font, const std::string& textString);

	void regularCleanup();

private:
	std::unordered_map<uint64, CacheItem> mCacheItems;
	uint32 mNextCheckTicks = 0;
};
