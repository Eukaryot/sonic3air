/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>	// Particularly for rmxbase's "PlatformDefinitions.h"



// --- Platform type: Desktop, Mobile, Console ---

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
	#define PLATFORM_IS_DESKTOP

#elif defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_WEB)
	#define PLATFORM_IS_MOBILE

#elif defined(PLATFORM_SWITCH) || defined(PLATFORM_VITA)
	#define PLATFORM_IS_CONSOLE
#endif



// --- Platform input devices ---

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
	#define PLATFORM_HAS_HARDWARE_KEYBOARD
	#define PLATFORM_HAS_MOUSE

#elif defined(PLATFORM_ANDROID)
	// TODO: Other platforms may have a virtual keyboard as well, but it's only tested on Android
	#define PLATFORM_HAS_VIRTUAL_KEYBOARD
	#define PLATFORM_HAS_TOUCH_INPUT

#elif defined(PLATFORM_IOS) || defined(PLATFORM_WEB)
	// For Web, we can't actually be sure about touch - but there's probably at least a mouse that can serve as touch input
	#define PLATFORM_HAS_TOUCH_INPUT

#endif



// --- Naming of things: "directory" or "folder"? ---

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_WEB)
	#define PLATFORM_DIRECTORY_STRING "folder"
#else
	#define PLATFORM_DIRECTORY_STRING "directory"
#endif
