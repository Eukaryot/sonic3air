/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	// To export classes, methods and variables
	#define GENERIC_API_EXPORT	__declspec(dllexport)

	// To export functions
	#define GENERIC_FUNCTION_EXPORT	extern "C" __declspec(dllexport)

#elif defined(__GNUC__) && __GNUC__ >= 4
	// To export classes, methods and variables
	#if defined(HAVE_VISIBILITY_ATTR)
		#define GENERIC_API_EXPORT __attribute__ ((visibility("default")))
	#else
		#define GENERIC_API_EXPORT
	#endif

	// To export functions
	#define GENERIC_FUNCTION_EXPORT	__attribute__ ((visibility("default")))

#elif __APPLE__
	// To export classes, methods and variables
	#define GENERIC_API_EXPORT __attribute__ ((visibility("default")))

	// To export functions
	#define GENERIC_FUNCTION_EXPORT	extern "C" __attribute__ ((visibility("default")))

#else
	#error "Unsupported platform"
#endif


#ifdef _WINDLL
	#define _USRDLL
#endif

#ifdef _USRDLL
	// Export
	#define API_EXPORT		GENERIC_API_EXPORT
	#define FUNCTION_EXPORT	GENERIC_FUNCTION_EXPORT
	#define TEMPLATE_EXPORT
#else
	// Build as static library
	#define API_EXPORT
	#define FUNCTION_EXPORT
	#define TEMPLATE_EXPORT
#endif
