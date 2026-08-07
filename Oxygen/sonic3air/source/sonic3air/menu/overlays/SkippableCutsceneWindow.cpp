/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/overlays/SkippableCutsceneWindow.h"
#include "sonic3air/menu/SharedResources.h"

#include "oxygen/application/EngineMain.h"


void SkippableCutsceneWindow::initialize()
{
}

void SkippableCutsceneWindow::deinitialize()
{
}

void SkippableCutsceneWindow::update(float timeElapsed)
{
	if (mShouldBeVisible)
	{
		mVisibility = saturate(mVisibility + timeElapsed * 20.0f);
	}
	else
	{
		mVisibility = saturate(mVisibility - timeElapsed * 10.0f);
	}
	mAnimationTimer += timeElapsed;
}

void SkippableCutsceneWindow::render()
{
	Drawer& drawer = EngineMain::instance().getDrawer();

	const Recti rect(mRect.width - 140, 0, 140, 14);
	const Color color(1.0f, 1.0f - std::fmod(mAnimationTimer * 2.0f, 1.0f) * 0.5f, 1.0f - std::fmod(mAnimationTimer * 2.0f, 1.0f), mVisibility);
	drawer.printText(global::mOxyfontTiny, rect, "Fast forwarding cutscene", 5, color);
}

void SkippableCutsceneWindow::show(bool visible)
{
	if (mShouldBeVisible == visible)
		return;

	mShouldBeVisible = visible;
	if (visible)
	{
		mAnimationTimer = 0.0f;
	}
}
