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
#include "rmxmedia/FileProviderSDL.h"
#include "rmxmedia/FileInputStreamSDL.h"
#include "rmxmedia/Texture.h"
#include "rmxmedia/SpriteAtlas.h"
#include "rmxmedia/Shader.h"
#include "rmxmedia/Framebuffer.h"
#include "rmxmedia/VertexArrayObject.h"
#include "rmxmedia/Camera.h"
#include "rmxmedia/FontSource.h"
#include "rmxmedia/Font.h"
#include "rmxmedia/FontOutput.h"
#include "rmxmedia/FontProcessor.h"
#include "rmxmedia/GLTools.h"
#include "rmxmedia/Painter.h"
#include "rmxmedia/Thread.h"
#include "rmxmedia/VideoBuffer.h"
#include "rmxmedia/AudioBuffer.h"
#include "rmxmedia/AudioReference.h"
#include "rmxmedia/AudioMixer.h"
#include "rmxmedia/JobManager.h"
#include "rmxmedia/GuiBase.h"
#include "rmxmedia/AppFramework.h"
#include "rmxmedia/FTX_System.h"


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
