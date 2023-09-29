/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/drawing/Drawer.h"


DrawerTexture::~DrawerTexture()
{
	invalidate();

	// Unregister
	if (nullptr != mRegisteredOwner)
	{
		mRegisteredOwner->unregisterTexture(*this);
	}
}

void DrawerTexture::invalidate()
{
	delete mImplementation;
	mImplementation = nullptr;
}

void DrawerTexture::setImplementation(DrawerTextureImplementation* implementation)
{
	delete mImplementation;
	mImplementation = implementation;

	if (nullptr != implementation)
	{
		implementation->refreshImplementation(mSetupAsRenderTarget, mSize);
	}
}

void DrawerTexture::clearBitmap()
{
	mBitmap.clear();
	invalidate();
}

Bitmap& DrawerTexture::accessBitmap()
{
	return mBitmap;
}

void DrawerTexture::bitmapUpdated()
{
	mSize.set(mBitmap.getWidth(), mBitmap.getHeight());

	if (nullptr != mImplementation)
	{
		mImplementation->updateFromBitmap(mBitmap);
	}
}

void DrawerTexture::setupAsRenderTarget(uint32 width, uint32 height)
{
	// Any change?
	if (mSetupAsRenderTarget && (uint32)mSize.x == width && (uint32)mSize.y == height)
		return;

	mSize.set(width, height);
	mSetupAsRenderTarget = true;

	if (nullptr != mImplementation)
	{
		mImplementation->setupAsRenderTarget(mSize);
	}
}

void DrawerTexture::writeContentToBitmap(Bitmap& outBitmap)
{
	if (nullptr != mImplementation)
	{
		mImplementation->writeContentToBitmap(outBitmap);
	}
}

void DrawerTexture::swap(DrawerTexture& other)
{
	mBitmap.swap(other.mBitmap);
	std::swap(mSize, other.mSize);
	std::swap(mImplementation, other.mImplementation);
}
