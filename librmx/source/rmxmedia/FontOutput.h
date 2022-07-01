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
struct FontProcessingData;


class API_EXPORT FontOutput
{
public:
	struct ExtendedTypeInfo
	{
		uint32 mCharacter = 0;
		Bitmap* mBitmap = nullptr;
		Vec2i mDrawPosition;
	};

	struct CharacterInfo
	{
		Bitmap mCachedBitmap;
		int mBorderLeft = 0;
		int mBorderRight = 0;
		int mBorderTop = 0;
		int mBorderBottom = 0;
	};

public:
	explicit FontOutput(const FontKey& key);
	~FontOutput();

	void reset();
	inline const FontKey& getFontKey() const  { return mKey; }

	void applyToTypeInfos(std::vector<ExtendedTypeInfo>& outTypeInfos, const std::vector<Font::TypeInfo>& inTypeInfos);
	CharacterInfo& applyEffects(const Font::TypeInfo& typeInfo);

private:
	void applyEffects(FontProcessingData& fontProcessingData, CharacterInfo& info);

private:
	FontKey mKey;
	std::map<uint32, CharacterInfo> mCharacterMap;
};


class API_EXPORT OpenGLFontOutput
{
public:
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
	explicit OpenGLFontOutput(FontOutput& fontOutput);

	void reset();
	inline const FontKey& getFontKey() const  { return mFontOutput.getFontKey(); }

	void print(const std::vector<Font::TypeInfo>& infos);
	void buildVertexGroups(std::vector<VertexGroup>& outVertexGroups, const std::vector<Font::TypeInfo>& infos);

private:
	struct SpriteHandleInfo
	{
		int mAtlasHandle = -1;
		int mBorderLeft = 0;
		int mBorderRight = 0;
		int mBorderTop = 0;
		int mBorderBottom = 0;
	};

private:
	bool loadTexture(const Font::TypeInfo& info);

private:
	FontOutput& mFontOutput;
	SpriteAtlas mAtlas;
	std::map<uint32, SpriteHandleInfo> mHandleMap;
};
