/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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

	void blitInto(Bitmap& output, const Vec2i& position, const BlitOptions& blitOptions = BlitOptions()) const;

	inline const Bitmap& getBitmap() const  { return mBitmap; }
	inline Bitmap& accessBitmap()  { return mBitmap; }

private:
	Bitmap mBitmap;
};
