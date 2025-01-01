/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/sprite/SpriteBase.h"


class ComponentSprite : public SpriteBase
{
public:
	void clear();

	inline const Bitmap& getBitmap() const  { return mBitmap; }
	inline Bitmap& accessBitmap()  { return mBitmap; }

	inline Vec2i getSize() const override  { return mBitmap.getSize(); }

private:
	Bitmap mBitmap;
};
