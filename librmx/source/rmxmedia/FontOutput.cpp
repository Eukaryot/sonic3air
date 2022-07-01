/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


FontOutput::FontOutput(const FontKey& key)
{
	mKey = key;
}

FontOutput::~FontOutput()
{
	reset();
}

void FontOutput::reset()
{
	mCharacterMap.clear();
}


void FontOutput::applyToTypeInfos(std::vector<ExtendedTypeInfo>& outTypeInfos, const std::vector<Font::TypeInfo>& inTypeInfos)
{
	outTypeInfos.reserve(inTypeInfos.size());
	for (size_t i = 0; i < inTypeInfos.size(); ++i)
	{
		const Font::TypeInfo& typeInfo = inTypeInfos[i];
		if (nullptr == typeInfo.bitmap)
			continue;

		CharacterInfo& characterInfo = applyEffects(typeInfo);

		ExtendedTypeInfo& extendedTypeInfo = vectorAdd(outTypeInfos);
		extendedTypeInfo.mCharacter = typeInfo.unicode;
		extendedTypeInfo.mBitmap = &characterInfo.mCachedBitmap;
		extendedTypeInfo.mDrawPosition = Vec2i(typeInfo.pos) - Vec2i(characterInfo.mBorderLeft, characterInfo.mBorderTop);
	}
}

FontOutput::CharacterInfo& FontOutput::applyEffects(const Font::TypeInfo& typeInfo)
{
	const uint32 character = typeInfo.unicode;
	CharacterInfo& characterInfo = mCharacterMap[character];
	if (characterInfo.mCachedBitmap.empty())
	{
		FontProcessingData fontProcessingData;
		fontProcessingData.mBitmap = *typeInfo.bitmap;
		applyEffects(fontProcessingData, characterInfo);
	}
	return characterInfo;
}

void FontOutput::applyEffects(FontProcessingData& fontProcessingData, CharacterInfo& info)
{
	// Run font processors
	for (const std::shared_ptr<FontProcessor>& processor : mKey.mProcessors)
	{
		processor->process(fontProcessingData);
	}

	info.mBorderLeft = fontProcessingData.mBorderLeft;
	info.mBorderRight = fontProcessingData.mBorderRight;
	info.mBorderTop = fontProcessingData.mBorderTop;
	info.mBorderBottom = fontProcessingData.mBorderBottom;
	info.mCachedBitmap.swap(fontProcessingData.mBitmap);
}



OpenGLFontOutput::OpenGLFontOutput(FontOutput& fontOutput) :
	mFontOutput(fontOutput)
{
}

void OpenGLFontOutput::reset()
{
	mAtlas.clear();
	mHandleMap.clear();
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
	Texture* currentTexture = nullptr;
	SpriteAtlas::Sprite sprite;

	for (const Font::TypeInfo& info : infos)
	{
		if (nullptr == info.bitmap)
			continue;

		const uint32 character = info.unicode;
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

		const float x0 = info.pos.x - (float)spriteHandleInfo.mBorderLeft;
		const float x1 = info.pos.x + (float)(info.bitmap->mWidth + spriteHandleInfo.mBorderRight);
		const float y0 = info.pos.y - (float)spriteHandleInfo.mBorderTop;
		const float y1 = info.pos.y + (float)(info.bitmap->mHeight + spriteHandleInfo.mBorderBottom);

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
	if (nullptr == typeInfo.bitmap)
		return false;

	const uint32 character = typeInfo.unicode;
	SpriteHandleInfo& spriteHandleInfo = mHandleMap[character];
	if (spriteHandleInfo.mAtlasHandle == -1)
	{
		FontOutput::CharacterInfo& characterInfo = mFontOutput.applyEffects(typeInfo);
		spriteHandleInfo.mAtlasHandle = mAtlas.add(characterInfo.mCachedBitmap);
		spriteHandleInfo.mBorderLeft = characterInfo.mBorderLeft;
		spriteHandleInfo.mBorderRight = characterInfo.mBorderRight;
		spriteHandleInfo.mBorderTop = characterInfo.mBorderTop;
		spriteHandleInfo.mBorderBottom = characterInfo.mBorderBottom;
	}
	return true;
}
