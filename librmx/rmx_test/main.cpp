/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "StdAfx.h"
#include <chrono>


int main(int argc, char** argv)
{
	INIT_RMX;

	std::string_view sv;
	String g = sv;

	FTX::System->initialize();
	FTX::Video->initialize(rmx::VideoConfig(false, 600, 600, "rmx_test"));

	Shader shader;
	shader.load("../rmx_test/simple_p2_c3.shader");
	shader.bind();

	const GLfloat data[] =
	{
		-1.0f, -1.0f,	  1.0f, 0.0f, 0.0f,
		-1.0f,  1.0f,	  0.0f, 1.0f, 0.0f,
		 1.0f, -1.0f,	  0.0f, 0.0f, 1.0f,
		-1.0f,  1.0f,	  0.0f, 1.0f, 0.0f,
		 1.0f, -1.0f,	  0.0f, 0.0f, 1.0f,
		 1.0f,  1.0f,	  1.0f, 1.0f, 1.0f
	};

	opengl::VertexArrayObject vao1;
	vao1.setup(opengl::VertexArrayObject::Format::P2_C3);
	vao1.updateVertexData(data, 6);

	opengl::VertexArrayObject vao2;
	vao2.setup(opengl::VertexArrayObject::Format::P2_C3);

	int count = 0;
	while (true)
	{
		glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		vao1.bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		{
			float size = 1.0f - std::cos((float)count / 30.0f) * 0.3f;
			const float angle = (float)count / 4.0f;
			Vec2f pos1 = Vec2f(-0.88f, -0.5f) * size;
			Vec2f pos2 = Vec2f( 0.88f, -0.5f) * size;
			Vec2f pos3 = Vec2f( 0.0f,   1.0f) * size;
			pos1.rotate(angle);
			pos2.rotate(angle);
			pos3.rotate(angle);
			const GLfloat data[] =
			{
				pos1.x, pos1.y,	  1.0f, 1.0f, 0.0f,
				pos2.x, pos2.y,	  0.0f, 1.0f, 1.0f,
				pos3.x, pos3.y,	  1.0f, 0.0f, 1.0f
			};

			vao2.updateVertexData(data, 3);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		SDL_GL_SwapWindow(FTX::Video->getMainWindow());

		SDL_Delay(20);
		++count;
	}

	FTX::System->exit();

	return 0;
}
