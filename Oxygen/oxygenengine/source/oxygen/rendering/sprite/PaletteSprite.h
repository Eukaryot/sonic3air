/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/sprite/SpriteBase.h"
#include "oxygen/rendering/utils/PaletteBitmap.h"
#include "oxygen/rendering/utils/RenderUtils.h"


class PaletteSprite : public SpriteBase
{
public:
	void clear();
	void create(const Vec2i& size, const Vec2i& offset);
	void createFromBitmap(const PaletteBitmap& bitmap, const Vec2i& offset);
	void createFromBitmap(PaletteBitmap&& bitmap, const Vec2i& offset);
	void createFromBitmap(const PaletteBitmap& bitmap, const Recti& sourceRect, const Vec2i& offset);
	void createFromSpritePatterns(const std::vector<RenderUtils::SinglePattern>& patterns);

	void blitInto(PaletteBitmap& output, const Vec2i& position) const;
	void blitInto(Bitmap& output, const Vec2i& position, const uint32* palette, const BlitOptions& blitOptions = BlitOptions()) const;

	inline PaletteBitmap& accessBitmap()  { return mBitmap; }
	inline const PaletteBitmap& getBitmap() const  { return mBitmap; }
	const PaletteBitmap& getUpscaledBitmap() const;

private:
	PaletteBitmap mBitmap;
	mutable PaletteBitmap mUpscaledBitmap;	// Used for smoother rotation
};
