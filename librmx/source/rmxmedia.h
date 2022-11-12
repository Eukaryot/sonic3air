/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

// Version
#define RMXMEDIA_VERSION 0x00040100


// This is for some reason needed under Linux
#if defined(__GNUC__) && __GNUC__ >= 4
	#define DECLSPEC __attribute__ ((visibility("default")))
#endif


// General includes
#include "rmxbase.h"
#include "rmxmedia_externals.h"

// RMX modules
#include "rmxmedia/file/FileProviderSDL.h"
#include "rmxmedia/file/FileInputStreamSDL.h"
#include "rmxmedia/font/FontSource.h"
#include "rmxmedia/font/Font.h"
#include "rmxmedia/font/FontProcessor.h"
#include "rmxmedia/opengl/Texture.h"
#include "rmxmedia/opengl/SpriteAtlas.h"
#include "rmxmedia/opengl/Shader.h"
#include "rmxmedia/opengl/Framebuffer.h"
#include "rmxmedia/opengl/VertexArrayObject.h"
#include "rmxmedia/opengl/OpenGLFontOutput.h"
#include "rmxmedia/opengl/GLTools.h"
#include "rmxmedia/opengl/Painter.h"
#include "rmxmedia/threads/Thread.h"
#include "rmxmedia/audiovideo/VideoBuffer.h"
#include "rmxmedia/audiovideo/AudioBuffer.h"
#include "rmxmedia/audiovideo/AudioReference.h"
#include "rmxmedia/audiovideo/AudioMixer.h"
#include "rmxmedia/threads/JobManager.h"
#include "rmxmedia/framework/GuiBase.h"
#include "rmxmedia/framework/AppFramework.h"
#include "rmxmedia/framework/FTX_System.h"


// Library linking via pragma
#if defined(PLATFORM_WINDOWS) && defined(RMX_LIB)
	#pragma comment(lib, "rmxmedia.lib")
#endif



// Singletons
namespace FTX
{
	extern SingletonPtr<rmx::JobManager>		JobManager;
	extern SingletonPtr<rmx::FTX_SystemManager>	System;
	extern SingletonPtr<rmx::FTX_VideoManager>	Video;
	extern SingletonPtr<rmx::AudioManager>		Audio;
	extern SingletonPtr<rmx::Painter>			Painter;
};

// Initialization
namespace rmxmedia
{
	void initialize();
	void getBuildInfo(String& info);
}

#undef INIT_RMX
#define INIT_RMX  { rmxmedia::initialize(); }
