/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

// OpenGL ES 2 does not support actual vertex array objects - but despite its name, the VertexArrayObject class will still without them (though a bit less efficient in that case)
#if !defined(RMX_USE_GLES2)
	#define RMX_OPENGL_SUPPORT_VAO
#endif

namespace opengl
{

	VertexArrayObject::~VertexArrayObject()
	{
	#ifdef RMX_OPENGL_SUPPORT_VAO
		if (mVertexArrayObjectHandle != 0)
			glDeleteVertexArrays(1, &mVertexArrayObjectHandle);
	#endif
	
		if (mVertexBufferObjectHandle != 0)
			glDeleteBuffers(1, &mVertexBufferObjectHandle);
	}

	void VertexArrayObject::setup(Format format)
	{
		const bool needsInitialization = (mVertexBufferObjectHandle == 0);
	#ifdef RMX_OPENGL_SUPPORT_VAO
		if (needsInitialization)
		{
			glGenVertexArrays(1, &mVertexArrayObjectHandle);
			glGenBuffers(1, &mVertexBufferObjectHandle);
			glBindVertexArray(mVertexArrayObjectHandle);
			glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObjectHandle);
		}
		else
		{
			glBindVertexArray(mVertexArrayObjectHandle);
		}
	#else
		if (needsInitialization)
		{
			glGenBuffers(1, &mVertexBufferObjectHandle);
			glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObjectHandle);
		}
	#endif

		mCurrentFormat = format;
		applyCurrentFormat();
	}

	void VertexArrayObject::updateVertexData(const float* vertexData, size_t numVertices)
	{
		if (mVertexBufferObjectHandle == 0)
		{
			RMX_ASSERT(false, "VAO must be setup with a format before updating data");
			return;
		}

	#ifdef RMX_OPENGL_SUPPORT_VAO
		glBindVertexArray(mVertexArrayObjectHandle);
	#endif
		glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObjectHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(mFloatsPerVertex * numVertices * sizeof(GLfloat)), vertexData, GL_STATIC_DRAW);
		mNumBufferedVertices = numVertices;
	}

	void VertexArrayObject::bind()
	{
	#ifdef RMX_OPENGL_SUPPORT_VAO
		// Bind the VAO, which will implicitly bind the VBO
		if (mVertexArrayObjectHandle != 0)
		{
			glBindVertexArray(mVertexArrayObjectHandle);
		}
	#else
		// Explicitly bind the VAO, and apply the format
		if (mVertexBufferObjectHandle != 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObjectHandle);
			applyCurrentFormat();
		}
	#endif
	}

	void VertexArrayObject::unbind() const
	{
	#ifdef RMX_OPENGL_SUPPORT_VAO
		glBindVertexArray(0);
	#else
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	#endif
	}

	void VertexArrayObject::draw(GLenum mode)
	{
		if (mNumBufferedVertices > 0)
		{
			bind();
			glDrawArrays(mode, 0, (GLsizei)mNumBufferedVertices);
		}
	}

	void VertexArrayObject::applyCurrentFormat()
	{
		switch (mCurrentFormat)
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
		for (size_t i = mNumVertexAttributes; i < 4; ++i)	// Assuming we'll never use more than 4 vertex attributes inside here
		{
			glDisableVertexAttribArray((GLuint)i);
		}
	}

}

#endif
