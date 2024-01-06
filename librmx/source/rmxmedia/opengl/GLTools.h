/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	GLTools
*		Helper functions for OpenGL.
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

	inline void glEnable_Toggle(GLenum cap, bool enable)	{ if (enable) glEnable(cap); else glDisable(cap); }
	inline void glViewport_Recti(Recti rect)				{ glViewport(rect.left, rect.top, rect.width, rect.height); }

	#ifdef ALLOW_LEGACY_OPENGL
		inline void glRectf(Rectf rect)					{ glRectf(rect.left, rect.top, rect.left + rect.width, rect.top + rect.height); }
		inline void glTranslatefv(const GLfloat* vec)	{ glTranslatef(vec[0], vec[1], vec[2]); }
		inline void glTranslatefv(const Vec3f& vec)		{ glTranslatef(vec.x, vec.y, vec.z); }
		inline void glScalef(GLfloat scale)				{ glScalef(scale, scale, scale); }
		inline void glColor1f(GLfloat gray)				{ glColor3f(gray, gray, gray); }
		inline void glColorAlpha(GLfloat alpha)			{ glColor4f(1.0f, 1.0f, 1.0f, alpha); }
		inline void glColor(const Vec3f& color)			{ glColor3fv(*color); }
		inline void glColor(const Vec4f& color)			{ glColor4fv(*color); }
		inline void glColor(const Color& color)			{ glColor4fv(color.data); }

		void drawRect(const Rectf& rect, const float* texcoords = nullptr);
		void drawRect(const Rectf& rect, const Texture& texture, const float* texcoords = nullptr);
		void drawRect(const Rectf& rect, const Texture& texture, float borderX, float borderY, const float* texcoords = nullptr);
		void drawQuadPatch(int numVertX, int numVertY, float* verticesX, float* verticesY, float* texcrdsX, float* texcrdsY);
	#endif


	std::string getGLErrorDescription(GLenum err);


	#ifdef ALLOW_LEGACY_OPENGL
		// DisplayList
		class API_EXPORT DisplayList
		{
		public:
			~DisplayList()	{ clear(); }

			bool valid() const  { return (mHandle > 0); }

			void clear()
			{
				if (mHandle <= 0)
					return;
				glDeleteLists(mHandle, 1);
				mHandle = 0;
			}

			void begin()
			{
				if (mHandle <= 0)
					mHandle = glGenLists(1);
				glNewList(mHandle, GL_COMPILE);
			}

			void end()
			{
				glEndList();
			}

			void render()
			{
				glCallList(mHandle);
			}

		private:
			int mHandle = 0;
		};
	#endif


	#define CHECK_OPENGL_ERROR() \
	{ \
		GLenum err; \
		while ((err = glGetError()) != GL_NO_ERROR) \
		{ \
			RMX_ERROR("OpenGL error in file '" << __FILE__ << "', line " << __LINE__ << ": " << getGLErrorDescription(err), ); \
		} \
	}

#endif
