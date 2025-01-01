/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	class Module;

	struct SourceFileInfo
	{
		Module* mModule = nullptr;	// Module that this is part of
		std::wstring mFilename;		// File name only, without path
		std::wstring mLocalPath;	// Local path relative to the main script
		size_t mIndex = 0;			// Index inside the array of source file infos

		std::wstring getFullFilePath() const;
		std::wstring getLocalFilePath() const;
	};
}
