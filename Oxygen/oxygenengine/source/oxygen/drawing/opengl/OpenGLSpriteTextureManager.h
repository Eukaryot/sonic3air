/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/OpenGLDrawerTexture.h"
#include "oxygen/rendering/utils/BufferTexture.h"
#include "oxygen/resources/SpriteCollection.h"


class OpenGLSpriteTextureManager : public SingleInstance<OpenGLSpriteTextureManager>
{
public:
	BufferTexture* getPaletteSpriteTexture(const SpriteCollection::Item& cacheItem, bool useUpscaledSprite);
	OpenGLTexture* getComponentSpriteTexture(const SpriteCollection::Item& cacheItem);

private:
	template<typename T> struct ChangeCounted
	{
		T mTexture;
		uint32 mChangeCounter = -1;
	};
	std::unordered_map<uint64, ChangeCounted<BufferTexture>> mPaletteSpriteTextures;
	std::unordered_map<uint64, ChangeCounted<OpenGLTexture>> mComponentSpriteTextures;
};

#endif
