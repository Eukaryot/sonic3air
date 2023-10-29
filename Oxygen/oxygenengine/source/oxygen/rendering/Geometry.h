/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/SpriteManager.h"


class Geometry
{
public:
	enum class Type
	{
		UNDEFINED = 0,
		PLANE,
		SPRITE,
		RECT,
		TEXTURED_RECT,
		EFFECT_BLUR,
		VIEWPORT
	};

public:
	inline virtual ~Geometry() {}

	inline Type getType() const  { return mType; }

	template<typename T> T& as()			  { return static_cast<T&>(*this); }
	template<typename T> const T& as() const  { return static_cast<const T&>(*this); }

public:
	uint16 mRenderQueue = 0;

protected:
	inline Geometry(Type type) : mType(type) {}

private:
	Type mType;
};



class PlaneGeometry : public Geometry
{
public:
	PlaneGeometry(const Recti& activeRect, int planeIndex, bool priorityFlag, uint8 scrollOffsets, uint16 renderQueue);

public:
	int mPlaneIndex = 0;
	bool mPriorityFlag = false;
	Recti mActiveRect;
	uint8 mScrollOffsets = 0;
};


class SpriteGeometry : public Geometry
{
public:
	SpriteGeometry(const SpriteManager::SpriteInfo& spriteInfo);

public:
	const SpriteManager::SpriteInfo& mSpriteInfo;
};


class RectGeometry : public Geometry
{
public:
	inline RectGeometry(const Recti& rect, const Color& color) : Geometry(Type::RECT), mRect(rect), mColor(color) {}

public:
	Recti mRect;
	Color mColor;
};


class TexturedRectGeometry : public Geometry
{
public:
	inline TexturedRectGeometry(const Recti& rect, DrawerTexture& drawerTexture, const Color& color, bool useGlobalComponentTint) : Geometry(Type::TEXTURED_RECT), mRect(rect), mDrawerTexture(drawerTexture), mColor(color), mUseGlobalComponentTint(useGlobalComponentTint) {}

public:
	Recti mRect;
	DrawerTexture& mDrawerTexture;
	Color mColor;
	bool mUseGlobalComponentTint;
};


class EffectBlurGeometry : public Geometry
{
public:
	inline EffectBlurGeometry(int blurValue) : Geometry(Type::EFFECT_BLUR), mBlurValue(blurValue) {}

public:
	int mBlurValue;
};


class ViewportGeometry : public Geometry
{
public:
	inline ViewportGeometry(const Recti& rect) : Geometry(Type::VIEWPORT), mRect(rect) {}

public:
	Recti mRect;
};


class GeometryFactory
{
public:
	PlaneGeometry& createPlaneGeometry(const Recti& activeRect, int planeIndex, bool priorityFlag, uint8 scrollOffsets, uint16 renderQueue)
	{
		return mPlaneGeometryBuffer.createObject(activeRect, planeIndex, priorityFlag, scrollOffsets, renderQueue);
	}

	SpriteGeometry& createSpriteGeometry(const SpriteManager::SpriteInfo& spriteInfo)
	{
		return mSpriteGeometryBuffer.createObject(spriteInfo);
	}

	RectGeometry& createRectGeometry(const Recti& rect, const Color& color)
	{
		return mRectGeometryBuffer.createObject(rect, color);
	}

	TexturedRectGeometry& createTexturedRectGeometry(const Recti& rect, DrawerTexture& drawerTexture, const Color& color, bool useGlobalComponentTint)
	{
		return mTexturedRectGeometryBuffer.createObject(rect, drawerTexture, color, useGlobalComponentTint);
	}

	EffectBlurGeometry& createEffectBlurGeometry(int blurValue)
	{
		return mEffectBlurGeometryBuffer.createObject(blurValue);
	}

	ViewportGeometry& createViewportGeometry(const Recti& rect)
	{
		return mViewportGeometryBuffer.createObject(rect);
	}

	void destroy(Geometry& geometry)
	{
		switch (geometry.getType())
		{
			case Geometry::Type::UNDEFINED:		break;	// This should never happen anyways
			case Geometry::Type::PLANE:			mPlaneGeometryBuffer.destroyObject(static_cast<PlaneGeometry&>(geometry));				 break;
			case Geometry::Type::SPRITE:		mSpriteGeometryBuffer.destroyObject(static_cast<SpriteGeometry&>(geometry));			 break;
			case Geometry::Type::RECT:			mRectGeometryBuffer.destroyObject(static_cast<RectGeometry&>(geometry));				 break;
			case Geometry::Type::TEXTURED_RECT:	mTexturedRectGeometryBuffer.destroyObject(static_cast<TexturedRectGeometry&>(geometry)); break;
			case Geometry::Type::EFFECT_BLUR:	mEffectBlurGeometryBuffer.destroyObject(static_cast<EffectBlurGeometry&>(geometry));	 break;
			case Geometry::Type::VIEWPORT:		mViewportGeometryBuffer.destroyObject(static_cast<ViewportGeometry&>(geometry));		 break;
		}
	}

private:
	ObjectPool<PlaneGeometry, 16>		 mPlaneGeometryBuffer;
	ObjectPool<SpriteGeometry, 64>		 mSpriteGeometryBuffer;
	ObjectPool<RectGeometry, 64>		 mRectGeometryBuffer;
	ObjectPool<TexturedRectGeometry, 64> mTexturedRectGeometryBuffer;
	ObjectPool<EffectBlurGeometry, 4>	 mEffectBlurGeometryBuffer;
	ObjectPool<ViewportGeometry, 4>		 mViewportGeometryBuffer;
};
