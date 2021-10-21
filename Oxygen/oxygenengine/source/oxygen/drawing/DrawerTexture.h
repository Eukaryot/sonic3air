/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Drawer;
class DrawerTexture;


class DrawerTextureImplementation
{
public:
	virtual ~DrawerTextureImplementation() {}

	virtual void updateFromBitmap(const Bitmap& bitmap) = 0;
	virtual void setupAsRenderTarget(const Vec2i& size, DrawerTexture& owner) = 0;
	virtual void writeContentToBitmap(Bitmap& outBitmap) = 0;
	virtual void refreshImplementation(DrawerTexture& owner, bool setupRenderTarget, const Vec2i& size) = 0;
};


class DrawerTexture
{
friend class Drawer;

public:
	~DrawerTexture();

	inline bool isValid() const		{ return (nullptr != mImplementation); }
	void invalidate();

	template<typename T>
	T* getImplementation() const	{ return static_cast<T*>(mImplementation); }
	void setImplementation(DrawerTextureImplementation* implementation);

	const Vec2i& getSize() const	{ return mSize; }
	int getWidth() const			{ return mSize.x; }
	int getHeight() const			{ return mSize.y; }

	Bitmap& accessBitmap();
	void bitmapUpdated();
	void setupAsRenderTarget(uint32 width, uint32 height);
	void writeContentToBitmap(Bitmap& outBitmap);

	void swap(DrawerTexture& other);

private:
	Drawer* mRegisteredOwner = nullptr;
	size_t mRegisteredIndex = 0;

	Bitmap mBitmap;		// Holding the texture content, except if this is a hardware render target
	Vec2i mSize;		// Resolution of the texture -- either the size of the bitmap or of a hardware render target
	bool mSetupAsRenderTarget = false;

	DrawerTextureImplementation* mImplementation = nullptr;
};
