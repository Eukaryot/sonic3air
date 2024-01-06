/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"
#include <rmxmedia.h>


class BackdropView : public GuiBase
{
public:
	void setGameViewRect(const Recti& rect);

	virtual void initialize() override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

private:
	DrawerTexture mBackdropTexture;
	int mCachedBackdropSetting = 0;
	Recti mCachedScreenRect;
	Recti mCachedGameViewRect;
	std::vector<Recti> mRenderRects;

	float mAnimationTime = 0.0f;
	float mColorMultiplier = 1.0f;
	bool mHasBlackTimeout = false;
	float mBlackTimeout = 0.0f;
};
