/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class SpriteBase
{
public:
	virtual ~SpriteBase() {}

	struct BlitOptions
	{
		const Recti* mTargetRect = nullptr;
		const float* mTransform = nullptr;
		const float* mInvTransform = nullptr;
		const uint8* mDepthBuffer = nullptr;
		uint8 mDepthValue = 0;
		bool mIgnoreAlpha = false;
		const Vec4f* mTintColor = nullptr;
		const Vec4f* mAddedColor = nullptr;
		bool mUseUpscaledSprite = false;
	};

public:
	virtual Vec2i getSize() const = 0;

protected:
	void blitInto(Bitmap& output, const Bitmap& input, const Vec2i& position, const BlitOptions& blitOptions) const;

public:
	Vec2i mOffset;		// Offset of upper left corner relative to pivot
};
