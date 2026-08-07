/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Platforms overview:
//  - PLATFORM_WINDOWS	-> Windows
//  - PLATFORM_LINUX	-> Linux
//  - PLATFORM_MAC		-> macOS
//  - PLATFORM_ANDROID	-> Android
//  - PLATFORM_IOS		-> iOS
//  - PLATFORM_WEB		-> Web version (via emscripten)
//  - PLATFORM_SWITCH	-> Nintendo Switch (homebrew)
//  - PLATFORM_VITA		-> Playstation Vita (homebrew)


// Platform specific
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define PLATFORM_WINDOWS
	#if defined(__GNUC__)
		#define USE_UTF8_PATHS
	#endif

#elif __linux__ && !__ANDROID__
	#define PLATFORM_LINUX
	#define USE_UTF8_PATHS		// Linux supports UTF-8 file names instead of wchar_t

#elif __APPLE__
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		#define PLATFORM_IOS
	#else
		#define PLATFORM_MAC
	#endif
	#define USE_UTF8_PATHS

#elif __ANDROID__
	#define PLATFORM_ANDROID
	#define USE_UTF8_PATHS

#elif __EMSCRIPTEN__
	#define PLATFORM_WEB
	#define USE_UTF8_PATHS

#elif __SWITCH__
	#define PLATFORM_SWITCH
	#define USE_UTF8_PATHS

#elif __vita__
	#define PLATFORM_VITA
	#define USE_UTF8_PATHS

#else
	#error "Unsupported platform"
#endif


// Compiler specific
#if defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
	#define RESTRICT __restrict

#elif defined(__GNUC__)
	#define FORCE_INLINE __attribute__((always_inline)) inline
	#define RESTRICT __restrict__

#elif defined(__clang__)
	#define FORCE_INLINE inline
	#define RESTRICT __restrict__

#else
	#define FORCE_INLINE inline
	#define RESTRICT
#endif
