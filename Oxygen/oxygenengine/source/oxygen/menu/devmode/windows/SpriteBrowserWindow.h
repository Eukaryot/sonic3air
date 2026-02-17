/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/devmode/DevModeWindowBase.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/resources/SpriteCollection.h"


class SpriteBrowserWindow : public DevModeWindowBase
{
public:
	SpriteBrowserWindow();

	virtual void buildContent() override;

private:
	uint32 mLastSpriteCollectionChangeCounter = 0;
	std::vector<const SpriteCollection::Item*> mSortedItems;

	const SpriteCollection::Item* mPreviewItem = nullptr;
	int mPreviewScale = 0;
	DrawerTexture mPreviewTexture;
	bool mHasValidPreviewTexture = false;
	bool mShowPalette = false;
};

#endif
