/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"
#include "oxygen/helper/FileHelper.h"


namespace openglresources
{
	enum Variant
	{
		VARIANT_STANDARD			= 0,
		VARIANT_TINTCOLOR			= 1,
		VARIANT_ALPHATEST			= 2,
		VARIANT_TINTCOLOR_ALPHATEST = 3
	};
	const char* variantString[4] = { "Standard", "TintColor", "Standard_AlphaTest", "TintColor_AlphaTest" };

	struct Internal
	{
		Shader mSimpleRectColoredShader;
		Shader mSimpleRectVertexColorShader;
		Shader mSimpleRectTexturedShader[4];		// Enumerated using enum Variant
		Shader mSimpleRectTexturedUVShader[4];		// Enumerated using enum Variant
		opengl::VertexArrayObject mSimpleQuadVAO;
	};
	Internal* mInternal = nullptr;
}


void OpenGLDrawerResources::startup()
{
	if (nullptr != openglresources::mInternal)
		return;

	openglresources::mInternal = new openglresources::Internal();

	// Load shaders
	FileHelper::loadShader(openglresources::mInternal->mSimpleRectColoredShader, L"data/shader/simple_rect_colored.shader", "Standard");
	FileHelper::loadShader(openglresources::mInternal->mSimpleRectVertexColorShader, L"data/shader/simple_rect_vertexcolor.shader", "Standard");
	for (int k = 0; k < 4; ++k)
	{
		FileHelper::loadShader(openglresources::mInternal->mSimpleRectTexturedShader[k], L"data/shader/simple_rect_textured.shader", openglresources::variantString[k]);
		FileHelper::loadShader(openglresources::mInternal->mSimpleRectTexturedUVShader[k], L"data/shader/simple_rect_textured_uv.shader", openglresources::variantString[k]);
	}

	// Setup simple quad VAO, consisting of two triangles
	{
		opengl::VertexArrayObject& vao = openglresources::mInternal->mSimpleQuadVAO;
		const float vertexData[] =
		{
			0.0f, 0.0f,		// Upper left
			0.0f, 1.0f,		// Lower left
			1.0f, 1.0f,		// Lower right
			1.0f, 1.0f,		// Lower right
			1.0f, 0.0f,		// Upper right
			0.0f, 0.0f		// Upper left
		};
		vao.setup(opengl::VertexArrayObject::Format::P2);
		vao.updateVertexData(vertexData, 6);
	}
}

void OpenGLDrawerResources::shutdown()
{
	SAFE_DELETE(openglresources::mInternal);
}

Shader& OpenGLDrawerResources::getSimpleRectColoredShader()
{
	return openglresources::mInternal->mSimpleRectColoredShader;
}

Shader& OpenGLDrawerResources::getSimpleRectVertexColorShader()
{
	return openglresources::mInternal->mSimpleRectVertexColorShader;
}

Shader& OpenGLDrawerResources::getSimpleRectTexturedShader(bool tint, bool alpha)
{
	return openglresources::mInternal->mSimpleRectTexturedShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

Shader& OpenGLDrawerResources::getSimpleRectTexturedUVShader(bool tint, bool alpha)
{
	return openglresources::mInternal->mSimpleRectTexturedUVShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

opengl::VertexArrayObject& OpenGLDrawerResources::getSimpleQuadVAO()
{
	return openglresources::mInternal->mSimpleQuadVAO;
}
