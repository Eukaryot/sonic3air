/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


OpenGLFontOutput::OpenGLFontOutput(Font& font) :
	mFont(font)
{
	mLastFontChangeCounter = mFont.getChangeCounter();
}

void OpenGLFontOutput::print(const std::vector<Font::TypeInfo>& infos)
{
	// Display with OpenGL
	if (FTX::Video->getVideoConfig().renderer != rmx::VideoConfig::Renderer::OPENGL)
		return;

#ifdef ALLOW_LEGACY_OPENGL
	// Fill vertex groups
	static std::vector<VertexGroup> vertexGroups;
	vertexGroups.clear();
	buildVertexGroups(vertexGroups, infos);

	// Render them (here still using OpenGL immediate mode rendering)
	for (const VertexGroup& vertexGroup : vertexGroups)
	{
		vertexGroup.mTexture->bind();

		glBegin(GL_TRIANGLES);
		for (const Vertex& vertex : vertexGroup.mVertices)
		{
			glTexCoord2f(vertex.mTexcoords.x, vertex.mTexcoords.y);
			glVertex2f(vertex.mPosition.x, vertex.mPosition.y);
		}
		glEnd();
	}
#else
	RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
#endif
}

void OpenGLFontOutput::buildVertexGroups(std::vector<VertexGroup>& outVertexGroups, const std::vector<Font::TypeInfo>& infos)
{
	checkCacheValidity();

	Texture* currentTexture = nullptr;
	SpriteAtlas::Sprite sprite;

	for (const Font::TypeInfo& info : infos)
	{
		if (nullptr == info.mBitmap)
			continue;

		const uint32 character = info.mUnicode;
		auto it = mHandleMap.find(character);
		if (it == mHandleMap.end())
		{
			if (!loadTexture(info))
				continue;
			it = mHandleMap.find(character);
		}
		const SpriteHandleInfo& spriteHandleInfo = it->second;

		const bool result = mAtlas.getSprite(spriteHandleInfo.mAtlasHandle, sprite);
		RMX_ASSERT(result, "Failed to get sprite from atlas");
		if (!result)
			continue;

		if (sprite.texture != currentTexture)
		{
			currentTexture = sprite.texture;
			vectorAdd(outVertexGroups).mTexture = sprite.texture;
		}

		std::vector<Vertex>& vertices = outVertexGroups.back().mVertices;
		const size_t firstIndex = vertices.size();
		vertices.resize(firstIndex + 6);

		const float x0 = info.mPosition.x - (float)spriteHandleInfo.mBorderLeft;
		const float x1 = info.mPosition.x + (float)(info.mBitmap->mWidth + spriteHandleInfo.mBorderRight);
		const float y0 = info.mPosition.y - (float)spriteHandleInfo.mBorderTop;
		const float y1 = info.mPosition.y + (float)(info.mBitmap->mHeight + spriteHandleInfo.mBorderBottom);

		vertices[firstIndex + 0].mPosition.set(x0, y0);
		vertices[firstIndex + 1].mPosition.set(x0, y1);
		vertices[firstIndex + 2].mPosition.set(x1, y1);
		vertices[firstIndex + 3].mPosition.set(x1, y1);
		vertices[firstIndex + 4].mPosition.set(x1, y0);
		vertices[firstIndex + 5].mPosition.set(x0, y0);

		vertices[firstIndex + 0].mTexcoords.set(sprite.uvStart.x, sprite.uvStart.y);
		vertices[firstIndex + 1].mTexcoords.set(sprite.uvStart.x, sprite.uvEnd.y);
		vertices[firstIndex + 2].mTexcoords.set(sprite.uvEnd.x,   sprite.uvEnd.y);
		vertices[firstIndex + 3].mTexcoords.set(sprite.uvEnd.x,   sprite.uvEnd.y);
		vertices[firstIndex + 4].mTexcoords.set(sprite.uvEnd.x,   sprite.uvStart.y);
		vertices[firstIndex + 5].mTexcoords.set(sprite.uvStart.x, sprite.uvStart.y);
	}
}

bool OpenGLFontOutput::loadTexture(const Font::TypeInfo& typeInfo)
{
	// Load characters as texture
	if (nullptr == typeInfo.mBitmap)
		return false;

	const uint32 character = typeInfo.mUnicode;
	SpriteHandleInfo& spriteHandleInfo = mHandleMap[character];
	if (spriteHandleInfo.mAtlasHandle == -1)
	{
		Font::CharacterInfo& characterInfo = mFont.applyEffects(typeInfo);
		spriteHandleInfo.mAtlasHandle = mAtlas.add(characterInfo.mCachedBitmap);
		spriteHandleInfo.mBorderLeft = characterInfo.mBorderLeft;
		spriteHandleInfo.mBorderRight = characterInfo.mBorderRight;
		spriteHandleInfo.mBorderTop = characterInfo.mBorderTop;
		spriteHandleInfo.mBorderBottom = characterInfo.mBorderBottom;
	}
	return true;
}

void OpenGLFontOutput::checkCacheValidity()
{
	// Cached data is only valid if the underlying font did not change in the meantime
	if (mLastFontChangeCounter != mFont.getChangeCounter())
	{
		mAtlas.clear();
		mHandleMap.clear();
		mLastFontChangeCounter = mFont.getChangeCounter();
	}
}
