/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class RenderParts;
class Geometry;
class DrawerTexture;

class Renderer
{
public:
	inline Renderer(uint8 rendererType, RenderParts& renderParts, DrawerTexture& outputTexture) :
		mRenderParts(renderParts), mGameScreenTexture(outputTexture), mRendererType(rendererType) {}
	inline virtual ~Renderer() {}

	inline uint8 getRendererType() const  { return mRendererType; }
	inline DrawerTexture& getOutputTexture() const  { return mGameScreenTexture; }

	virtual void initialize() = 0;
	virtual void reset() = 0;
	virtual void setGameResolution(const Vec2i& gameResolution) = 0;
	virtual void clearGameScreen() = 0;
	virtual void renderGameScreen(const std::vector<Geometry*>& geometries) = 0;
	virtual void renderDebugDraw(int debugDrawMode, const Recti& rect) = 0;

protected:
	RenderParts& mRenderParts;
	DrawerTexture& mGameScreenTexture;

private:
	uint8 mRendererType;
};
