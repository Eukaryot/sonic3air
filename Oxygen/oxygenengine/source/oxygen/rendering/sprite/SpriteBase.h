/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
		uint8* mDepthBuffer = nullptr;
		uint8 mDepthValue = 0;
		bool mIgnoreAlpha = false;
		const Color* mTintColor = nullptr;
		const Color* mAddedColor = nullptr;
		bool mUseUpscaledSprite = false;
	};

public:
	virtual Vec2i getSize() const = 0;

public:
	Vec2i mOffset;		// Offset of upper left corner relative to pivot (i.e. it's usually negative coordinates)
};
