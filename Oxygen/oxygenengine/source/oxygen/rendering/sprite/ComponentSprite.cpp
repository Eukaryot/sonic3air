/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/sprite/ComponentSprite.h"


void ComponentSprite::clear()
{
	mBitmap.clear(0);
}

void ComponentSprite::blitInto(Bitmap& output, const Vec2i& position, const BlitOptions& blitOptions) const
{
	SpriteBase::blitInto(output, mBitmap, position, blitOptions);
}
