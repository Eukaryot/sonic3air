/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Library linking via pragma
#if defined(PLATFORM_WINDOWS) && defined(RMX_LIB)
	#pragma comment(lib, "sdl2main.lib")
	#pragma comment(lib, "sdl2.lib")
	#pragma comment(lib, "winmm.lib")
	#pragma comment(lib, "imm32.lib")
	#pragma comment(lib, "version.lib")
	#pragma comment(lib, "setupapi.lib")
	#pragma comment(lib, "opengl32.lib")
#endif

// This is for some reason needed under Linux
#if defined(__GNUC__) && __GNUC__ >= 4
	#define DECLSPEC __attribute__ ((visibility("default")))
#endif


// SDL
#ifdef PLATFORM_WINDOWS
	// Needed for MSYS2
	#if defined(__GNUC__)
		#include <SDL2/SDL.h>
	#else
		#include <SDL/SDL.h>
	#endif
#else
	#include <SDL.h>
#endif

// OpenGL
#if defined(PLATFORM_MAC)
	#define ALLOW_LEGACY_OPENGL		// Should be removed for macOS I guess?
	#include <OpenGL/gl3.h>
	#include <OpenGL/glu.h>
#elif defined(PLATFORM_WEB)
	#include <GL/glew.h>
#elif defined(PLATFORM_ANDROID)
	#define RMX_USE_GLES2
	#define GL_GLEXT_PROTOTYPES
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#elif defined(PLATFORM_IOS)
	#define RMX_USE_GLES2
	#define GL_GLEXT_PROTOTYPES
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>
#elif defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
	#define ALLOW_LEGACY_OPENGL
	#define RMX_USE_GLEW
#elif defined(PLATFORM_SWITCH)
	#include <EGL/egl.h>    // EGL library
	#include <EGL/eglext.h> // EGL extensions
	#include <glad/glad.h>  // glad library (OpenGL loader)
	#define RMX_USE_GLAD
	#define GL_LUMINANCE GL_RED
#else
	#error Unsupported platform
#endif

#if defined(RMX_USE_GLES2) && !defined(__EMSCRIPTEN__)
	#define GL_RGB8				 GL_RGB
	#define GL_RGBA8			 GL_RGBA
	#define glGenVertexArrays	 glGenVertexArraysOES
	#define glDeleteVertexArrays glDeleteVertexArraysOES
	#define glBindVertexArray	 glBindVertexArrayOES
	#define glClearDepth		 glClearDepthf
	#define glDepthRange		 glDepthRangef
#endif

#ifdef RMX_USE_GLEW
	#ifndef GLEW_STATIC
		#define GLEW_STATIC
	#endif
	#define GLEW_NO_GLU
	#include "rmxmedia/glew/GL/glew.h"
#endif
