/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/drawing/DrawCollection.h"
#include "oxygen/drawing/DrawCommand.h"


class DrawerInterface;
class DrawCollection;
class DrawerTexture;
struct SDL_Window;

class Drawer
{
friend class DrawerTexture;

public:
	enum class Type
	{
		SOFTWARE,
		OPENGL
	};

public:
	Drawer();
	~Drawer();

	Type getType() const;

	template<typename T>
	bool createDrawer()
	{
		destroyDrawer();
		mActiveDrawer = new T();
		return onDrawerCreated();
	}

	void destroyDrawer();

	void shutdown();

	inline DrawerInterface* getActiveDrawer() const  { return mActiveDrawer; }

	void createTexture(DrawerTexture& outTexture);

	void setRenderTarget(DrawerTexture& texture, const Recti& rect);
	void setWindowRenderTarget(const Recti& rect);
	void setBlendMode(DrawerBlendMode blendMode);
	void setSamplingMode(DrawerSamplingMode samplingMode);
	void setWrapMode(DrawerWrapMode wrapMode);

	void drawRect(const Rectf& rect, const Color& color);
	void drawRect(const Rectf& rect, DrawerTexture& texture);
	void drawRect(const Rectf& rect, DrawerTexture& texture, const Color& tintColor);
	void drawRect(const Rectf& rect, DrawerTexture& texture, const Vec2f& uv0, const Vec2f& uv1, const Color& tintColor);
	void drawUpscaledRect(const Rectf& rect, DrawerTexture& texture);
	void drawMesh(const std::vector<DrawerMeshVertex>& triangles, DrawerTexture& texture);
	void drawMesh(const std::vector<DrawerMeshVertex_P2_C4>& triangles);
	void drawQuad(const DrawerMeshVertex* quad, DrawerTexture& texture);

	void printText(Font& font, const Recti& rect, const String& text, int alignment = 1, Color color = Color::WHITE);
	void printText(Font& font, const Recti& rect, const String& text, const rmx::Painter::PrintOptions& printOptions);
	void printText(Font& font, const Recti& rect, const WString& text, int alignment = 1, Color color = Color::WHITE);
	void printText(Font& font, const Recti& rect, const WString& text, const rmx::Painter::PrintOptions& printOptions);

	void pushScissor(const Recti& rect);
	void popScissor();

	void setupRenderWindow(SDL_Window* window);
	void performRendering();
	void presentScreen();

private:
	bool onDrawerCreated();
	void unregisterTexture(DrawerTexture& texture);

private:
	DrawerInterface* mActiveDrawer = nullptr;
	DrawCollection mDrawCollection;
	std::vector<DrawerTexture*> mDrawerTextures;
};
