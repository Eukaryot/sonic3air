/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/hardware/shaders/DebugDrawPlaneShader.h"
#include "oxygen/rendering/hardware/HardwareRenderResources.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"


void DebugDrawPlaneShader::initialize()
{
	const std::string additionalDefines = BufferTexture::supportsBufferTextures() ? "USE_BUFFER_TEXTURES" : "";
	FileHelper::loadShader(mShader, L"data/shader/debugdraw_plane.shader", "Standard", additionalDefines);

	mLocPlayfieldSize	= mShader.getUniformLocation("PlayfieldSize");
	mLocIndexTex		= mShader.getUniformLocation("IndexTexture");
	mLocPatternCacheTex	= mShader.getUniformLocation("PatternCacheTexture");
	mLocPaletteTex		= mShader.getUniformLocation("PaletteTexture");
	mLocHighlightPrio	= mShader.getUniformLocation("HighlightPrio");
}

void DebugDrawPlaneShader::draw(int planeIndex, RenderParts& renderParts, const HardwareRenderResources& resources)
{
	mShader.bind();

	if (planeIndex <= PlaneManager::PLANE_A)
		glUniform4iv(mLocPlayfieldSize, 1, *renderParts.getPlaneManager().getPlayfieldSizeForShaders());
	else
		glUniform4iv(mLocPlayfieldSize, 1, *Vec4i(512, 256, 64, 32));

	glActiveTexture(GL_TEXTURE0);
	resources.mPlanePatternsTexture[planeIndex].bindTexture();
	glUniform1i(mLocIndexTex, 0);

	glActiveTexture(GL_TEXTURE1);
	resources.mPatternCacheTexture.bindTexture();
	glUniform1i(mLocPatternCacheTex, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, resources.mPaletteTexture.getHandle());
	glUniform1i(mLocPaletteTex, 2);

	glUniform1i(mLocHighlightPrio, FTX::keyState(SDLK_LSHIFT) ? 1 : 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	mShader.unbind();
}
