/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/Drawer.h"
#include "oxygen/drawing/DrawerInterface.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/resources/SpriteCollection.h"


namespace
{
	template<typename T> ObjectPool<T>& getPool()	{ return T::INVALID; }

	template<> ObjectPool<SetWindowRenderTargetDrawCommand>& getPool<SetWindowRenderTargetDrawCommand>()	{ return DrawCommand::mFactory.mSetWindowRenderTargetDrawCommands; }
	template<> ObjectPool<SetRenderTargetDrawCommand>&		 getPool<SetRenderTargetDrawCommand>	  ()	{ return DrawCommand::mFactory.mSetRenderTargetDrawCommands; }
	template<> ObjectPool<RectDrawCommand>&					 getPool<RectDrawCommand>				  ()	{ return DrawCommand::mFactory.mRectDrawCommands; }
	template<> ObjectPool<UpscaledRectDrawCommand>&			 getPool<UpscaledRectDrawCommand>		  ()	{ return DrawCommand::mFactory.mUpscaledRectDrawCommands; }
	template<> ObjectPool<SpriteDrawCommand>&				 getPool<SpriteDrawCommand>				  ()	{ return DrawCommand::mFactory.mSpriteDrawCommands; }
	template<> ObjectPool<SpriteRectDrawCommand>&			 getPool<SpriteRectDrawCommand>			  ()	{ return DrawCommand::mFactory.mSpriteRectDrawCommands; }
	template<> ObjectPool<MeshDrawCommand>&					 getPool<MeshDrawCommand>				  ()	{ return DrawCommand::mFactory.mMeshDrawCommands; }
	template<> ObjectPool<MeshVertexColorDrawCommand>&		 getPool<MeshVertexColorDrawCommand>	  ()	{ return DrawCommand::mFactory.mMeshVertexColorDrawCommands; }
	template<> ObjectPool<SetBlendModeDrawCommand>&			 getPool<SetBlendModeDrawCommand>		  ()	{ return DrawCommand::mFactory.mSetBlendModeDrawCommands; }
	template<> ObjectPool<SetSamplingModeDrawCommand>&		 getPool<SetSamplingModeDrawCommand>	  ()	{ return DrawCommand::mFactory.mSetSamplingModeDrawCommands; }
	template<> ObjectPool<SetWrapModeDrawCommand>&			 getPool<SetWrapModeDrawCommand>		  ()	{ return DrawCommand::mFactory.mSetWrapModeDrawCommands; }
	template<> ObjectPool<PrintTextDrawCommand>&			 getPool<PrintTextDrawCommand>			  ()	{ return DrawCommand::mFactory.mPrintTextDrawCommands; }
	template<> ObjectPool<PrintTextWDrawCommand>&			 getPool<PrintTextWDrawCommand>			  ()	{ return DrawCommand::mFactory.mPrintTextWDrawCommands; }
	template<> ObjectPool<PushScissorDrawCommand>&			 getPool<PushScissorDrawCommand>		  ()	{ return DrawCommand::mFactory.mPushScissorDrawCommands; }
	template<> ObjectPool<PopScissorDrawCommand>&			 getPool<PopScissorDrawCommand>			  ()	{ return DrawCommand::mFactory.mPopScissorDrawCommands; }
}


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

void Drawer::updateDrawer(float deltaSeconds)
{
	if (nullptr != mActiveDrawer)
		mActiveDrawer->updateDrawer(deltaSeconds);
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
	const SpriteCollection::Item* item = SpriteCollection::instance().getSprite(spriteKey);
	if (nullptr == item || nullptr == item->mSprite)
		return Recti();

	return Recti(item->mSprite->mOffset, item->mSprite->getSize());
}

void Drawer::setWindowRenderTarget(const Recti& rect)
{
	addDrawCommand(getPool<SetWindowRenderTargetDrawCommand>().createObject(rect));
}

void Drawer::setRenderTarget(DrawerTexture& texture, const Recti& rect)
{
	addDrawCommand(getPool<SetRenderTargetDrawCommand>().createObject(texture, rect));
}

void Drawer::setBlendMode(BlendMode blendMode)
{
	addDrawCommand(getPool<SetBlendModeDrawCommand>().createObject(blendMode));
}

void Drawer::setSamplingMode(SamplingMode samplingMode)
{
	addDrawCommand(getPool<SetSamplingModeDrawCommand>().createObject(samplingMode));
}

void Drawer::setWrapMode(TextureWrapMode wrapMode)
{
	addDrawCommand(getPool<SetWrapModeDrawCommand>().createObject(wrapMode));
}

void Drawer::drawRect(const Rectf& rect, const Color& color)
{
	if (!rect.isEmpty())
	{
		addDrawCommand(getPool<RectDrawCommand>().createObject(rect, color));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture)
{
	if (!rect.isEmpty())
	{
		addDrawCommand(getPool<RectDrawCommand>().createObject(rect, texture));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture, const Color& tintColor)
{
	if (!rect.isEmpty())
	{
		addDrawCommand(getPool<RectDrawCommand>().createObject(rect, texture, tintColor));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture, const Vec2f& uv0, const Vec2f& uv1, const Color& tintColor)
{
	if (!rect.isEmpty())
	{
		addDrawCommand(getPool<RectDrawCommand>().createObject(rect, texture, uv0, uv1, tintColor));
	}
}

void Drawer::drawRect(const Rectf& rect, DrawerTexture& texture, const Recti& textureInnerRect, const Color& tintColor)
{
	const Vec2f texSize(texture.getSize());
	if (texSize.x < 1.0f || texSize.y < 1.0f)
		return;

	const Vec2f uv0 = Vec2f(textureInnerRect.getPos()) / texSize;
	const Vec2f uv1 = Vec2f(textureInnerRect.getPos() + textureInnerRect.getSize()) / texSize;
	drawRect(rect, texture, uv0, uv1, tintColor);
}

void Drawer::drawUpscaledRect(const Rectf& rect, DrawerTexture& texture)
{
	addDrawCommand(getPool<UpscaledRectDrawCommand>().createObject(rect, texture));
}

void Drawer::drawSprite(Vec2i position, uint64 spriteKey, const Color& tintColor, Vec2f scale)
{
	addDrawCommand(getPool<SpriteDrawCommand>().createObject(position, spriteKey, 0, tintColor, scale));
}

void Drawer::drawSprite(Vec2i position, uint64 spriteKey, uint64 paletteKey, const Color& tintColor, Vec2f scale)
{
	addDrawCommand(getPool<SpriteDrawCommand>().createObject(position, spriteKey, paletteKey, tintColor, scale));
}

void Drawer::drawSpriteRect(const Recti& rect, uint64 spriteKey, const Color& tintColor)
{
	if (!rect.isEmpty())
	{
		addDrawCommand(getPool<SpriteRectDrawCommand>().createObject(rect, spriteKey, tintColor));
	}
}

void Drawer::drawMesh(const std::vector<DrawerMeshVertex>& triangles, DrawerTexture& texture)
{
	addDrawCommand(getPool<MeshDrawCommand>().createObject(triangles, texture));
}

void Drawer::drawMesh(const std::vector<DrawerMeshVertex_P2_C4>& triangles)
{
	addDrawCommand(getPool<MeshVertexColorDrawCommand>().createObject(triangles));
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
	addDrawCommand(getPool<MeshDrawCommand>().createObject(std::move(triangles), texture));
}

void Drawer::printText(Font& font, const Recti& rect, const String& text, int alignment, Color color)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextDrawCommand>().createObject(font, rect, text, alignment, color));
}

void Drawer::printText(Font& font, const Vec2i& position, const String& text, int alignment, Color color)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextDrawCommand>().createObject(font, Recti(position.x, position.y, 0, 0), text, alignment, color));
}

void Drawer::printText(Font& font, const Recti& rect, const String& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextDrawCommand>().createObject(font, rect, text, printOptions));
}

void Drawer::printText(Font& font, const Vec2i& position, const String& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextDrawCommand>().createObject(font, Recti(position.x, position.y, 0, 0), text, printOptions));
}

void Drawer::printText(Font& font, const Recti& rect, const WString& text, int alignment, Color color)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextWDrawCommand>().createObject(font, rect, text, alignment, color));
}

void Drawer::printText(Font& font, const Vec2i& position, const WString& text, int alignment, Color color)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextWDrawCommand>().createObject(font, Recti(position.x, position.y, 0, 0), text, alignment, color));
}

void Drawer::printText(Font& font, const Recti& rect, const WString& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextWDrawCommand>().createObject(font, rect, text, printOptions));
}

void Drawer::printText(Font& font, const Vec2i& position, const WString& text, const DrawerPrintOptions& printOptions)
{
	if (!text.empty())
		addDrawCommand(getPool<PrintTextWDrawCommand>().createObject(font, Recti(position.x, position.y, 0, 0), text, printOptions));
}

void Drawer::pushScissor(const Recti& rect)
{
	addDrawCommand(getPool<PushScissorDrawCommand>().createObject(rect));
}

void Drawer::popScissor()
{
	addDrawCommand(getPool<PopScissorDrawCommand>().createObject());
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

void Drawer::addDrawCommand(DrawCommand& drawCommand)
{
	mDrawCollection.addDrawCommand(drawCommand);
}
