/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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
	struct CacheItem
	{
		uint64 mFontKeyHash = 0;
		uint64 mTextHash = 0;
		DrawerTexture mTexture;
		Recti mInnerRect;
		bool mRecentlyUsed = true;
	};

public:
	PrintedTextCache();
	~PrintedTextCache();

	void clear();
	CacheItem* getCacheItem(uint64 fontKeyHash, uint64 textHash);
	CacheItem& addCacheItem(Font& font, uint64 fontKeyHash, const std::string& textString, uint64 textHash);

	void regularCleanup();

private:
	std::unordered_map<uint64, CacheItem> mCacheItems;
	uint32 mNextCheckTicks = 0;
};
