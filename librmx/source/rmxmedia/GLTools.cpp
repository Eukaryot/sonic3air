/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


#ifdef ALLOW_LEGACY_OPENGL

inline const float* _ftx2_internal_GetStdTexcoords()
{
	static const float texcrd[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	return texcrd;
}

void drawRect(const Rectf& rect, const float* texcoords)
{
	if (nullptr == texcoords)
		texcoords = _ftx2_internal_GetStdTexcoords();

	glBegin(GL_QUADS);
		glTexCoord2f(texcoords[0], texcoords[1]);  glVertex2f(rect.left, rect.top);
		glTexCoord2f(texcoords[0], texcoords[3]);  glVertex2f(rect.left, rect.top + rect.height);
		glTexCoord2f(texcoords[2], texcoords[3]);  glVertex2f(rect.left + rect.width, rect.top + rect.height);
		glTexCoord2f(texcoords[2], texcoords[1]);  glVertex2f(rect.left + rect.width, rect.top);
	glEnd();
}

void drawRect(const Rectf& rect, const Texture& texture, const float* texcoords)
{
	texture.bind();
	drawRect(rect, texcoords);
}

void drawRect(const Rectf& rect, const Texture& texture, float borderX, float borderY, const float* texcoords)
{
	texture.bind();

	if (nullptr == texcoords)
		texcoords = _ftx2_internal_GetStdTexcoords();

	float vertex_x[4] = { rect.x, rect.x+borderX, rect.x+rect.width-borderX,  rect.x+rect.width };
	float vertex_y[4] = { rect.y, rect.y+borderY, rect.y+rect.height-borderY, rect.y+rect.height };

	float bx = (borderX / (float)texture.getWidth()) * (texcoords[2] - texcoords[0]);
	float by = (borderY / (float)texture.getHeight()) * (texcoords[3] - texcoords[1]);
	float texcrd_x[4] = { texcoords[0], texcoords[0]+bx, texcoords[2]-bx, texcoords[2] };
	float texcrd_y[4] = { texcoords[1], texcoords[1]+by, texcoords[3]-by, texcoords[3] };

	drawQuadPatch(4, 4, vertex_x, vertex_y, texcrd_x, texcrd_y);
}

void drawQuadPatch(int numVertX, int numVertY, float* verticesX, float* verticesY, float* texcrdsX, float* texcrdsY)
{
	for (int y = 0; y < numVertY-1; ++y)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < numVertX; ++x)
		{
			glTexCoord2f(texcrdsX[x], texcrdsY[y]);    glVertex2f(verticesX[x], verticesY[y]);
			glTexCoord2f(texcrdsX[x], texcrdsY[y+1]);  glVertex2f(verticesX[x], verticesY[y+1]);
		}
		glEnd();
	}
}

#endif



std::string getGLErrorDescription(GLenum err)
{
	switch (err)
	{
		case GL_NONE:							return "NONE";
		case GL_INVALID_OPERATION:				return "INVALID_OPERATION";
		case GL_INVALID_ENUM:					return "INVALID_ENUM";
		case GL_INVALID_VALUE:					return "INVALID_VALUE";
		case GL_OUT_OF_MEMORY:					return "OUT_OF_MEMORY";
		case GL_INVALID_FRAMEBUFFER_OPERATION:	return "INVALID_FRAMEBUFFER_OPERATION";
	}
	return rmx::hexString((int)err, 4);
}
