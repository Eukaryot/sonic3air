/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


void SpriteAtlasBase::Node::clear()
{
	for (int i = 0; i < 2; ++i)
	{
		if (mChildNode[i])
		{
			mChildNode[i]->clear();
			SAFE_DELETE(mChildNode[i]);
		}
	}
	mUsed = false;
}

SpriteAtlasBase::Node* SpriteAtlasBase::Node::insert(const Vec2i& size, int padding)
{
	if (mChildNode[0] != nullptr)
	{
		assert(mChildNode[1] != nullptr);

		// Add to a new child node
		Node* newNode = mChildNode[0]->insert(size, padding);
		if (nullptr != newNode)
			return newNode;

		// No more space in there, then try the second child
		return mChildNode[1]->insert(size, padding);
	}

	// Node already in use?
	if (mUsed)
		return nullptr;

	// Not enough space left?
	const bool fits = (size.x <= mRect.width && size.y <= mRect.height);
	if (!fits)
		return nullptr;

	// Check for a perfect fit
	const bool perfectFit = (size.x == mRect.width && size.y == mRect.height);
	if (perfectFit)
	{
		mUsed = true;
		return this;
	}

	// Split along the longer rectangle side
	Recti rect0(mRect);
	Recti rect1(mRect);
	const int dw = mRect.width - size.x;
	const int dh = mRect.height - size.y;
	if (dw > dh)
	{
		rect0.width = size.x;
		rect1.x += size.x + padding;
		rect1.width -= size.x + padding;
	}
	else
	{
		rect0.height = size.y;
		rect1.y += size.y + padding;
		rect1.height -= size.y + padding;
	}

	// Split into two child nodes
	mChildNode[0] = new Node(rect0);
	mChildNode[1] = new Node(rect1);

	// Now add to the first child
	return mChildNode[0]->insert(size, padding);
}


SpriteAtlasBase::SpriteAtlasBase()
{
}

SpriteAtlasBase::~SpriteAtlasBase()
{
	clear();
}

void SpriteAtlasBase::clear()
{
	mPages.clear();
	mSprites.clear();
}

bool SpriteAtlasBase::add(uint32 key, const Vec2i& size)
{
	return internalAdd(key, size);
}

void SpriteAtlasBase::rebuild()
{
	// Save sprite data
	std::vector<SpriteInfo> sprites;
	for (unsigned int i = 0; i < mSprites.size(); ++i)
	{
		if (mSprites[i].mPageIndex >= 0)
		{
			sprites.push_back(mSprites[i]);
		}
	}

	// Clear old content
	clear();

	// Sort sprites by their sizes
	std::sort(sprites.begin(), sprites.end(), compareSpriteInfoBySize);

	// Insert sprites again
	for (size_t i = 0; i < sprites.size(); ++i)
	{
		internalAdd(sprites[i].mKey, Vec2i(sprites[i].mRect.width, sprites[i].mRect.height));
	}
}

bool SpriteAtlasBase::valid(uint32 key)
{
	const SpriteInfo* info = getSpriteInfo(key);
	return (nullptr != info && info->mPageIndex >= 0);
}

bool SpriteAtlasBase::getSprite(uint32 key, Sprite& sprite)
{
	SpriteInfo* info = getSpriteInfo(key);
	if (nullptr == info || info->mPageIndex < 0)
		return false;

	sprite.mPage.mIndex = info->mPageIndex;
	sprite.mPage.mPageSize = mPageSize;
	sprite.mRect = info->mRect;
	return true;
}

bool SpriteAtlasBase::getPage(int num, Page& page)
{
	if (num < 0 || num >= (int)mPages.size())
		return false;

	page.mIndex = num;
	page.mPageSize = mPageSize;
	return true;
}

bool SpriteAtlasBase::internalAdd(uint32 key, const Vec2i& size)
{
	// TODO: Check if input is too large for one page

	PageInfo* pageInfo = nullptr;
	Node* node = nullptr;
	int pageIndex = -1;

	for (unsigned int page = 0; page < mPages.size(); ++page)
	{
		node = mPages[page].mRootNode.insert(size, mPadding);
		if (nullptr != node)
		{
			pageInfo = &mPages[page];
			pageIndex = page;
			break;
		}
	}

	if (nullptr == node)
	{
		// Add a new page
		pageInfo = &vectorAdd(mPages);
		pageInfo->mRootNode.mRect.set(0, 0, mPageSize.x, mPageSize.y);

		node = pageInfo->mRootNode.insert(size, mPadding);
		RMX_ASSERT(nullptr != node, "Invalid node pointer returned");
		pageIndex = (int)mPages.size() - 1;
	}

	RMX_ASSERT(mSprites.count(key) == 0, "Double usage of the same key in sprite atlas");
	SpriteInfo& spriteInfo = mSprites[key];
	spriteInfo.mPageIndex = pageIndex;
	spriteInfo.mRect = node->mRect;

	return true;
}

SpriteAtlasBase::SpriteInfo* SpriteAtlasBase::getSpriteInfo(uint32 key)
{
	const auto it = mSprites.find(key);
	return (it == mSprites.end()) ? nullptr : &it->second;
}

bool SpriteAtlasBase::compareSpriteInfoBySize(const SpriteInfo& first, const SpriteInfo& second)
{
	int size1 = first.mRect.width * first.mRect.height;
	int size2 = second.mRect.width * second.mRect.height;
	return (size1 > size2);
}


SpriteAtlas::SpriteAtlas()
{
}

SpriteAtlas::~SpriteAtlas()
{
	clear();
}

void SpriteAtlas::clear()
{
	mPages.clear();
	mSprites.clear();
}

int SpriteAtlas::add(const Bitmap& bmp)
{
	return internalAdd(bmp, nullptr);
}

int SpriteAtlas::add(const Bitmap& bmp, const Recti& rect)
{
	return internalAdd(bmp, &rect);
}

void SpriteAtlas::rebuild()
{
	// Save page bitmaps
	std::vector<Bitmap> oldPageBitmaps;
	oldPageBitmaps.resize(mPageData.size());
	for (unsigned int i = 0; i < mPageData.size(); ++i)
	{
		oldPageBitmaps[i].copy(mPageData[i].mBitmap);
	}
	mPageData.clear();

	// Save sprite data
	std::map<uint32, SpriteInfo> oldSpriteInfo = mSprites;

	// Rebuild sprite atlas nodes
	SpriteAtlasBase::rebuild();

	// Rebuild the new page bitmaps
	mPageData.resize(mPages.size());
	for (PageData& pageData : mPageData)
	{
		pageData.mBitmap.create(mPageSize.x, mPageSize.y);
		pageData.mTexture.create(mPageSize.x, mPageSize.y);
	}
	for (const auto& pair : oldSpriteInfo)
	{
		const Bitmap& src = oldPageBitmaps[pair.second.mPageIndex];

		const SpriteInfo* newSprite = getSpriteInfo(pair.second.mKey);
		RMX_ASSERT(nullptr != newSprite, "Invalid sprite pointer");

		mPageData[newSprite->mPageIndex].mBitmap.insert(newSprite->mRect.x, newSprite->mRect.y, src, pair.second.mRect);
	}

	// Update textures
	for (PageData& pageData : mPageData)
	{
		pageData.mTexture.load(pageData.mBitmap);
	}
}

bool SpriteAtlas::valid(int handle)
{
	return SpriteAtlasBase::valid((uint32)handle);
}

bool SpriteAtlas::getSprite(int handle, Sprite& sprite)
{
	const SpriteInfo* info = getSpriteInfo((uint32)handle);
	if (nullptr == info || info->mPageIndex < 0)
		return false;

	const Bitmap& bitmap = mPageData[info->mPageIndex].mBitmap;
	sprite.texture = &mPageData[info->mPageIndex].mTexture;
	sprite.uvStart.x = (float)info->mRect.x / (float)bitmap.mWidth;
	sprite.uvStart.y = (float)info->mRect.y / (float)bitmap.mHeight;
	sprite.uvEnd.x = (float)(info->mRect.x + info->mRect.width) / (float)bitmap.mWidth;
	sprite.uvEnd.y = (float)(info->mRect.y + info->mRect.height) / (float)bitmap.mHeight;
	return true;
}

const Texture* SpriteAtlas::getPage(int num)
{
	if (num < 0 || num >= (int)mPageData.size())
		return nullptr;
	return &mPageData[num].mTexture;
}


int SpriteAtlas::internalAdd(const Bitmap& bmp, const Recti* rect, bool updateTexture)
{
	Vec2i insertionSize;
	insertionSize.x = (nullptr != rect) ? rect->width : bmp.mWidth;
	insertionSize.y = (nullptr != rect) ? rect->height : bmp.mHeight;

	const int key = (int)mSprites.size();	// No special key, just enumeration
	SpriteAtlasBase::internalAdd(key, insertionSize);

	// Check if the sprite requires adding a new page
	SpriteAtlasBase::Sprite sprite;
	if (!SpriteAtlasBase::getSprite(key, sprite))
	{
		RMX_ASSERT(false, "Fatal error");
		return -1;
	}

	PageData* pageData = nullptr;
	if (sprite.mPage.mIndex < (int)mPageData.size())
	{
		pageData = &mPageData[sprite.mPage.mIndex];
	}
	else
	{
		pageData = &vectorAdd(mPageData);
		pageData->mBitmap.create(sprite.mPage.mPageSize.x, sprite.mPage.mPageSize.y, 0x80808080);
		pageData->mTexture.create(sprite.mPage.mPageSize.x, sprite.mPage.mPageSize.y);
	}

	if (nullptr != rect)
	{
		pageData->mBitmap.insert(sprite.mRect.x, sprite.mRect.y, bmp, *rect);
	}
	else
	{
		pageData->mBitmap.insert(sprite.mRect.x, sprite.mRect.y, bmp);
	}

	if (updateTexture)
	{
		if (nullptr != rect)
		{
			Bitmap part;
			part.copy(bmp, *rect);
			pageData->mTexture.updateRect(part, sprite.mRect.x, sprite.mRect.y);
		}
		else
		{
			pageData->mTexture.updateRect(bmp, sprite.mRect.x, sprite.mRect.y);
		}
	}

	return key;
}