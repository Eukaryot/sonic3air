/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	struct SourceFileInfo
	{
		std::wstring mFilename;		// File name only, without path
		std::wstring mFullPath;		// Path including file name
		size_t mIndex = 0;
	};
}
