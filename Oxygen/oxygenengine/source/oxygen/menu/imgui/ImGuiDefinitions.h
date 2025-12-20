/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

// Enable or disable ImGui support, depending on the platform to build for
#if defined(PLATFORM_WINDOWS) || (defined(PLATFORM_LINUX) && defined(USE_IMGUI)) || defined(PLATFORM_MAC) || defined(PLATFORM_ANDROID) || (defined(PLATFORM_WEB) && defined(USE_IMGUI))
	#define SUPPORT_IMGUI
#endif


#if defined(SUPPORT_IMGUI)

#include "imgui.h"

#endif
