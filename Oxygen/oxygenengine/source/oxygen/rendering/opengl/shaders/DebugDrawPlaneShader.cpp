/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/DebugDrawPlaneShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"


void DebugDrawPlaneShader::initialize()
{
	if (BufferTexture::supportsBufferTextures())	// Buffer texture support is required for this shader
	{
		if (FileHelper::loadShader(mShader, L"data/shader/debugdraw_plane.shader", "Standard", "USE_BUFFER_TEXTURES"))
		{
			bindShader();

			mLocPlayfieldSize = mShader.getUniformLocation("PlayfieldSize");
			mLocHighlightPrio = mShader.getUniformLocation("HighlightPrio");

			mShader.setParam("IndexTexture", 0);
			mShader.setParam("PatternCacheTexture", 1);
			mShader.setParam("PaletteTexture", 2);
		}
	}
}

void DebugDrawPlaneShader::draw(int planeIndex, RenderParts& renderParts, const OpenGLRenderResources& resources)
{
	bindShader();

	// Bind textures
	{
		glActiveTexture(GL_TEXTURE0);
		resources.getPlanePatternsTexture(planeIndex).bindTexture();

		glActiveTexture(GL_TEXTURE1);
		resources.getPatternCacheTexture().bindTexture();

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, resources.getMainPaletteTexture().getHandle());
	}

	// Update uniforms
	{
		if (planeIndex <= PlaneManager::PLANE_A)
			mShader.setParam(mLocPlayfieldSize, renderParts.getPlaneManager().getPlayfieldSizeForShaders());
		else
			mShader.setParam(mLocPlayfieldSize, Vec4i(512, 256, 64, 32));

		mShader.setParam(mLocHighlightPrio, FTX::keyState(SDLK_LSHIFT) ? 1 : 0);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);

	mShader.unbind();
}

#endif
