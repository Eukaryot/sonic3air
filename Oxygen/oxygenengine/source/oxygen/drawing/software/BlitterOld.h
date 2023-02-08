/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class BlitterOld
{
public:
	struct Options
	{
		// Note: Not all options are supported everywhere; and in some cases, there's specific BlitterOld functions for certain options
		bool mUseAlphaBlending = false;
		bool mUseBilinearSampling = false;
		Color mTintColor = Color::WHITE;
	};

public:
	static void blitColor(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const Color& color, const Options& options);
	static void blitBitmap(BitmapViewMutable<uint32>& destBitmap, Vec2i destPosition, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options);
	static void blitBitmapWithScaling(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options);
	static void blitBitmapWithUVs(BitmapViewMutable<uint32>& destBitmap, Recti destRect, const BitmapViewMutable<uint32>& sourceBitmap, Recti sourceRect, const Options& options);
};
