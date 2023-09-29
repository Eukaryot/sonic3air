/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"


class SoftwareDrawerTexture final : public DrawerTextureImplementation
{
public:
	inline explicit SoftwareDrawerTexture(DrawerTexture& owner) : DrawerTextureImplementation(owner) {}

	void updateFromBitmap(const Bitmap& bitmap) override;
	void setupAsRenderTarget(const Vec2i& size) override;
	void writeContentToBitmap(Bitmap& outBitmap) override;
	void refreshImplementation(bool setupRenderTarget, const Vec2i& size) override;
};
