/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	OpenGLFontOutput
*		Rendering of text via OpenGL.
*/

#pragma once

class Font;


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
	explicit OpenGLFontOutput(Font& font);

	void reset();
	inline const FontKey& getFontKey() const  { return mFont.getKey(); }

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
	Font& mFont;
	SpriteAtlas mAtlas;
	std::map<uint32, SpriteHandleInfo> mHandleMap;
};
