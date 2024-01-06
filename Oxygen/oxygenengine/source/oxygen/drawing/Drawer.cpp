/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/Drawer.h"
#include "oxygen/drawing/DrawerInterface.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/resources/SpriteCache.h"


Drawer::Drawer()
{
}

Drawer::~Drawer()
{
	shutdown();

	// Unregister all textures
	for (DrawerTexture* texture : mDrawerTextures)
	{
		texture->mRegisteredOwner = nullptr;
	}
}

Drawer::Type Drawer::getType() const
{
	return mActiveDrawer->getType();
}

void Drawer::destroyDrawer()
{
	SAFE_DELETE(mActiveDrawer);

	// Invalidate drawer textures
	for (DrawerTexture* texture : mDrawerTextures)
	{
		texture->invalidate();
	}
}

void Drawer::shutdown()
{
	destroyDrawer();
}

void Drawer::createTexture(DrawerTexture& outTexture)
{
	RMX_ASSERT(nullptr != mActiveDrawer, "No active drawer instance created");
	RMX_ASSERT(nullptr == outTexture.mRegisteredOwner, "Drawer texture already registered");
	RMX_ASSERT(!outTexture.isValid(), "Drawer texture already created");

	mActiveDrawer->createTexture(outTexture);
	outTexture.mRegisteredOwner = this;
	outTexture.mRegisteredIndex = mDrawerTextures.size();
	mDrawerTextures.push_back(&outTexture);
}

Recti Drawer::getSpriteRect(uint64 spriteKey) const
{
	const SpriteCache::CacheItem* item = SpriteCache::instance().getSprite(spriteKey);
	if (nullptr == item || nullptr == item->mSprite)
		return Recti();

	return Recti(item->mSprite->mOffset, item->mSprite->getSize());
}

void Drawer::setRenderTarget(DrawerTexture& texture, const Recti& rect)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSetRenderTargetDrawCommands.createObject(texture, rect));
}

void Drawer::setWindowRenderTarget(const Recti& rect)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSetWindowRenderTargetDrawCommands.createObject(rect));
}

void Drawer::setBlendMode(BlendMode blendMode)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSetBlendModeDrawCommands.createObject(blendMode));
}

void Drawer::setSamplingMode(SamplingMode samplingMode)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSetSamplingModeDrawCommands.createObject(samplingMode));
}

void Drawer::setWrapMode(TextureWrapMode wrapMode)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSetWrapModeDrawCommands.createObject(wrapMode));
}

void Drawer::drawRect(const Rectf& rect, const Color& color)
{
	if (!rect.isEmpty())
	{
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mRectDrawCommands.createObject(rect, color));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture)
{
	if (!rect.isEmpty())
	{
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mRectDrawCommands.createObject(rect, texture));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture, const Color& tintColor)
{
	if (!rect.isEmpty())
	{
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mRectDrawCommands.createObject(rect, texture, tintColor));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture, const Vec2f& uv0, const Vec2f& uv1, const Color& tintColor)
{
	if (!rect.isEmpty())
	{
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mRectDrawCommands.createObject(rect, texture, uv0, uv1, tintColor));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture, const Recti& textureInnerRect, const Color& tintColor)
{
	const Vec2f texSize = Vec2f(texture.getSize());
	if (texSize.x < 1.0f || texSize.y < 1.0f)
		return;

	const Vec2f uv0 = Vec2f(textureInnerRect.getPos()) / texSize;
	const Vec2f uv1 = Vec2f(textureInnerRect.getPos() + textureInnerRect.getSize()) / texSize;
	drawRect(rect, texture, uv0, uv1, tintColor);
}

void Drawer::drawUpscaledRect(const Rectf& rect, DrawerTexture& texture)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mUpscaledRectDrawCommands.createObject(rect, texture));
}

void Drawer::drawSprite(Vec2i position, uint64 spriteKey, const Color& tintColor, Vec2f scale)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSpriteDrawCommands.createObject(position, spriteKey, tintColor, scale));
}

void Drawer::drawSpriteRect(const Recti& rect, uint64 spriteKey, const Color& tintColor)
{
	if (!rect.isEmpty())
	{
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mSpriteRectDrawCommands.createObject(rect, spriteKey, tintColor));
	}
}

void Drawer::drawMesh(const std::vector<DrawerMeshVertex>& triangles, DrawerTexture& texture)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mMeshDrawCommands.createObject(triangles, texture));
}

void Drawer::drawMesh(const std::vector<DrawerMeshVertex_P2_C4>& triangles)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mMeshVertexColorDrawCommands.createObject(triangles));
}

void Drawer::drawQuad(const DrawerMeshVertex* quad, DrawerTexture& texture)
{
	std::vector<DrawerMeshVertex> triangles;
	triangles.resize(6);
	triangles[0] = quad[0];
	triangles[1] = quad[1];
	triangles[2] = quad[2];
	triangles[3] = quad[2];
	triangles[4] = quad[1];
	triangles[5] = quad[3];
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mMeshDrawCommands.createObject(std::move(triangles), texture));
}

void Drawer::printText(Font& font, const Recti& rect, const String& text, int alignment, Color color)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextDrawCommands.createObject(font, rect, text, alignment, color));
}

void Drawer::printText(Font& font, const Vec2i& position, const String& text, int alignment, Color color)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextDrawCommands.createObject(font, Recti(position.x, position.y, 0, 0), text, alignment, color));
}

void Drawer::printText(Font& font, const Recti& rect, const String& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextDrawCommands.createObject(font, rect, text, printOptions));
}

void Drawer::printText(Font& font, const Vec2i& position, const String& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextDrawCommands.createObject(font, Recti(position.x, position.y, 0, 0), text, printOptions));
}

void Drawer::printText(Font& font, const Recti& rect, const WString& text, int alignment, Color color)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextWDrawCommands.createObject(font, rect, text, alignment, color));
}

void Drawer::printText(Font& font, const Vec2i& position, const WString& text, int alignment, Color color)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextWDrawCommands.createObject(font, Recti(position.x, position.y, 0, 0), text, alignment, color));
}

void Drawer::printText(Font& font, const Recti& rect, const WString& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextWDrawCommands.createObject(font, rect, text, printOptions));
}

void Drawer::printText(Font& font, const Vec2i& position, const WString& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPrintTextWDrawCommands.createObject(font, Recti(position.x, position.y, 0, 0), text, printOptions));
}

void Drawer::pushScissor(const Recti& rect)
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPushScissorDrawCommands.createObject(rect));
}

void Drawer::popScissor()
{
	mDrawCollection.addDrawCommand(DrawCommand::mFactory.mPopScissorDrawCommands.createObject());
}

void Drawer::setupRenderWindow(SDL_Window* window)
{
	RMX_ASSERT(nullptr != mActiveDrawer, "No active drawer instance created");
	mActiveDrawer->setupRenderWindow(window);
}

void Drawer::performRendering()
{
	RMX_ASSERT(nullptr != mActiveDrawer, "No active drawer instance created");
	mActiveDrawer->performRendering(mDrawCollection);
	mDrawCollection.clear();
}

void Drawer::presentScreen()
{
	RMX_ASSERT(nullptr != mActiveDrawer, "No active drawer instance created");
	mActiveDrawer->presentScreen();
}

bool Drawer::onDrawerCreated()
{
	if (!mActiveDrawer->wasSetupSuccessful())
	{
		destroyDrawer();
		return false;
	}

	// Recreate implementations of registered drawer textures
	for (DrawerTexture* texture : mDrawerTextures)
	{
		RMX_ASSERT(!texture->isValid(), "Drawer texture already created");
		mActiveDrawer->refreshTexture(*texture);
	}
	return true;
}

void Drawer::unregisterTexture(DrawerTexture& texture)
{
	// Remove by swapping with last texture
	const size_t index = texture.mRegisteredIndex;
	if (index + 1 < mDrawerTextures.size())
	{
		mDrawerTextures[index] = mDrawerTextures.back();
		mDrawerTextures[index]->mRegisteredIndex = index;
	}
	mDrawerTextures.pop_back();
}
