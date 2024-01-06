/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	SpriteAtlas
*		Dynamic sprite/texture atlas.
*/

#pragma once

class API_EXPORT SpriteAtlasBase
{
public:
	struct Page
	{
		int mIndex = 0;
		Vec2i mPageSize;
	};
	struct Sprite
	{
		uint32 mKey = 0;
		Page mPage;
		Recti mRect;
	};

public:
	SpriteAtlasBase();
	~SpriteAtlasBase();

	void clear();
	bool add(uint32 key, const Vec2i& size);
	void rebuild();

	bool valid(uint32 key);
	bool getSprite(uint32 key, Sprite& sprite);

	inline int getNumPages() const  { return (int)mPages.size(); }
	bool getPage(int index, Page& page);

protected:
	struct SpriteInfo;

	bool internalAdd(uint32 key, const Vec2i& size);
	SpriteInfo* getSpriteInfo(uint32 key);

private:
	static bool compareSpriteInfoBySize(const SpriteInfo& first, const SpriteInfo& second);

protected:
	Vec2i mPageSize = Vec2i(512, 128);		// That size is a bit small for usual text rendering, but okay for pixelized rendering as used by Oxygen
	int mPadding = 1;

	struct Node
	{
		Node* mChildNode[2] = { nullptr, nullptr };
		Recti mRect;
		bool mUsed = false;

		inline Node() {}
		inline Node(const Recti& rct) : mRect(rct) {}
		inline ~Node()  { clear(); }

		void clear();
		Node* insert(const Vec2i& size, int padding);
	};

	struct PageInfo
	{
		Node mRootNode;
	};
	std::vector<PageInfo> mPages;

	struct SpriteInfo
	{
		uint32 mKey = 0xffffffff;
		int mPageIndex = -1;
		Recti mRect;
	};
	std::map<uint32, SpriteInfo> mSprites;
};



#ifdef RMX_WITH_OPENGL_SUPPORT

class API_EXPORT SpriteAtlas : protected SpriteAtlasBase
{
public:
	struct Sprite
	{
		Texture* mTexture = nullptr;
		Vec2f mUVStart;
		Vec2f mUVEnd;
	};

public:
	SpriteAtlas();
	~SpriteAtlas();

	void clear();
	int add(const Bitmap& bmp);
	int add(const Bitmap& bmp, const Recti& rect);
	void rebuild();

	bool valid(int handle);
	bool getSprite(int handle, Sprite& sprite);

	int getPageCount()  { return (int)mPages.size(); }
	const Texture* getPage(int num);

private:
	int internalAdd(const Bitmap& bmp, const Recti* rect, bool updateTexture = true);

private:
	struct PageData
	{
		Bitmap mBitmap;
		Texture mTexture;
	};
	std::vector<PageData> mPageData;
};

#endif
