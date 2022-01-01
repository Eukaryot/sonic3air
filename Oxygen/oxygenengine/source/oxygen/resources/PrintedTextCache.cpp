/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/PrintedTextCache.h"


PrintedTextCache::PrintedTextCache()
{
}

PrintedTextCache::~PrintedTextCache()
{
}

void PrintedTextCache::clear()
{
	mCacheItems.clear();
}

PrintedTextCache::CacheItem* PrintedTextCache::getCacheItem(const Key& key)
{
	const auto it = mCacheItems.find(key.combined());
	if (it == mCacheItems.end())
		return nullptr;

	CacheItem& cacheItem = it->second;
	cacheItem.mRecentlyUsed = true;
	return &cacheItem;
}

PrintedTextCache::CacheItem& PrintedTextCache::addCacheItem(const Key& key, Font& font, const std::string& textString)
{
	CacheItem& cacheItem = mCacheItems[key.combined()];
	cacheItem.mKey = key;
	cacheItem.mRecentlyUsed = true;

	EngineMain::instance().getDrawer().createTexture(cacheItem.mTexture);
	Bitmap& bitmap = cacheItem.mTexture.accessBitmap();
	font.printBitmap(bitmap, cacheItem.mInnerRect, textString, key.mSpacing);
	cacheItem.mTexture.bitmapUpdated();
	return cacheItem;
}

void PrintedTextCache::regularCleanup()
{
	const uint32 currentTicks = SDL_GetTicks();
	if ((int32)(currentTicks - mNextCheckTicks) < 0)
		return;

	// Remove all cached items that were not used since last cleanup
	std::vector<uint64> keysToRemove;
	for (auto& pair : mCacheItems)
	{
		if (pair.second.mRecentlyUsed)
		{
			pair.second.mRecentlyUsed = false;
		}
		else
		{
			keysToRemove.push_back(pair.first);
		}
	}
	for (uint64 key : keysToRemove)
	{
		mCacheItems.erase(key);
	}

	// Check again in 5 seconds
	mNextCheckTicks = currentTicks + 5000;
}
