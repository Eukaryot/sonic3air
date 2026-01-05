/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

static ImVec2 operator+(ImVec2 a, ImVec2 b)
{
	ImVec2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

static ImVec2 operator-(ImVec2 a, ImVec2 b)
{
	ImVec2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

static ImVec2 operator*(ImVec2 a, float multiplier)
{
	ImVec2 result;
	result.x = a.x * multiplier;
	result.y = a.y * multiplier;
	return result;
}

static ImVec4 operator+(ImVec4 a, ImVec4 b)
{
	ImVec4 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = a.w + b.w;
	return result;
}

static ImVec4 operator-(ImVec4 a, ImVec4 b)
{
	ImVec4 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = a.w - b.w;
	return result;
}

static ImVec4 operator*(ImVec4 a, float multiplier)
{
	ImVec4 result;
	result.x = a.x * multiplier;
	result.y = a.y * multiplier;
	result.z = a.z * multiplier;
	result.w = a.w * multiplier;
	return result;
}


// Extend ImGui namespace itself by some helpful functions
namespace ImGui
{
	// Use instead of "SameLine" if you want to write two texts after another without any additional space (e.g. when just changing text colors)
	extern void SameLineNoSpace();

	// Use instead of "SameLine" if you want to have more control over the space in between elements
	extern void SameLineRelSpace(float relativeSpace);

	// A red button that really does not seem to want to be pressed
	extern bool RedButton(const char* label, const ImVec2& size = ImVec2(0, 0));
}

#endif
