/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class UpscalerDefinition
{
public:
	struct Variant
	{
		int mVariantID = 0;		// Usually the variant's index, starting at zero
		std::string mDisplayName;
		bool mFilterLinear = false;
	};

	struct ShaderInfo
	{
		std::wstring mPath;
		std::string mTechnique;
	};

public:
	uint64 mNameHash = 0;	// String hash of internal name
	std::string mInternalName;
	std::string mDisplayName;
	std::vector<Variant> mVariants;

	std::vector<ShaderInfo> mOpenGLShaders;
	std::vector<std::wstring> mLookupTextures;

public:
	Variant& addVariant(int id, std::string_view name);
	ShaderInfo& addOpenGLShader(std::wstring_view path, std::string_view technique);
};
