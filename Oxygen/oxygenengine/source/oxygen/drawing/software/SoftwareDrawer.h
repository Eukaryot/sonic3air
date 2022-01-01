/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerInterface.h"


namespace softwaredrawer
{
	struct Internal;
}


class SoftwareDrawer final : public DrawerInterface
{
public:
	SoftwareDrawer();
	~SoftwareDrawer();

public:
	inline Drawer::Type getType() override  { return Drawer::Type::SOFTWARE; }

	void createTexture(DrawerTexture& outTexture) override;
	void refreshTexture(DrawerTexture& texture) override;
	void setupRenderWindow(SDL_Window* window) override;
	void performRendering(const DrawCollection& drawCollection) override;
	void presentScreen() override;

private:
	softwaredrawer::Internal& mInternal;
};
