/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/DrawerInterface.h"

class OpenGLDrawerResources;
namespace opengldrawer
{
	struct Internal;
}


class OpenGLDrawer final : public DrawerInterface
{
public:
	OpenGLDrawer();
	~OpenGLDrawer();

	inline Drawer::Type getType() override  { return Drawer::Type::OPENGL; }
	bool wasSetupSuccessful() override;

	void updateDrawer(float deltaSeconds) override;

	void createTexture(DrawerTexture& outTexture) override;
	void refreshTexture(DrawerTexture& texture) override;
	void setupRenderWindow(SDL_Window* window) override;
	void performRendering(const DrawCollection& drawCollection) override;
	void presentScreen() override;

	OpenGLDrawerResources& getResources();

private:
	opengldrawer::Internal& mInternal;
};

#endif
