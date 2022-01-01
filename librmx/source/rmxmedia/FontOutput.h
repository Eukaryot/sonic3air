/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	FontOutput
*		Rendering of text.
*/

#pragma once

class Font;

class API_EXPORT FontOutput
{
public:
	struct ExtendedTypeInfo
	{
		uint32 mCharacter = 0;
		Bitmap* mBitmap = nullptr;
		Vec2i mDrawPosition;
	};

	struct Vertex
	{
		Vec2f mPosition;
		Vec2f mTexcoords;
	};
	struct VertexGroup
	{
		Texture* mTexture = nullptr;
		std::vector<Vertex> mVertices;
	};

public:
	FontOutput(const FontKey& key);
	~FontOutput();

	void reset();

	void buildVertexGroups(std::vector<VertexGroup>& outVertexGroups, const std::vector<Font::TypeInfo>& infos);
	void print(const std::vector<Font::TypeInfo>& infos);
	void applyToTypeInfos(std::vector<ExtendedTypeInfo>& outTypeInfos, const std::vector<Font::TypeInfo>& inTypeInfos);

private:
	struct SpriteHandleInfo
	{
		Bitmap mCachedBitmap;		// Only used for printing to bitmap
		int mAtlasHandle = -1;		// Only used for printing with OpenGL (i.e. using sprite atlas)
		int mBorderLeft = 0;
		int mBorderRight = 0;
		int mBorderTop = 0;
		int mBorderBottom = 0;
	};
	typedef std::map<uint32, SpriteHandleInfo> SpriteHandleMap;

private:
	void applyEffects(FontProcessingData& fontProcessingData, SpriteHandleInfo& info, bool cacheBitmap);
	void createAtlasHandle(const Bitmap& bitmap, SpriteHandleInfo& info);
	bool loadTexture(const Font::TypeInfo& info);

private:
	FontKey mKey;
	SpriteAtlas mAtlas;
	SpriteHandleMap mHandleMap;
};
