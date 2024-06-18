/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

namespace opengl
{
	VertexArrayObject::~VertexArrayObject()
	{
		if (mHandle != 0)
		{
			glDeleteVertexArrays(1, &mHandle);
			glDeleteBuffers(1, &mVertexBufferObjectHandle);
		}
	}

	void VertexArrayObject::setup(Format format)
	{
		const bool needsInitialization = (mHandle == 0);
		if (needsInitialization)
		{
			glGenVertexArrays(1, &mHandle);
			glGenBuffers(1, &mVertexBufferObjectHandle);
			glBindVertexArray(mHandle);
			glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObjectHandle);
		}
		else
		{
			glBindVertexArray(mHandle);
		}

		mCurrentFormat = format;
		switch (format)
		{
			case Format::P2:
			{
				mNumVertexAttributes = 1;
				mFloatsPerVertex = 2;
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(0 * sizeof(float)));	// Positions
				break;
			}

			case Format::P2_C3:
			{
				mNumVertexAttributes = 2;
				mFloatsPerVertex = 5;
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(0 * sizeof(float)));	// Positions
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(2 * sizeof(float)));	// Colors
				break;
			}

			case Format::P2_C4:
			{
				mNumVertexAttributes = 2;
				mFloatsPerVertex = 6;
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(0 * sizeof(float)));	// Positions
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(2 * sizeof(float)));	// Colors
				break;
			}

			case Format::P2_T2:
			{
				mNumVertexAttributes = 2;
				mFloatsPerVertex = 4;
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(0 * sizeof(float)));	// Positions
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(2 * sizeof(float)));	// Texcoords
				break;
			}

			case Format::P3_C3:
			{
				mNumVertexAttributes = 2;
				mFloatsPerVertex = 6;
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(0 * sizeof(float)));	// Positions
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(3 * sizeof(float)));	// Colors
				break;
			}

			case Format::P3_N3_C3:
			{
				mNumVertexAttributes = 3;
				mFloatsPerVertex = 9;
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(0 * sizeof(float)));	// Positions
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE,  (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(3 * sizeof(float)));	// Normals
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (GLsizei)(mFloatsPerVertex * sizeof(float)), (void*)(6 * sizeof(float)));	// Colors
				break;
			}

			default:
				RMX_ERROR("Unrecognized or invalid format", );
				break;
		}

		for (size_t i = 0; i < mNumVertexAttributes; ++i)
		{
			glEnableVertexAttribArray((GLuint)i);
		}
		if (!needsInitialization)
		{
			for (size_t i = mNumVertexAttributes; i < 4; ++i)	// Assuming we'll never use more than 4 vertex attributes inside here
			{
				glDisableVertexAttribArray((GLuint)i);
			}
		}
	}

	void VertexArrayObject::updateVertexData(const float* vertexData, size_t numVertices)
	{
		if (mHandle == 0)
		{
			RMX_ASSERT(false, "VAO must be setup with a format before updating data");
			return;
		}

		glBindVertexArray(mHandle);
		glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObjectHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(mFloatsPerVertex * numVertices * sizeof(GLfloat)), vertexData, GL_STATIC_DRAW);
		mNumBufferedVertices = numVertices;
	}

	void VertexArrayObject::bind() const
	{
		if (mHandle != 0)
		{
			glBindVertexArray(mHandle);
		}
	}

	void VertexArrayObject::unbind() const
	{
		glBindVertexArray(0);
	}

	void VertexArrayObject::draw(GLenum mode) const
	{
		if (mHandle != 0 && mNumBufferedVertices > 0)
		{
			glBindVertexArray(mHandle);
			glDrawArrays(mode, 0, (GLsizei)mNumBufferedVertices);
		}
	}
}

#endif
