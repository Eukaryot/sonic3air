/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

// Enable or disable ImGui support, depending on the platform to build for
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_MAC)
	#define SUPPORT_IMGUI
#endif


#if defined(SUPPORT_IMGUI)

#include "imgui.h"

#endif
