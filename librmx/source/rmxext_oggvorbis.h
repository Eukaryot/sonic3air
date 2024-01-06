/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

// Version
#define RMXEXT_OGGVORBIS_VERSION 0x00030200


// Library linking via pragma
#if defined(PLATFORM_WINDOWS) && defined(RMX_LIB)
	#pragma comment(lib, "rmxext_oggvorbis.lib")
    #pragma comment(lib, "libogg.lib")
    #pragma comment(lib, "libvorbis_static.lib")
    #pragma comment(lib, "libvorbisfile_static.lib")
#endif

// General includes
#include <vorbis/codec.h>
#include <rmxmedia.h>

// RMX modules
#include "rmxext_oggvorbis/OggLoader.h"


// Initialization
namespace rmxext_oggvorbis
{
    void initialize();
}

#undef INIT_RMXEXT_OGGVORBIS
#define INIT_RMXEXT_OGGVORBIS  { rmxext_oggvorbis::initialize(); }
