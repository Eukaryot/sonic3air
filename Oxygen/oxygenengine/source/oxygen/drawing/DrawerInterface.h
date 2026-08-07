/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/Drawer.h"


class DrawCollection;
class DrawerTexture;

class DrawerInterface
{
public:
	virtual ~DrawerInterface() {}

	virtual Drawer::Type getType() = 0;
	virtual bool wasSetupSuccessful()  { return true; }

	virtual void updateDrawer(float deltaSeconds) {}

	virtual void createTexture(DrawerTexture& outTexture) = 0;
	virtual void refreshTexture(DrawerTexture& texture) = 0;
	virtual void setupRenderWindow(SDL_Window* window) = 0;
	virtual void performRendering(const DrawCollection& drawCollection) = 0;
	virtual void presentScreen() = 0;
};
