/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/OpenGLSpriteTextureManager.h"


BufferTexture* OpenGLSpriteTextureManager::getPaletteSpriteTexture(const SpriteCollection::Item& cacheItem, bool useUpscaledSprite)
{
	RMX_CHECK(!cacheItem.mUsesComponentSprite, "Sprite is not a palette sprite", RMX_REACT_THROW);
	const PaletteSprite& sprite = *static_cast<PaletteSprite*>(cacheItem.mSprite);
	ChangeCounted<BufferTexture>& texture = mPaletteSpriteTextures[useUpscaledSprite ? (cacheItem.mKey + 0x123456) : cacheItem.mKey];

	// Check for change
	if (texture.mChangeCounter != cacheItem.mChangeCounter)
	{
		const PaletteBitmap& bitmap = useUpscaledSprite ? sprite.getUpscaledBitmap() : sprite.getBitmap();
		texture.mTexture.create(BufferTexture::PixelFormat::UINT_8, bitmap.getSize(), bitmap.getData());
		texture.mChangeCounter = cacheItem.mChangeCounter;
	}
	return &texture.mTexture;
}

OpenGLTexture* OpenGLSpriteTextureManager::getComponentSpriteTexture(const SpriteCollection::Item& cacheItem)
{
	RMX_CHECK(cacheItem.mUsesComponentSprite, "Sprite is not a component sprite", RMX_REACT_THROW);
	ChangeCounted<OpenGLTexture>& texture = mComponentSpriteTextures[cacheItem.mKey];

	// Check for change
	if (texture.mChangeCounter != cacheItem.mChangeCounter)
	{
		const Bitmap& bitmap = static_cast<ComponentSprite*>(cacheItem.mSprite)->getBitmap();
		texture.mTexture.loadBitmap(bitmap);
		texture.mChangeCounter = cacheItem.mChangeCounter;
	}
	return &texture.mTexture;
}

#endif
